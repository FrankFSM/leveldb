// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

/**
 * `VersionSet` 类是 LevelDB 中的一个核心组件，它负责管理数据库的所有版本信息，包括 SSTable 文件（也称为表文件）、元数据以及与 Compaction 相关的信息。`VersionSet` 类的实现位于 `db/version_set.cc` 和 `db/version_set.h` 文件中。

以下是 `VersionSet` 类的详细说明：

    1. 类的作用：`VersionSet` 类的主要作用是管理不同版本的数据库状态。在 LevelDB 中，每次写操作都会产生一个新的数据库状态，这些状态由一组 SSTable 文件组成。为了高效地存储和查找这些状态，`VersionSet` 类使用了一种称为 Version 的轻量级数据结构。每个 Version 对象都包含了一个指向 SSTable 文件列表的指针，以及一个指向下一个 Version 对象的指针。这样，`VersionSet` 类可以通过链接不同的 Version 对象来表示数据库的历史状态，从而实现高效的版本管理。

    2. 类的成员：

    - `env_`：一个 `Env` 指针，表示操作系统环境。`VersionSet` 类使用 `env_` 成员来执行文件 I/O 操作、获取当前时间等。

    - `dbname_`：一个字符串，表示数据库的名称。`VersionSet` 类使用 `dbname_` 成员来构造 SSTable 文件的路径。

    - `options_`：一个 `Options` 对象，表示数据库的配置选项。`VersionSet` 类使用 `options_` 成员来获取缓存策略、比较器等设置。

    - `icmp_`：一个 `InternalKeyComparator` 对象，表示内部键的比较器。`VersionSet` 类使用 `icmp_` 成员来比较数据库中的内部键。

    - `next_file_number_`：一个整数，表示下一个 SSTable 文件的文件号。`VersionSet` 类使用 `next_file_number_` 成员来生成新的 SSTable 文件。

    - `manifest_file_number_`：一个整数，表示当前 Manifest 文件的文件号。`VersionSet` 类使用 `manifest_file_number_` 成员来记录和加载 Manifest 文件。

    - `last_sequence_`：一个整数，表示最后一个操作的序列号。`VersionSet` 类使用 `last_sequence_` 成员来生成新的操作序列号，以保证操作的顺序执行。

    - `log_number_`：一个整数，表示当前 WAL（写前日志）文件的序号。`VersionSet` 类使用 `log_number_` 成员来记录和加载 WAL 文件。

    - `prev_log_number_`：一个整数，表示前一个 WAL 文件的序号。`VersionSet` 类使用 `prev_log_number_` 成员来记录和加载前一个 WAL 文件。

    - `descriptor_log_`：一个 `log::Writer` 指针，表示当前的 Manifest 文件。`VersionSet` 类使用 `descriptor_log_` 成员来记录版本信息，以便在故障时恢复数据库状态。

    - `current_`：一个 `Version` 指针，表示当前的数据库状态。`VersionSet` 类使用 `current_` 成员来存储和查找键值对。

    - `dummy_versions_`：一个 `Version` 对象，表示一个虚拟的版本节点。`VersionSet` 类使用 `dummy_versions_` 成员来链接不同的 Version 对象，形成一个双向链表。

    3. 类的成员函数：

    - `AddVersion`：向 `VersionSet` 中添加一个新的 `Version` 对象。这个函数会将新的 `Version` 对象链接到双向链表中。

    - `AppendVersion`：将一个新的 `Version` 对象追加到当前版本。这个函数会更新 `current_` 指针，并将新的 `Version` 对象链接到双向链表中。

    - `Recover`：从 Manifest 文件中恢复版本信息。这个函数会读取 Manifest 文件，并将其中的版本信息加载到内存中。

    - `WriteSnapshot`：将当前版本的快照写入 Manifest 文件。这个函数会将当前版本的所有 SSTable 文件和元数据写入到 Manifest 文件中，以便在故障时恢复数据库状态。

- `Finalize`：确定当前版本的 Compaction 结果。这个函数会计算每个级别的分数、文件大小等信息，以便为后续的 Compaction 操作做准备。

    - `NeedsCompaction`：检查当前版本是否需要执行 Compaction 操作。这个函数会根据每个级别的分数和文件数量来判断是否需要执行 Compaction。

    - `PickCompaction`：选择一个合适的 Compaction 操作。这个函数会根据当前版本的状态和配置选项来选择一个最佳的 Compaction 操作，以便最大限度地提高数据库的性能。

    - `Get`：获取指定内部键的值。这个函数会在当前版本的所有 SSTable 文件中查找指定的内部键，并返回相应的值。

    - `NewIterator`：为当前版本创建一个迭代器。这个函数会为每个级别的 SSTable 文件创建一个迭代器，并将这些迭代器组合成一个多级迭代器，以便遍历当前版本的所有键值对。

    - `MakeInputIterator`：为指定的 Compaction 操作创建一个输入迭代器。这个函数会为 Compaction 操作的输入文件创建一个迭代器，并将这些迭代器组合成一个多级迭代器，以便遍历 Compaction 操作的所有键值对。

    - `NumLevelBytes`：获取指定级别的总字节数。这个函数会遍历指定级别的所有 SSTable 文件，并计算它们的总字节数。

    - `MaxNextLevelOverlappingBytes`：计算下一个级别的最大重叠字节数。这个函数会遍历当前级别的所有 SSTable 文件，并计算它们在下一个级别的重叠字节数。

          通过这些成员变量和成员函数，`VersionSet` 类实现了一个高效的版本管理机制，以支持 LevelDB 的快照、迭代和 Compaction 等功能。在实际应用中，`VersionSet` 类可以有效地存储和查找不同版本的数据库状态，从而提高数据库的性能和可靠性。
 */


