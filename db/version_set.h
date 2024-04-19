// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// The representation of a DBImpl consists of a set of Versions.  The
// newest version is called "current".  Older versions may be kept
// around to provide a consistent view to live iterators.
//
// Each Version keeps track of a set of Table files per level.  The
// entire set of versions is maintained in a VersionSet.
//
// Version,VersionSet are thread-compatible, but require external
// synchronization on all accesses.

/**
 * `VersionSet` 类是 LevelDB 中的一个核心组件，它负责管理数据库的所有版本信息，包括 SSTable 文件（也称为表文件）、元数据以及与 Compaction 相关的信息
 * 类的作用：`VersionSet` 类的主要作用是管理不同版本的数据库状态。
 * 在 LevelDB 中，每次写操作都会产生一个新的数据库状态，这些状态由一组 SSTable 文件组成。为了高效地存储和查找这些状态，
 * `VersionSet` 类使用了一种称为 Version 的轻量级数据结构。每个 Version 对象都包含了一个指向 SSTable 文件列表的指针，
 * 以及一个指向下一个 Version 对象的指针。这样，`VersionSet` 类可以通过链接不同的 Version 对象来表示数据库的历史状态，从而实现高效的版本管理。
 */

#ifndef STORAGE_LEVELDB_DB_VERSION_SET_H_
#define STORAGE_LEVELDB_DB_VERSION_SET_H_

#include <map>
#include <set>
#include <vector>

#include "db/dbformat.h"
#include "db/version_edit.h"
#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

namespace log {
class Writer;
}

class Compaction;
class Iterator;
class MemTable;
class TableBuilder;
class TableCache;
class Version;
class VersionSet;
class WritableFile;

// Return the smallest index i such that files[i]->largest >= key.
// Return files.size() if there is no such file.
// REQUIRES: "files" contains a sorted list of non-overlapping files.
int FindFile(const InternalKeyComparator& icmp,
             const std::vector<FileMetaData*>& files, const Slice& key);

// Returns true iff some file in "files" overlaps the user key range
// [*smallest,*largest].
// smallest==nullptr represents a key smaller than all keys in the DB.
// largest==nullptr represents a key largest than all keys in the DB.
// REQUIRES: If disjoint_sorted_files, files[] contains disjoint ranges
//           in sorted order.
bool SomeFileOverlapsRange(const InternalKeyComparator& icmp,
                           bool disjoint_sorted_files,
                           const std::vector<FileMetaData*>& files,
                           const Slice* smallest_user_key,
                           const Slice* largest_user_key);

/**
 * `Version`类是LevelDB中的一个关键组件，用于表示数据库的一个版本。
 * 它包含了一个特定时间点的数据库状态，包括所有级别的SSTable文件。
 * `Version`对象通常与`VersionSet`一起使用，后者负责管理数据库的所有版本。

1. `GetStats`结构：


    3. `Get()`：

    4. `UpdateStats()`：

    5. `RecordReadSample()`：

    6. `Ref()`和`Unref()`：

    7. `GetOverlappingInputs()`：

    8. `OverlapInLevel()`：

    9. `PickLevelForMemTableOutput()`：

    10. `NumFiles()`：

    11. `DebugString()`：

    此外，`Version`类还包含一些私有成员函数和数据成员。这些成员函数主要用于内部操作，如创建级联迭代器、遍历重叠文件等。数据成员包括版本集合指针、前一个和后一个版本指针、引用计数、每个级别的文件列表等。

    总之，`Version`类用于表示和管理数据库的一个版本。它提供了一组方法来访问和操作版本数据，例如添加迭代器、获取键值、更新统计信息等。`Version`对象通常与`VersionSet`一起使用，后者负责管理数据库的所有版本。
 */
class Version {
 public:
  // 用于存储`Version::Get()`函数的统计信息，包括需要压缩的文件和其所在的级别。
  struct GetStats {
    FileMetaData* seek_file;
    int seek_file_level;
  };

  // 为当前版本创建一个迭代器。这个函数会为每个级别的SSTable文件创建一个迭代器，并将这些迭代器组合成一个多级迭代器，以便遍历当前版本的所有键值对。
  // Append to *iters a sequence of iterators that will
  // yield the contents of this Version when merged together.
  // REQUIRES: This version has been saved (see VersionSet::SaveTo)
  void AddIterators(const ReadOptions&, std::vector<Iterator*>* iters);

