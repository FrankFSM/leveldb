// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

/**
 * `TableCache` 类是 LevelDB 中的一个关键组件，它负责缓存已打开的 SSTable 文件（也称为表文件），以加速键值对的查找和迭代操作。`TableCache` 类的实现位于 `db/table_cache.cc` 和 `db/table_cache.h` 文件中。

以下是 `TableCache` 类的详细说明：

    1. 类的作用：`TableCache` 类的主要作用是提供一个高效的缓存机制，以避免频繁地打开和关闭 SSTable 文件。它通过将打开的文件和相应的 `Table` 对象存储在一个 LRU（最近最少使用）缓存中来实现这一点。当需要访问一个 SSTable 文件时，`TableCache` 首先检查缓存中是否已经存在该文件的 `Table` 对象。如果存在，则直接使用缓存中的对象；否则，它会打开文件并将新创建的 `Table` 对象添加到缓存中。为了防止缓存占用过多内存，`TableCache` 类会限制缓存中的最大条目数。当缓存满时，它会根据 LRU 策略删除最近最少使用的条目。

    2. 类的成员：

    - `env_`：一个 `Env` 指针，表示操作系统环境。`TableCache` 类使用 `env_` 成员来执行文件 I/O 操作。

    - `options_`：一个 `Options` 对象，表示数据库的配置选项。`TableCache` 类使用 `options_` 成员来获取缓存策略、文件大小等设置。

    - `dbname_`：一个字符串，表示数据库的名称。`TableCache` 类使用 `dbname_` 成员来构造 SSTable 文件的路径。

    - `cache_`：一个 `Cache` 指针，表示 LRU 缓存。`TableCache` 类使用 `cache_` 成员来存储已打开的 SSTable 文件和相应的 `Table` 对象。

    3. 类的成员函数：

    - `FindTable`：查找指定文件号的 `Table` 对象。这个函数首先检查缓存中是否已经存在该文件的 `Table` 对象。如果存在，则返回缓存中的对象；否则，它会打开文件并将新创建的 `Table` 对象添加到缓存中。

    - `Get`：获取指定文件号和内部键的值。这个函数会先调用 `FindTable` 函数找到相应的 `Table` 对象，然后调用 `Table::InternalGet` 函数获取键值对。

    - `NewIterator`：为指定文件号创建一个迭代器。这个函数会先调用 `FindTable` 函数找到相应的 `Table` 对象，然后调用 `Table::NewIterator` 函数创建迭代器。

    - `Evict`：从缓存中删除指定文件号的 `Table` 对象。这个函数在删除不再需要的 SSTable 文件时被调用，以确保缓存中不会保留已删除文件的 `Table` 对象。

          通过这些成员变量和成员函数，`TableCache` 类实现了一个高效的缓存机制，以加速键值对的查找和迭代操作。在实际应用中，`TableCache` 类可以有效地减少磁盘 I/O 开销，提高数据库的性能。
 */

#include "db/table_cache.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

namespace leveldb {

struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
};

static void DeleteEntry(const Slice& key, void* value) {
  TableAndFile* tf = reinterpret_cast<TableAndFile*>(value);
  delete tf->table;
  delete tf->file;
  delete tf;
}

static void UnrefEntry(void* arg1, void* arg2) {
  Cache* cache = reinterpret_cast<Cache*>(arg1);
  Cache::Handle* h = reinterpret_cast<Cache::Handle*>(arg2);
  cache->Release(h);
}

TableCache::TableCache(const std::string& dbname, const Options& options,
                       int entries)
    : env_(options.env),
      dbname_(dbname),
      options_(options),
      cache_(NewLRUCache(entries)) {}

TableCache::~TableCache() { delete cache_; }

Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  *handle = cache_->Lookup(key);
  if (*handle == nullptr) {
    std::string fname = TableFileName(dbname_, file_number);
    RandomAccessFile* file = nullptr;
    Table* table = nullptr;
    s = env_->NewRandomAccessFile(fname, &file);
    if (!s.ok()) {
      std::string old_fname = SSTTableFileName(dbname_, file_number);
      if (env_->NewRandomAccessFile(old_fname, &file).ok()) {
        s = Status::OK();
      }
    }
    if (s.ok()) {
      s = Table::Open(options_, file, file_size, &table);
    }

    if (!s.ok()) {
      assert(table == nullptr);
      delete file;
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  }
  return s;
}

Iterator* TableCache::NewIterator(const ReadOptions& options,
                                  uint64_t file_number, uint64_t file_size,
                                  Table** tableptr) {
  if (tableptr != nullptr) {
    *tableptr = nullptr;
  }

  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (!s.ok()) {
    return NewErrorIterator(s);
  }

  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  Iterator* result = table->NewIterator(options);
  result->RegisterCleanup(&UnrefEntry, cache_, handle);
  if (tableptr != nullptr) {
    *tableptr = table;
  }
  return result;
}

Status TableCache::Get(const ReadOptions& options, uint64_t file_number,
                       uint64_t file_size, const Slice& k, void* arg,
                       void (*handle_result)(void*, const Slice&,
                                             const Slice&)) {
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (s.ok()) {
    Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
    s = t->InternalGet(options, k, arg, handle_result);
    cache_->Release(handle);
  }
  return s;
}

void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}

}  // namespace leveldb
