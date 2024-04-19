// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDB_INCLUDE_DB_H_

#include <cstdint>
#include <cstdio>

#include "leveldb/export.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"

namespace leveldb {

// Update CMakeLists.txt if you change these
static const int kMajorVersion = 1;
static const int kMinorVersion = 23;

struct Options;
struct ReadOptions;
struct WriteOptions;
class WriteBatch;

// Abstract handle to particular state of a DB.
// A Snapshot is an immutable object and can therefore be safely
// accessed from multiple threads without any external synchronization.
class LEVELDB_EXPORT Snapshot {
 protected:
  virtual ~Snapshot();
};

// A range of keys
struct LEVELDB_EXPORT Range {
  Range() = default;
  Range(const Slice& s, const Slice& l) : start(s), limit(l) {}

  Slice start;  // Included in the range
  Slice limit;  // Not included in the range
};

// A DB is a persistent ordered map from keys to values.
// A DB is safe for concurrent access from multiple threads without
// any external synchronization.
class LEVELDB_EXPORT DB {
 public:

  /**
   * 打开一个数据库。这个函数接收一个Options结构体和一个数据库路径作为参数，然后返回一个DB对象
   * @param options
   * @param name
   * @param dbptr
   * @return
   */
  // Open the database with the specified "name".
  // Stores a pointer to a heap-allocated database in *dbptr and returns
  // OK on success.
  // Stores nullptr in *dbptr and returns a non-OK status on error.
  // Caller should delete *dbptr when it is no longer needed.
  static Status Open(const Options& options, const std::string& name,
                     DB** dbptr);

  DB() = default;

  DB(const DB&) = delete;
  DB& operator=(const DB&) = delete;

  virtual ~DB();

  /**
   * 插入或更新一个键值对。这个函数接收一个WriteOptions对象和一个键值对作为参数
   */
  // Set the database entry for "key" to "value".  Returns OK on success,
  // and a non-OK status on error.
  // Note: consider setting options.sync = true.
  virtual Status Put(const WriteOptions& options, const Slice& key,
                     const Slice& value) = 0;

  /**
   * 删除一个键值对。这个函数接收一个WriteOptions对象和一个键作为参数
   */
  // Remove the database entry (if any) for "key".  Returns OK on
  // success, and a non-OK status on error.  It is not an error if "key"
  // did not exist in the database.
  // Note: consider setting options.sync = true.
  virtual Status Delete(const WriteOptions& options, const Slice& key) = 0;


  /**
   * 功能差异：DB::Put函数用于插入或更新单个键值对。它接收一个键和一个值作为参数，并将键值对写入数据库。而DB::Write函数则用于将一系列修改操作（例如插入、删除等）应用到数据库。它接收一个WriteBatch对象作为参数，这个对象可以包含多个修改操作。
   * 原子性差异：DB::Put函数仅保证单个键值对写入的原子性。也就是说，如果在执行多个DB::Put操作时发生故障，可能会出现部分操作成功、部分操作失败的情况。而DB::Write函数则可以保证一系列修改操作的原子性。这意味着，如果在执行DB::Write操作时发生故障，要么所有操作都成功，要么所有操作都失败。
   * 用途差异：DB::Put函数适用于简单的插入或更新操作，特别是当你只需要修改一个键值对时。而DB::Write函数则适用于需要批量处理多个修改操作的场景，例如事务操作或批量导入数据等。
   */
  // Apply the specified updates to the database.
  // Returns OK on success, non-OK on failure.
  // Note: consider setting options.sync = true.
  virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;


  /**
   * 获取一个键的值。这个函数接收一个ReadOptions对象和一个键作为参数，然后返回键对应的值
   */
  // If the database contains an entry for "key" store the
  // corresponding value in *value and return OK.
  //
  // If there is no entry for "key" leave *value unchanged and return
  // a status for which Status::IsNotFound() returns true.
  //
  // May return some other Status on an error.
  virtual Status Get(const ReadOptions& options, const Slice& key,
                     std::string* value) = 0;

  /**
   * 用于创建一个可以遍历数据库中键值对的迭代器。迭代器可以按照键的顺序访问数据库中的所有键值对，并支持查找、范围查询等操作。
   * 可以方便地遍历LevelDB数据库中的所有键值对，并执行查找、范围查询等操作。这对于实现数据分析、导出等功能非常有用
   * @param options
   * @return
   */
  // Return a heap-allocated iterator over the contents of the database.
  // The result of NewIterator() is initially invalid (caller must
  // call one of the Seek methods on the iterator before using it).
  //
  // Caller should delete the iterator when it is no longer needed.
  // The returned iterator should be deleted before this db is deleted.
  virtual Iterator* NewIterator(const ReadOptions& options) = 0;

  /**
   * 获取快照。快照提供了数据库在某一时刻的只读视图
   * @return
   */
  // Return a handle to the current DB state.  Iterators created with
  // this handle will all observe a stable snapshot of the current DB
  // state.  The caller must call ReleaseSnapshot(result) when the
  // snapshot is no longer needed.
  virtual const Snapshot* GetSnapshot() = 0;