  // 获取指定内部键的值。这个函数会在当前版本的所有SSTable文件中查找指定的内部键，并返回相应的值。
  // Lookup the value for key.  If found, store it in *val and
  // return OK.  Else return a non-OK status.  Fills *stats.
  // REQUIRES: lock is not held
  Status Get(const ReadOptions&, const LookupKey& key, std::string* val,
             GetStats* stats);

  // 将给定的统计信息合并到当前状态。返回`true`如果可能需要触发新的压缩，否则返回`false`。
  // Adds "stats" into the current state.  Returns true if a new
  // compaction may need to be triggered, false otherwise.
  // REQUIRES: lock is held
  bool UpdateStats(const GetStats& stats);

  // 记录在指定内部键处读取的字节样本。每隔`config::kReadBytesPeriod`字节采样一次。如果可能需要触发新的压缩，则返回`true`。
  // Record a sample of bytes read at the specified internal key.
  // Samples are taken approximately once every config::kReadBytesPeriod
  // bytes.  Returns true if a new compaction may need to be triggered.
  // REQUIRES: lock is held
  bool RecordReadSample(Slice key);

  // 引用计数管理。这些方法用于确保在活动迭代器下不会删除版本。
  // Reference count management (so Versions do not disappear out from
  // under live iterators)
  void Ref();
  void Unref();

  // 获取与指定键范围重叠的输入文件。
  void GetOverlappingInputs(
      int level,
      const InternalKey* begin,  // nullptr means before all keys
      const InternalKey* end,    // nullptr means after all keys
      std::vector<FileMetaData*>* inputs);

  // 如果指定级别的某个文件与给定的键范围重叠，则返回`true`。
  // Returns true iff some file in the specified level overlaps
  // some part of [*smallest_user_key,*largest_user_key].
  // smallest_user_key==nullptr represents a key smaller than all the DB's keys.
  // largest_user_key==nullptr represents a key largest than all the DB's keys.
  bool OverlapInLevel(int level, const Slice* smallest_user_key,
                      const Slice* largest_user_key);

  // 返回应将新的MemTable压缩结果放置在哪个级别。
  // Return the level at which we should place a new memtable compaction
  // result that covers the range [smallest_user_key,largest_user_key].
  int PickLevelForMemTableOutput(const Slice& smallest_user_key,
                                 const Slice& largest_user_key);

  // 返回指定级别的文件数。
  int NumFiles(int level) const { return files_[level].size(); }

  // 返回描述此版本内容的可读字符串。
  // Return a human readable string that describes this version's contents.
  std::string DebugString() const;

  /**
   * 这些是`Version`类的私有数据成员和成员函数。以下是对这些成员的详细解释：

1. `friend class Compaction`和`friend class VersionSet`：声明`Compaction`和`VersionSet`类为友元类，这样它们就可以访问`Version`类的私有成员。

    2. `LevelFileNumIterator`：一个内部类，用于在某个级别的SSTable文件中迭代文件号。它通常用于创建级联迭代器。

    3. 构造函数和析构函数：

    - `explicit Version(VersionSet* vset)`：构造函数接受一个指向`VersionSet`对象的指针，用于初始化`vset_`成员。它还初始化其他成员，如`next_`、`prev_`、`refs_`等。
    - `~Version()`：析构函数。在版本不再被引用时调用，释放资源。

          4. `NewConcatenatingIterator()`：

          5. `ForEachOverlapping()`：

          6. `vset_`：

          7. `next_`和`prev_`：

          8. `refs_`：

          9. `files_`：

          10. `file_to_compact_`和`file_to_compact_level_`：

          11. `compaction_score_`和`compaction_level_`：

          这些数据成员和成员函数主要用于内部操作，如创建级联迭代器、遍历重叠文件等。它们与公有方法一起实现了`Version`类的核心功能，即表示和管理数据库的一个版本。
   */
 private:
  friend class Compaction;
  friend class VersionSet;

  //   // 用于在某个级别的SSTable文件中迭代文件号。它通常用于创建级联迭代器。
  class LevelFileNumIterator;

