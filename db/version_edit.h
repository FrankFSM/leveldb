// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_VERSION_EDIT_H_
#define STORAGE_LEVELDB_DB_VERSION_EDIT_H_

#include <set>
#include <utility>
#include <vector>

#include "db/dbformat.h"

namespace leveldb {

class VersionSet;


/**
 * 主要作用是存储和管理 SSTable 文件的元数据信息，如文件号、文件大小、键范围等。
 * 这些元数据信息在 LevelDB 的各种操作中起到了关键作用，
 * 如查询、迭代、Compaction 等。通过使用 `FileMetaData` 结构体，LevelDB 可以有效地组织和管理 SSTable 文件，从而提高数据库的性能和可靠性。
 */

struct FileMetaData {
  FileMetaData() : refs(0), allowed_seeks(1 << 30), file_size(0) {}

  // 表示当前 `FileMetaData` 对象的引用计数。在 LevelDB 中，`FileMetaData` 对象可能被多个 Version 对象共享，因此需要使用引用计数来确保对象在使用过程中不被意外删除。
  int refs;
  // 表示当前文件允许的最大查找次数。这个值主要用于控制 Compaction 的触发时机。当文件的查找次数超过 `allowed_seeks` 时，会触发一次 Compaction，以降低查询开销。
  int allowed_seeks;  // Seeks allowed until compaction

  // 表示 SSTable 文件的文件号。这个值用于唯一标识一个 SSTable 文件，并在文件系统中构造文件的路径。
  uint64_t number;
  // 表示 SSTable 文件的大小。这个值用于计算数据库的总存储空间和 Compaction 的触发条件。
  uint64_t file_size;    // File size in bytes
  // 一个 `InternalKey` 对象，表示 SSTable 文件中的最小键。这个值用于加速查询和迭代操作，通过判断查询键是否在文件的键范围内，可以快速跳过不相关的文件。
  InternalKey smallest;  // Smallest internal key served by table
  // 一个 `InternalKey` 对象，表示 SSTable 文件中的最大键。这个值同样用于加速查询和迭代操作，通过判断查询键是否在文件的键范围内，可以快速跳过不相关的文件。
  InternalKey largest;   // Largest internal key served by table
};


/**
 * `VersionEdit`类是LevelDB中的一个关键组件，用于表示数据库元数据的更改。
 * 它主要用于记录和应用与数据库状态相关的更改，例如添加或删除SSTable文件、设置日志文件号等。
 * `VersionEdit`对象通常与`VersionSet`一起使用，后者负责管理数据库的所有版本。
 */
class VersionEdit {
 public:
  VersionEdit() { Clear(); }
  ~VersionEdit() = default;

  // 清除此`VersionEdit`对象的所有更改。
  void Clear();

  // 设置比较器名称。这个名称用于在不同版本之间保持键的顺序一致。
  void SetComparatorName(const Slice& name) {
    has_comparator_ = true;
    comparator_ = name.ToString();
  }

  // 设置日志文件号。这个数字用于标识当前活动的日志文件。
  void SetLogNumber(uint64_t num) {
    has_log_number_ = true;
    log_number_ = num;
  }

  // 设置前一个日志文件号。这个数字用于在恢复过程中识别旧的日志文件。
  void SetPrevLogNumber(uint64_t num) {
    has_prev_log_number_ = true;
    prev_log_number_ = num;
  }

  // 设置下一个文件号。这个数字用于分配新的SSTable文件和日志文件的文件号。
  void SetNextFile(uint64_t num) {
    has_next_file_number_ = true;
    next_file_number_ = num;
  }

  // 设置最后一个序列号。这个序列号用于为写入操作分配一个全局唯一的序列号。
  void SetLastSequence(SequenceNumber seq) {
    has_last_sequence_ = true;
    last_sequence_ = seq;
  }

  // 设置压缩指针。压缩指针用于标识在每个级别上需要压缩的键范围。
  void SetCompactPointer(int level, const InternalKey& key) {
    compact_pointers_.push_back(std::make_pair(level, key));
  }

  // 添加一个新的SSTable文件到指定的级别。这个方法需要文件号、文件大小、文件中的最小键和最大键作为参数。
  // Add the specified file at the specified number.
  // REQUIRES: This version has not been saved (see VersionSet::SaveTo)
  // REQUIRES: "smallest" and "largest" are smallest and largest keys in file
  void AddFile(int level, uint64_t file, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest) {
    FileMetaData f;
    f.number = file;
    f.file_size = file_size;
    f.smallest = smallest;
    f.largest = largest;
    new_files_.push_back(std::make_pair(level, f));
  }

  // 从指定的级别中删除一个SSTable文件。这个方法需要文件号和级别作为参数。
  // Delete the specified "file" from the specified "level".
  void RemoveFile(int level, uint64_t file) {
    deleted_files_.insert(std::make_pair(level, file));
  }

  // 将此`VersionEdit`对象的更改编码到一个字符串中。这个字符串可以写入磁盘或在进程间传输。
  void EncodeTo(std::string* dst) const;

  // 从一个字符串中解码`VersionEdit`对象的更改。这个字符串通常是通过`EncodeTo()`方法生成的。
  Status DecodeFrom(const Slice& src);

  // 返回此`VersionEdit`对象的调试字符串。这个字符串包含了所有的更改，用于调试和日志记录。
  std::string DebugString() const;

 private:
  friend class VersionSet;

  typedef std::set<std::pair<int, uint64_t>> DeletedFileSet;

  // 存储比较器名称的字符串。比较器用于在不同版本之间保持键的顺序一致。
  std::string comparator_;
  // 当前活动的日志文件号。
  uint64_t log_number_;
  // 前一个日志文件号。这个数字用于在恢复过程中识别旧的日志文件。
  uint64_t prev_log_number_;
  // 下一个文件号。这个数字用于分配新的SSTable文件和日志文件的文件号。
  uint64_t next_file_number_;
  // 最后一个序列号。这个序列号用于为写入操作分配一个全局唯一的序列号。
  SequenceNumber last_sequence_;
  // 表示相应的元数据项是否已设置
  bool has_comparator_;
  bool has_log_number_;
  bool has_prev_log_number_;
  bool has_next_file_number_;
  bool has_last_sequence_;
  // 压缩指针的列表。每个压缩指针表示在每个级别上需要压缩的键范围。
  std::vector<std::pair<int, InternalKey>> compact_pointers_;
  // 已删除文件的集合。每个元素是一个包含级别和文件号的对。
  DeletedFileSet deleted_files_;
  // 新添加文件的列表。每个元素是一个包含级别和文件元数据的对。
  std::vector<std::pair<int, FileMetaData>> new_files_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_VERSION_EDIT_H_