  /**
   * 释放快照
   * const leveldb::Snapshot* snapshot = db->GetSnapshot();
   * 使用快照读取数据
   * leveldb::ReadOptions options;
   * options.snapshot = snapshot;
   * leveldb::Iterator* it = db->NewIterator(options);
   * 释放快照
   * db->ReleaseSnapshot(snapshot);
   * @param snapshot
   */
  // Release a previously acquired snapshot.  The caller must not
  // use "snapshot" after this call.
  virtual void ReleaseSnapshot(const Snapshot* snapshot) = 0;

  /**
   * 获取数据库的属性，如统计信息、内部状态等
   */
  // DB implementations can export properties about their state
  // via this method.  If "property" is a valid property understood by this
  // DB implementation, fills "*value" with its current value and returns
  // true.  Otherwise returns false.
  //
  //
  // Valid property names include:
  //
  //  "leveldb.num-files-at-level<N>" - return the number of files at level <N>,
  //     where <N> is an ASCII representation of a level number (e.g. "0").
  //  "leveldb.stats" - returns a multi-line string that describes statistics
  //     about the internal operation of the DB.
  //  "leveldb.sstables" - returns a multi-line string that describes all
  //     of the sstables that make up the db contents.
  //  "leveldb.approximate-memory-usage" - returns the approximate number of
  //     bytes of memory in use by the DB.
  virtual bool GetProperty(const Slice& property, std::string* value) = 0;

  /**
   * 用于获取数据库中一系列键范围的近似大小。这个方法可以帮助你了解数据库中不同键范围的存储空间占用情况，
   * 从而进行优化或者分析。它接收一个Range数组作为参数，这个数组表示要查询的键范围。GetApproximateSizes方法还接收一个整数参数，
   * 表示数组中的范围数量，以及一个uint64_t数组，用于存储返回的近似大小
   */
  // For each i in [0,n-1], store in "sizes[i]", the approximate
  // file system space used by keys in "[range[i].start .. range[i].limit)".
  //
  // Note that the returned sizes measure file system space usage, so
  // if the user data compresses by a factor of ten, the returned
  // sizes will be one-tenth the size of the corresponding user data size.
  //
  // The results may not include the sizes of recently written data.
  virtual void GetApproximateSizes(const Range* range, int n,
                                   uint64_t* sizes) = 0;

  /**
   * 用于手动触发一次Compaction操作。Compaction操作的主要目的是合并和压缩数据库中的数据，
   * 从而减小存储空间占用并提高查询性能。在正常情况下，LevelDB会自动执行后台Compaction操作。
   * 然而，在某些情况下，你可能需要手动触发Compaction，例如在导入大量数据后或者在数据库空闲时
   *
   * 注意：可能会阻塞一段时间，因为它需要执行磁盘I/O操作。
   *    在大型数据库中，Compaction操作可能需要很长时间才能完成。因此，在调用DB::CompactRange方法时，你需要考虑其对性能的影响。
   */
  // Compact the underlying storage for the key range [*begin,*end].
  // In particular, deleted and overwritten versions are discarded,
  // and the data is rearranged to reduce the cost of operations
  // needed to access the data.  This operation should typically only
  // be invoked by users who understand the underlying implementation.
  //
  // begin==nullptr is treated as a key before all keys in the database.
  // end==nullptr is treated as a key after all keys in the database.
  // Therefore the following call will compact the entire database:
  //    db->CompactRange(nullptr, nullptr);
  virtual void CompactRange(const Slice* begin, const Slice* end) = 0;
};

/**
 * 此方法用于删除一个数据库，包括其所有文件（例如SSTable、WAL日志等）。
 * 在某些情况下，你可能需要彻底删除一个数据库，例如在测试过程中或者在数据库不再需要时。DestroyDB方法接收一个数据库路径和一个Options对象作为参数
 *
 * 需要注意的是，需要执行磁盘I/O操作，因此可能会阻塞一段时间。调用此方法时需要考虑其对性能的影响。
 *  另外，在执行这些方法之前，确保已经关闭了对应的数据库实例，以避免数据不一致或其他错误
 */
// Destroy the contents of the specified database.
// Be very careful using this method.
//
// Note: For backwards compatibility, if DestroyDB is unable to list the
// database files, Status::OK() will still be returned masking this failure.
LEVELDB_EXPORT Status DestroyDB(const std::string& name,
                                const Options& options);

/**
 * 此方法用于修复一个损坏的数据库。在某些情况下，数据库可能会因为硬件故障、软件错误等原因变得不一致或损坏。
 * RepairDB方法可以尝试恢复损坏的数据库，使其重新变得可用。RepairDB方法接收一个数据库路径和一个Options对象作为参数
 *
 *
 * 需要注意的是，需要执行磁盘I/O操作，因此可能会阻塞一段时间。调用此方法时需要考虑其对性能的影响。
 *  另外，在执行这些方法之前，确保已经关闭了对应的数据库实例，以避免数据不一致或其他错误
 */
// If a DB cannot be opened, you may attempt to call this method to
// resurrect as much of the contents of the database as possible.
// Some data may be lost, so be careful when calling this function
// on a database that contains important information.
LEVELDB_EXPORT Status RepairDB(const std::string& dbname,
                               const Options& options);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_DB_H_