  explicit Version(VersionSet* vset)
      : vset_(vset),
        next_(this),
        prev_(this),
        refs_(0),
        file_to_compact_(nullptr),
        file_to_compact_level_(-1),
        compaction_score_(-1),
        compaction_level_(-1) {}

  Version(const Version&) = delete;
  Version& operator=(const Version&) = delete;

  ~Version();

  // 创建一个级联迭代器。这个迭代器将多个级别的SSTable文件迭代器组合在一起，以便遍历整个数据库。
  Iterator* NewConcatenatingIterator(const ReadOptions&, int level) const;

  // 对于与给定用户键重叠的每个文件，按从最新到最旧的顺序调用指定的函数。如果函数调用返回`false`，则不再进行调用。这个方法用于查找与给定键重叠的文件。
  // Call func(arg, level, f) for every file that overlaps user_key in
  // order from newest to oldest.  If an invocation of func returns
  // false, makes no more calls.
  //
  // REQUIRES: user portion of internal_key == user_key.
  void ForEachOverlapping(Slice user_key, Slice internal_key, void* arg,
                          bool (*func)(void*, int, FileMetaData*));

  // 指向此`Version`对象所属的`VersionSet`对象的指针。
  VersionSet* vset_;  // VersionSet to which this Version belongs
  // 指向链接列表中的下一个和上一个版本的指针。这些指针用于在`VersionSet`中组织版本。
  Version* next_;     // Next version in linked list
  Version* prev_;     // Previous version in linked list
  // 此版本的活动引用计数。当引用计数为0时，可以删除此版本。
  int refs_;          // Number of live refs to this version

  // 每个级别的文件列表。这是一个二维数组，其中每个元素是一个包含`FileMetaData`指针的向量。
  // List of files per level
  std::vector<FileMetaData*> files_[config::kNumLevels];

  // 基于查找统计信息选择要压缩的下一个文件及其所在的级别。
  // Next file to compact based on seek stats.
  FileMetaData* file_to_compact_;
  int file_to_compact_level_;

  // 下一个要压缩的级别及其压缩分数。分数小于1表示不严格需要压缩。这些字段由`Finalize()`函数初始化。
  // Level that should be compacted next and its compaction score.
  // Score < 1 means compaction is not strictly needed.  These fields
  // are initialized by Finalize().
  double compaction_score_;
  int compaction_level_;
};

/**
 * `VersionSet`类是LevelDB中的一个关键组件，用于管理数据库的所有版本。
 * 它包含了一个版本链表，并提供了一组方法来创建新版本、获取当前版本、执行压缩操作等。
 */
class VersionSet {
 public:
  VersionSet(const std::string& dbname, const Options* options,
             TableCache* table_cache, const InternalKeyComparator*);
  VersionSet(const VersionSet&) = delete;
  VersionSet& operator=(const VersionSet&) = delete;

  ~VersionSet();

  // 将给定的版本编辑应用到当前版本，形成新的版本，并保存到持久状态。
  // Apply *edit to the current version to form a new descriptor that
  // is both saved to persistent state and installed as the new
  // current version.  Will release *mu while actually writing to the file.
  // REQUIRES: *mu is held on entry.
  // REQUIRES: no other thread concurrently calls LogAndApply()
  Status LogAndApply(VersionEdit* edit, port::Mutex* mu)
      EXCLUSIVE_LOCKS_REQUIRED(mu);

  // 从 Manifest 文件中恢复版本信息。这个函数会读取 Manifest 文件，并将其中的版本信息加载到内存中。
  // Recover the last saved descriptor from persistent storage.
  Status Recover(bool* save_manifest);

  // 返回当前版本。
  // Return the current version.
  Version* current() const { return current_; }

  // 返回当前Manifest文件的文件号。
  // Return the current manifest file number
  uint64_t ManifestFileNumber() const { return manifest_file_number_; }

  // 分配并返回一个新的文件号。
  // Allocate and return a new file number
  uint64_t NewFileNumber() { return next_file_number_++; }

  // 安排重用指定的文件号，除非已经分配了更新的文件号。
  // Arrange to reuse "file_number" unless a newer file number has
  // already been allocated.
  // REQUIRES: "file_number" was returned by a call to NewFileNumber().
  void ReuseFileNumber(uint64_t file_number) {
    if (next_file_number_ == file_number + 1) {
      next_file_number_ = file_number;
    }
  }