#include "db/version_edit.h"

#include "db/version_set.h"
#include "util/coding.h"

namespace leveldb {

// Tag numbers for serialized VersionEdit.  These numbers are written to
// disk and should not be changed.
enum Tag {
  kComparator = 1,
  kLogNumber = 2,
  kNextFileNumber = 3,
  kLastSequence = 4,
  kCompactPointer = 5,
  kDeletedFile = 6,
  kNewFile = 7,
  // 8 was used for large value refs
  kPrevLogNumber = 9
};

void VersionEdit::Clear() {
  comparator_.clear();
  log_number_ = 0;
  prev_log_number_ = 0;
  last_sequence_ = 0;
  next_file_number_ = 0;
  has_comparator_ = false;
  has_log_number_ = false;
  has_prev_log_number_ = false;
  has_next_file_number_ = false;
  has_last_sequence_ = false;
  compact_pointers_.clear();
  deleted_files_.clear();
  new_files_.clear();
}

void VersionEdit::EncodeTo(std::string* dst) const {
  if (has_comparator_) {
    PutVarint32(dst, kComparator);
    PutLengthPrefixedSlice(dst, comparator_);
  }
  if (has_log_number_) {
    PutVarint32(dst, kLogNumber);
    PutVarint64(dst, log_number_);
  }
  if (has_prev_log_number_) {
    PutVarint32(dst, kPrevLogNumber);
    PutVarint64(dst, prev_log_number_);
  }
  if (has_next_file_number_) {
    PutVarint32(dst, kNextFileNumber);
    PutVarint64(dst, next_file_number_);
  }
  if (has_last_sequence_) {
    PutVarint32(dst, kLastSequence);
    PutVarint64(dst, last_sequence_);
  }

  for (size_t i = 0; i < compact_pointers_.size(); i++) {
    PutVarint32(dst, kCompactPointer);
    PutVarint32(dst, compact_pointers_[i].first);  // level
    PutLengthPrefixedSlice(dst, compact_pointers_[i].second.Encode());
  }

  for (const auto& deleted_file_kvp : deleted_files_) {
    PutVarint32(dst, kDeletedFile);
    PutVarint32(dst, deleted_file_kvp.first);   // level
    PutVarint64(dst, deleted_file_kvp.second);  // file number
  }

  for (size_t i = 0; i < new_files_.size(); i++) {
    const FileMetaData& f = new_files_[i].second;
    PutVarint32(dst, kNewFile);
    PutVarint32(dst, new_files_[i].first);  // level
    PutVarint64(dst, f.number);
    PutVarint64(dst, f.file_size);
    PutLengthPrefixedSlice(dst, f.smallest.Encode());
    PutLengthPrefixedSlice(dst, f.largest.Encode());
  }
}

static bool GetInternalKey(Slice* input, InternalKey* dst) {
  Slice str;
  if (GetLengthPrefixedSlice(input, &str)) {
    return dst->DecodeFrom(str);
  } else {
    return false;
  }
}

static bool GetLevel(Slice* input, int* level) {
  uint32_t v;
  if (GetVarint32(input, &v) && v < config::kNumLevels) {
    *level = v;
    return true;
  } else {
    return false;
  }
}

Status VersionEdit::DecodeFrom(const Slice& src) {
  Clear();
  Slice input = src;
  const char* msg = nullptr;
  uint32_t tag;

  // Temporary storage for parsing
  int level;
  uint64_t number;
  FileMetaData f;
  Slice str;
  InternalKey key;

  while (msg == nullptr && GetVarint32(&input, &tag)) {
    switch (tag) {
      case kComparator:
        if (GetLengthPrefixedSlice(&input, &str)) {
          comparator_ = str.ToString();
          has_comparator_ = true;
        } else {
          msg = "comparator name";
        }
        break;

      case kLogNumber:
        if (GetVarint64(&input, &log_number_)) {
          has_log_number_ = true;
        } else {
          msg = "log number";
        }
        break;

      case kPrevLogNumber:
        if (GetVarint64(&input, &prev_log_number_)) {
          has_prev_log_number_ = true;
        } else {
          msg = "previous log number";
        }
        break;

      case kNextFileNumber:
        if (GetVarint64(&input, &next_file_number_)) {
          has_next_file_number_ = true;
        } else {
          msg = "next file number";
        }
        break;

      case kLastSequence:
        if (GetVarint64(&input, &last_sequence_)) {
          has_last_sequence_ = true;
        } else {
          msg = "last sequence number";
        }
        break;

      case kCompactPointer:
        if (GetLevel(&input, &level) && GetInternalKey(&input, &key)) {
          compact_pointers_.push_back(std::make_pair(level, key));
        } else {
          msg = "compaction pointer";
        }
        break;

      case kDeletedFile:
        if (GetLevel(&input, &level) && GetVarint64(&input, &number)) {
          deleted_files_.insert(std::make_pair(level, number));
        } else {
          msg = "deleted file";
        }
        break;

      case kNewFile:
        if (GetLevel(&input, &level) && GetVarint64(&input, &f.number) &&
            GetVarint64(&input, &f.file_size) &&
            GetInternalKey(&input, &f.smallest) &&
            GetInternalKey(&input, &f.largest)) {
          new_files_.push_back(std::make_pair(level, f));
        } else {
          msg = "new-file entry";
        }
        break;

      default:
        msg = "unknown tag";
        break;
    }
  }

  if (msg == nullptr && !input.empty()) {
    msg = "invalid tag";
  }

  Status result;
  if (msg != nullptr) {
    result = Status::Corruption("VersionEdit", msg);
  }
  return result;
}

std::string VersionEdit::DebugString() const {
  std::string r;
  r.append("VersionEdit {");
  if (has_comparator_) {
    r.append("\n  Comparator: ");
    r.append(comparator_);
  }
  if (has_log_number_) {
    r.append("\n  LogNumber: ");
    AppendNumberTo(&r, log_number_);
  }
  if (has_prev_log_number_) {
    r.append("\n  PrevLogNumber: ");
    AppendNumberTo(&r, prev_log_number_);
  }
  if (has_next_file_number_) {
    r.append("\n  NextFile: ");
    AppendNumberTo(&r, next_file_number_);
  }
  if (has_last_sequence_) {
    r.append("\n  LastSeq: ");
    AppendNumberTo(&r, last_sequence_);
  }
  for (size_t i = 0; i < compact_pointers_.size(); i++) {
    r.append("\n  CompactPointer: ");
    AppendNumberTo(&r, compact_pointers_[i].first);
    r.append(" ");
    r.append(compact_pointers_[i].second.DebugString());
  }
  for (const auto& deleted_files_kvp : deleted_files_) {
    r.append("\n  RemoveFile: ");
    AppendNumberTo(&r, deleted_files_kvp.first);
    r.append(" ");
    AppendNumberTo(&r, deleted_files_kvp.second);
  }
  for (size_t i = 0; i < new_files_.size(); i++) {
    const FileMetaData& f = new_files_[i].second;
    r.append("\n  AddFile: ");
    AppendNumberTo(&r, new_files_[i].first);
    r.append(" ");
    AppendNumberTo(&r, f.number);
    r.append(" ");
    AppendNumberTo(&r, f.file_size);
    r.append(" ");
    r.append(f.smallest.DebugString());
    r.append(" .. ");
    r.append(f.largest.DebugString());
  }
  r.append("\n}\n");
  return r;
}

}  // namespace leveldb