  // 返回指定级别的文件数量。
  // Return the number of Table files at the specified level.
  int NumLevelFiles(int level) const;

  // 获取指定级别的总字节数。这个函数会遍历指定级别的所有 SSTable 文件，并计算它们的总字节数。
  // Return the combined file size of all files at the specified level.
  int64_t NumLevelBytes(int level) const;

  // 返回最后一个序列号。
  // Return the last sequence number.
  uint64_t LastSequence() const { return last_sequence_; }

  // 设置最后一个序列号。
  // Set the last sequence number to s.
  void SetLastSequence(uint64_t s) {
    assert(s >= last_sequence_);
    last_sequence_ = s;
  }

  // 将指定的文件号标记为已使用。
  // Mark the specified file number as used.
  void MarkFileNumberUsed(uint64_t number);

  // 返回当前日志文件的文件号。
  // Return the current log file number.
  uint64_t LogNumber() const { return log_number_; }

  // 返回当前正在压缩的日志文件的文件号，如果没有此类日志文件，则返回0。
  // Return the log file number for the log file that is currently
  // being compacted, or zero if there is no such log file.
  uint64_t PrevLogNumber() const { return prev_log_number_; }

  // 选择一个合适的 Compaction 操作。这个函数会根据当前版本的状态和配置选项来选择一个最佳的 Compaction 操作，以便最大限度地提高数据库的性能。
  // Pick level and inputs for a new compaction.
  // Returns nullptr if there is no compaction to be done.
  // Otherwise returns a pointer to a heap-allocated object that
  // describes the compaction.  Caller should delete the result.
  Compaction* PickCompaction();

  // 返回一个压缩对象，用于压缩指定级别的指定范围。
  // Return a compaction object for compacting the range [begin,end] in
  // the specified level.  Returns nullptr if there is nothing in that
  // level that overlaps the specified range.  Caller should delete
  // the result.
  Compaction* CompactRange(int level, const InternalKey* begin,
                           const InternalKey* end);

  // 计算下一个级别的最大重叠字节数。这个函数会遍历当前级别的所有 SSTable 文件，并计算它们在下一个级别的重叠字节数。
  // Return the maximum overlapping data (in bytes) at next level for any
  // file at a level >= 1.
  int64_t MaxNextLevelOverlappingBytes();

  // 为指定的 Compaction 操作创建一个输入迭代器。这个函数会为 Compaction 操作的输入文件创建一个迭代器，并将这些迭代器组合成一个多级迭代器，以便遍历 Compaction 操作的所有键值对。
  // Create an iterator that reads over the compaction inputs for "*c".
  // The caller should delete the iterator when no longer needed.
  Iterator* MakeInputIterator(Compaction* c);

  // 检查当前版本是否需要执行 Compaction 操作。这个函数会根据每个级别的分数和文件数量来判断是否需要执行 Compaction。
  // Returns true iff some level needs a compaction.
  bool NeedsCompaction() const {
    Version* v = current_;
    return (v->compaction_score_ >= 1) || (v->file_to_compact_ != nullptr);
  }

  // 将所有活动版本中列出的所有文件添加到`live`集合。
  // Add all files listed in any live version to *live.
  // May also mutate some internal state.
  void AddLiveFiles(std::set<uint64_t>* live);

  // 返回数据库中指定键的大致偏移量。
  // Return the approximate offset in the database of the data for
  // "key" as of version "v".
  uint64_t ApproximateOffsetOf(Version* v, const InternalKey& key);


  // Return a human-readable short (single-line) summary of the number
  // of files per level.  Uses *scratch as backing store.
  struct LevelSummaryStorage {
    char buffer[100];
  };
  // 返回每个级别文件数量的可读简短摘要。
  const char* LevelSummary(LevelSummaryStorage* scratch) const;

 private:
  class Builder;

  friend class Compaction;
  friend class Version;

  bool ReuseManifest(const std::string& dscname, const std::string& dscbase);

  // 确定当前版本的 Compaction 结果。这个函数会计算每个级别的分数、文件大小等信息，以便为后续的 Compaction 操作做准备。
  void Finalize(Version* v);

  void GetRange(const std::vector<FileMetaData*>& inputs, InternalKey* smallest,
                InternalKey* largest);

  void GetRange2(const std::vector<FileMetaData*>& inputs1,
                 const std::vector<FileMetaData*>& inputs2,
                 InternalKey* smallest, InternalKey* largest);

  void SetupOtherInputs(Compaction* c);

  // 将当前版本的快照写入 Manifest 文件。这个函数会将当前版本的所有 SSTable 文件和元数据写入到 Manifest 文件中，以便在故障时恢复数据库状态。
  // Save current contents to *log
  Status WriteSnapshot(log::Writer* log);

  void AppendVersion(Version* v);

  // 一个 `Env` 指针，表示操作系统环境。`VersionSet` 类使用 `env_` 成员来执行文件 I/O 操作、获取当前时间等。
  Env* const env_;
  // 一个字符串，表示数据库的名称。`VersionSet` 类使用 `dbname_` 成员来构造 SSTable 文件的路径。
  const std::string dbname_;
  // 一个 `Options` 对象，表示数据库的配置选项。`VersionSet` 类使用 `options_` 成员来获取缓存策略、比较器等设置。
  const Options* const options_;
  TableCache* const table_cache_;
  // 一个 `InternalKeyComparator` 对象，表示内部键的比较器。`VersionSet` 类使用 `icmp_` 成员来比较数据库中的内部键。
  const InternalKeyComparator icmp_;
  // 一个整数，表示下一个 SSTable 文件的文件号。`VersionSet` 类使用 `next_file_number_` 成员来生成新的 SSTable 文件。
  uint64_t next_file_number_;
  // 一个整数，表示当前 Manifest 文件的文件号。`VersionSet` 类使用 `manifest_file_number_` 成员来记录和加载 Manifest 文件。
  uint64_t manifest_file_number_;
  // 一个整数，表示最后一个操作的序列号。`VersionSet` 类使用 `last_sequence_` 成员来生成新的操作序列号，以保证操作的顺序执行。
  uint64_t last_sequence_;
  // 一个整数，表示当前 WAL（写前日志）文件的序号。`VersionSet` 类使用 `log_number_` 成员来记录和加载 WAL 文件。
  uint64_t log_number_;
  // 一个整数，表示前一个 WAL 文件的序号。`VersionSet` 类使用 `prev_log_number_` 成员来记录和加载前一个 WAL 文件。
  uint64_t prev_log_number_;  // 0 or backing store for memtable being compacted

  // 一个 `log::Writer` 指针，表示当前的 Manifest 文件。`VersionSet` 类使用 `descriptor_log_` 成员来记录版本信息，以便在故障时恢复数据库状态。
  // Opened lazily
  WritableFile* descriptor_file_;
  log::Writer* descriptor_log_;
  // 一个 `Version` 对象，表示一个虚拟的版本节点。`VersionSet` 类使用 `dummy_versions_` 成员来链接不同的 Version 对象，形成一个双向链表。
  Version dummy_versions_;  // Head of circular doubly-linked list of versions.
  // 一个 `Version` 指针，表示当前的数据库状态。`VersionSet` 类使用 `current_` 成员来存储和查找键值对。
  Version* current_;        // == dummy_versions_.prev_

  // Per-level key at which the next compaction at that level should start.
  // Either an empty string, or a valid InternalKey.
  std::string compact_pointer_[config::kNumLevels];
};

/**
 * `Compaction`类是LevelDB中的一个关键组件，用于表示和管理数据库的压缩操作。压缩操作主要用于合并多个SSTable文件，并清除过期数据和重复数据，以提高数据库的性能和空间利用率。
 * Compaction`类用于表示和管理数据库的压缩操作。它提供了一组方法来访问和操作压缩数据，
 * 例如获取级别、编辑版本、检查基础级别等。`Compaction`对象通常与`Version`和`VersionSet`类一起使用，后者分别表示数据库的一个版本和一组版本。
 */
// A Compaction encapsulates information about a compaction.
class Compaction {
 public:
  ~Compaction();

  // 返回正在进行压缩的级别。将“level”和“level+1”的输入合并，以生成一组“level+1”文件。
  // Return the level that is being compacted.  Inputs from "level"
  // and "level+1" will be merged to produce a set of "level+1" files.
  int level() const { return level_; }

  // 返回一个指向版本编辑对象的指针，该对象保存了此压缩操作对描述符所做的编辑。
  // Return the object that holds the edits to the descriptor done
  // by this compaction.
  VersionEdit* edit() { return &edit_; }

  // 返回指定输入集（which必须为0或1）的文件数。
  // "which" must be either 0 or 1
  int num_input_files(int which) const { return inputs_[which].size(); }

  // 返回“level()+which”处的第i个输入文件（which必须为0或1）。
  // Return the ith input file at "level()+which" ("which" must be 0 or 1).
  FileMetaData* input(int which, int i) const { return inputs_[which][i]; }

  // 返回此压缩过程中生成的文件的最大大小。
  // Maximum size of files to build during this compaction.
  uint64_t MaxOutputFileSize() const { return max_output_file_size_; }

  // 判断此压缩操作是否为简单移动，即仅通过将单个输入文件移动到下一级别（无需合并或拆分）即可实现。
  // Is this a trivial compaction that can be implemented by just
  // moving a single input file to the next level (no merging or splitting)
  bool IsTrivialMove() const;

  // 将此压缩操作的所有输入作为删除操作添加到`edit`。
  // Add all inputs to this compaction as delete operations to *edit.
  void AddInputDeletions(VersionEdit* edit);

  // 如果我们拥有的信息保证压缩操作正在为“level+1”生成数据，且大于“level+1”的级别中不存在数据，则返回true。
  // Returns true if the information we have available guarantees that
  // the compaction is producing data in "level+1" for which no data exists
  // in levels greater than "level+1".
  bool IsBaseLevelForKey(const Slice& user_key);

  // 如果在处理`internal_key`之前应停止构建当前输出，则返回true。
  // Returns true iff we should stop building the current output
  // before processing "internal_key".
  bool ShouldStopBefore(const Slice& internal_key);

  // 一旦压缩成功，释放压缩操作的输入版本。
  // Release the input version for the compaction, once the compaction
  // is successful.
  void ReleaseInputs();

 private:
  friend class Version;
  friend class VersionSet;

  Compaction(const Options* options, int level);

  // 正在进行压缩的级别。`level_`和`level_+1`的输入将被合并以生成一组`level_+1`的文件。
  int level_;
  // 此压缩过程中生成的文件的最大大小。
  uint64_t max_output_file_size_;
  // 指向输入版本的指针。输入版本是压缩操作的输入，包含了要合并的SSTable文件。
  Version* input_version_;
  // 一个版本编辑对象，保存了此压缩操作对描述符所做的编辑。
  VersionEdit edit_;

  // 每次压缩操作从`level_`和`level_+1`读取输入。这是一个二维数组，其中每个元素是一个包含`FileMetaData`指针的向量。
  // Each compaction reads inputs from "level_" and "level_+1"
  std::vector<FileMetaData*> inputs_[2];  // The two sets of inputs

  // 用于检查重叠祖父文件数量的状态（parent == `level_ + 1`，grandparent == `level_ + 2`）。这是一个包含`FileMetaData`指针的向量。
  // State used to check for number of overlapping grandparent files
  // (parent == level_ + 1, grandparent == level_ + 2)
  std::vector<FileMetaData*> grandparents_;
  // 祖父文件的索引。
  size_t grandparent_index_;  // Index in grandparent_starts_
  // 表示是否已经看到一些输出键。
  bool seen_key_;             // Some output key has been seen
  // 当前输出与祖父文件之间重叠的字节数。
  int64_t overlapped_bytes_;  // Bytes of overlap between current output
                              // and grandparent files

  // State for implementing IsBaseLevelForKey

  // 保存了索引到`input_version_->levels_`的指针。我们的状态是我们位于每个高于此压缩操作所涉及的级别的文件范围之一（即所有L >= `level_ + 2`）。
  // level_ptrs_ holds indices into input_version_->levels_: our state
  // is that we are positioned at one of the file ranges for each
  // higher level than the ones involved in this compaction (i.e. for
  // all L >= level_ + 2).
  size_t level_ptrs_[config::kNumLevels];
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_VERSION_SET_H_
