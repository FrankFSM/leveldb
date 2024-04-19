// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <atomic>
#include <deque>
#include <set>
#include <string>

#include "db/dbformat.h"
#include "db/log_writer.h"
#include "db/snapshot.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

class MemTable;
class TableCache;
class Version;
class VersionEdit;
class VersionSet;



/**
 * `db_impl.h`文件定义了`DBImpl`类，它是LevelDB的核心实现类。`DBImpl`类继承自`DB`接口，并实现了各种数据库操作。以下是`DBImpl`类及其成员的详细说明：

1. `DBImpl`类：此类实现了LevelDB的核心功能，包括打开数据库、读写数据、遍历数据、管理快照等。它包含以下成员：



    - `mutex_`：

    - `bg_cv_`：一个条件变量，用于在后台线程中执行Compaction操作。`DBImpl`类使用`bg_cv_`成员来通知后台线程开始或结束Compaction操作。

    - `shutting_down_`：

    - `bg_compaction_scheduled_`：

    - `versions_`：

    - `log_`：

    - `mem_`：

    - `imm_`：

    - `has_imm_`：

    - `log_number_`：一个整数，表示当前WAL文件的序号。`DBImpl`类使用`log_number_`成员来生成新的WAL文件。

    - `prev_log_number_`：一个整数，表示前一个WAL文件的序号。`DBImpl`类使用`prev_log_number_`成员来记录前一个WAL文件，以便在故障时恢复数据库状态。

    - `last_sequence_`：一个原子整数，表示最后一个操作的序列号。`DBImpl`类使用`last_sequence_`成员来生成新的操作序列号，以保证操作的顺序执行。

    - `snapshots_`：

    - `bg_error_`：

    - `stats_`：

    - `compaction_pool_`：

    2. `DBImpl`类的成员函数：此类实现了`DB`接口的各种成员函数，包括：

    - `Put`：插入或更新一个键值对。
    - `Delete`：删除一个键值对。
    - `Write`：将一系列修改操作应用到数据库。
    - `Get`：获取一个键的值。
    - `NewIterator`：创建一个迭代器，用于遍历数据库中的键值对。
    - `GetSnapshot`：获取一个快照。
    - `ReleaseSnapshot`：释放一个快照。
    - `GetProperty`：获取数据库的属性。
    - `GetApproximateSizes`：获取数据库中一系列键范围的近似大小。
    - `CompactRange`：手动触发一次Compaction操作。

    除了这些公共成员函数外，`DBImpl`类还包含许多私有成员函数，这些函数在内部实现中起到辅助作用。例如：

    - `CompactMemTable`：
    - `MaybeScheduleCompaction`：
    - `BackgroundCall`：
    - `BackgroundCompaction`：
    - `CleanupCompaction`：

          通过这些成员变量和成员函数，`DBImpl`类实现了LevelDB的核心功能，并提供了高性能、可靠性和易用性。



          抱歉刚刚的回答未包含所有成员。以下是补充的`DBImpl`类成员的详细说明：

1. `DBImpl`类的构造函数和析构函数：

- `DBImpl(const Options& options, const std::string& dbname)`：构造函数，用于初始化`DBImpl`类的实例。它接收一个`Options`对象和一个数据库名称作为参数，并设置相应的成员变量。

- `~DBImpl() override`：析构函数，用于释放`DBImpl`类实例占用的资源。在析构函数中，它会关闭数据库、删除不需要的文件、释放锁等。

2. `DBImpl`类的私有成员函数：

- `NewInternalIterator`：创建一个内部迭代器，用于遍历数据库中的键值对。与`NewIterator`成员函数不同，此函数返回的迭代器使用内部键（参见`dbformat.h`）。

- `NewDB`：创建一个新的数据库。

- `Recover`：从持久性存储中恢复描述符。可能需要执行大量工作来恢复最近记录的更新。将对描述符进行的任何更改添加到`*edit`中。

- `MaybeIgnoreError`：如果`paranoid_checks`选项为`false`，则忽略错误。

- `RemoveObsoleteFiles`：删除不需要的文件和过时的内存条目。

- `CompactMemTable`：将内存中的写缓冲区压缩到磁盘。成功切换到新的日志文件/ MemTable 并写入新描述符。错误记录在`bg_error_`中。

- `RecoverLogFile`：恢复指定的日志文件。设置`*save_manifest`以指示是否需要写入新的描述符文件。

- `WriteLevel0Table`：将内存表刷新到Level-0的SSTable文件。

- `MakeRoomForWrite`：确保有足够的空间进行写操作。如果`force`参数为`true`，则在有空间的情况下也进行压缩。

- `BuildBatchGroup`：将一组写入操作组合成一个批量操作。

- `RecordBackgroundError`：记录后台线程中发生的错误。

- `MaybeScheduleCompaction`：在需要时安排一个后台压缩操作。

- `BGWork`：后台线程的静态入口函数，用于执行压缩操作。

- `BackgroundCall`：后台线程的入口函数，用于执行压缩操作。

- `BackgroundCompaction`：在后台线程中执行压缩操作。

- `CleanupCompaction`：在压缩操作完成后清理相关资源。

- `DoCompactionWork`：执行压缩操作。

- `OpenCompactionOutputFile`：打开用于压缩操作的输出文件。

- `FinishCompactionOutputFile`：完成压缩操作的输出文件。

- `InstallCompactionResults`：安装压缩结果。

3. `DBImpl`类的测试成员函数：

- `TEST_CompactRange`：压缩指定级别中与给定键范围重叠的任何文件。

- `TEST_CompactMemTable`：强制将当前的MemTable内容压缩。

- `TEST_NewInternalIterator`：返回一个遍历数据库当前状态的内部迭代器。这些迭代器的键是内部键（参见`format.h`）。

- `TEST_MaxNextLevelOverlappingBytes`：返回任何大于等于1级的文件在下一级别中的最大重叠数据（以字节为单位）。

- `RecordReadSample`：在指定的内部键处记录读取的字节样本。样本大约每隔`config::kReadBytesPeriod`字节采集一次。

通过这些成员变量和成员函数，`

 DBImpl` 类实现了 LevelDB 的核心功能，并提供了高性能、可靠性和易用性。以下是一些其他成员函数的补充说明：

4. `DBImpl` 类的辅助函数：

   - `user_comparator()`：返回用户提供的比较器，该比较器用于比较数据库中的键。这是一个内联函数，用于方便地访问内部比较器的用户比较器。

   - `SanitizeOptions()`：对 db 选项进行清理。如果结果的 `info_log` 不等于 `src.info_log`，调用者应删除它。这个函数在构造 `DBImpl` 对象时被调用，以确保选项的有效性。

         这些补充的成员变量和成员函数与之前提到的成员变量和成员函数一起，共同构成了 `DBImpl` 类的完整实现。`DBImpl` 类实现了 LevelDB 的核心功能，并提供了高性能、可靠性和易用性。
 */

class DBImpl : public DB {
 public:
  DBImpl(const Options& options, const std::string& dbname);

  DBImpl(const DBImpl&) = delete;
  DBImpl& operator=(const DBImpl&) = delete;

  ~DBImpl() override;

  // Implementations of the DB interface
  Status Put(const WriteOptions&, const Slice& key,
             const Slice& value) override;
  Status Delete(const WriteOptions&, const Slice& key) override;
  Status Write(const WriteOptions& options, WriteBatch* updates) override;
  Status Get(const ReadOptions& options, const Slice& key,
             std::string* value) override;
  Iterator* NewIterator(const ReadOptions&) override;
  const Snapshot* GetSnapshot() override;
  void ReleaseSnapshot(const Snapshot* snapshot) override;
  bool GetProperty(const Slice& property, std::string* value) override;
  void GetApproximateSizes(const Range* range, int n, uint64_t* sizes) override;
  void CompactRange(const Slice* begin, const Slice* end) override;

  // Extra methods (for testing) that are not in the public DB interface

  // Compact any files in the named level that overlap [*begin,*end]
  void TEST_CompactRange(int level, const Slice* begin, const Slice* end);

  // Force current memtable contents to be compacted.
  Status TEST_CompactMemTable();

  // Return an internal iterator over the current state of the database.
  // The keys of this iterator are internal keys (see format.h).
  // The returned iterator should be deleted when no longer needed.
  Iterator* TEST_NewInternalIterator();

  // Return the maximum overlapping data (in bytes) at next level for any
  // file at a level >= 1.
  int64_t TEST_MaxNextLevelOverlappingBytes();

  // Record a sample of bytes read at the specified internal key.
  // Samples are taken approximately once every config::kReadBytesPeriod
  // bytes.
  void RecordReadSample(Slice key);

 private:
  friend class DB;
  struct CompactionState;
  struct Writer;

  // Information for a manual compaction
  struct ManualCompaction {
    int level;
    bool done;
    const InternalKey* begin;  // null means beginning of key range
    const InternalKey* end;    // null means end of key range
    InternalKey tmp_storage;   // Used to keep track of compaction progress
  };

  // Per level compaction stats.  stats_[level] stores the stats for
  // compactions that produced data for the specified "level".
  struct CompactionStats {
    CompactionStats() : micros(0), bytes_read(0), bytes_written(0) {}

    void Add(const CompactionStats& c) {
      this->micros += c.micros;
      this->bytes_read += c.bytes_read;
      this->bytes_written += c.bytes_written;
    }

    int64_t micros;
    int64_t bytes_read;
    int64_t bytes_written;
  };

  Iterator* NewInternalIterator(const ReadOptions&,
                                SequenceNumber* latest_snapshot,
                                uint32_t* seed);

  Status NewDB();

  // 从持久存储中恢复描述符。可能会做大量的工作来恢复最近记录的更新。
  // 对描述符所做的任何更改都会添加到 *edit。
  // Recover the descriptor from persistent storage.  May do a significant
  // amount of work to recover recently logged updates.  Any changes to
  // be made to the descriptor are added to *edit.
  Status Recover(VersionEdit* edit, bool* save_manifest)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void MaybeIgnoreError(Status* s) const;

  // Delete any unneeded files and stale in-memory entries.
  void RemoveObsoleteFiles() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // 将当前的MemTable转换为不可变的只读MemTable，并创建一个新的可写MemTable。
  // Compact the in-memory write buffer to disk.  Switches to a new
  // log-file/memtable and writes a new descriptor iff successful.
  // Errors are recorded in bg_error_.
  void CompactMemTable() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  Status RecoverLogFile(uint64_t log_number, bool last_log, bool* save_manifest,
                        VersionEdit* edit, SequenceNumber* max_sequence)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  Status WriteLevel0Table(MemTable* mem, VersionEdit* edit, Version* base)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  Status MakeRoomForWrite(bool force /* compact even if there is room? */)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  WriteBatch* BuildBatchGroup(Writer** last_writer)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void RecordBackgroundError(const Status& s);

  // 在需要时安排一个后台Compaction操作。
  void MaybeScheduleCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  static void BGWork(void* db);

  // 后台线程的入口函数，用于执行Compaction操作。
  void BackgroundCall();
  // 在后台线程中执行Compaction操作。
  void BackgroundCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  // 在Compaction操作完成后清理相关资源。
  void CleanupCompaction(CompactionState* compact)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  Status DoCompactionWork(CompactionState* compact)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  Status OpenCompactionOutputFile(CompactionState* compact);
  Status FinishCompactionOutputFile(CompactionState* compact, Iterator* input);
  Status InstallCompactionResults(CompactionState* compact)
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  const Comparator* user_comparator() const {
    return internal_comparator_.user_comparator();
  }

  // 一个`Env`指针，表示数据库的环境。它包含了文件系统、时钟等操作系统相关的功能。`DBImpl`类使用`env_`成员来执行文件I/O操作、获取当前时间等。
  // Constant after construction
  Env* const env_;
  const InternalKeyComparator internal_comparator_;
  const InternalFilterPolicy internal_filter_policy_;

  // 一个`Options`对象，表示数据库的配置选项。`DBImpl`类使用`options_`成员来控制数据库行为，如比较器、缓存策略、压缩算法等。
  const Options options_;  // options_.comparator == &internal_comparator_
  const bool owns_info_log_;
  const bool owns_cache_;
  const std::string dbname_;

  // table_cache_ provides its own synchronization
  TableCache* const table_cache_;

  // Lock over the persistent DB state.  Non-null iff successfully acquired.
  FileLock* db_lock_;

  // 一个互斥锁，用于同步多线程访问数据库。`DBImpl`类使用`mutex_`成员来保护内部状态，如版本管理、日志管理等。
  // State below is protected by mutex_
  port::Mutex mutex_;
  // 一个原子布尔值，表示数据库是否正在关闭。`DBImpl`类使用`shutting_down_`成员来控制后台线程的生命周期。
  std::atomic<bool> shutting_down_;
  port::CondVar background_work_finished_signal_ GUARDED_BY(mutex_);
  // 一个`MemTable`指针，表示当前的可写MemTable。`DBImpl`类使用`mem_`成员来存储最近的写操作，以便快速查找。
  MemTable* mem_;
  // 一个`MemTable`指针，表示当前的只读MemTable。`DBImpl`类使用`imm_`成员来存储已满的MemTable，等待Compaction操作。
  MemTable* imm_ GUARDED_BY(mutex_);  // Memtable being compacted
  // 一个原子布尔值，表示是否存在只读MemTable。`DBImpl`类使用`has_imm_`成员来避免同时执行多个MemTable Compaction操作。
  std::atomic<bool> has_imm_;         // So bg thread can detect non-null imm_
  WritableFile* logfile_;
  uint64_t logfile_number_ GUARDED_BY(mutex_);

  // 一个`WritableFile`指针，表示当前的WAL（写前日志）文件。`DBImpl`类使用`log_`成员来记录写操作，以便在故障时恢复数据库状态。
  log::Writer* log_;
  uint32_t seed_ GUARDED_BY(mutex_);  // For sampling.

  // Queue of writers.
  std::deque<Writer*> writers_ GUARDED_BY(mutex_);
  WriteBatch* tmp_batch_ GUARDED_BY(mutex_);

  // 一个`SnapshotList`对象，表示数据库的快照列表。`DBImpl`类使用`snapshots_`成员来管理快照，如创建快照、释放快照等。
  SnapshotList snapshots_ GUARDED_BY(mutex_);

  // Set of table files to protect from deletion because they are
  // part of ongoing compactions.
  std::set<uint64_t> pending_outputs_ GUARDED_BY(mutex_);

  // Has a background compaction been scheduled or is running?
  bool background_compaction_scheduled_ GUARDED_BY(mutex_);

  ManualCompaction* manual_compaction_ GUARDED_BY(mutex_);

  // 一个`VersionSet`指针，表示数据库的版本管理。`DBImpl`类使用`versions_`成员来管理SSTable文件、查找键值对等。
  VersionSet* const versions_ GUARDED_BY(mutex_);

  // 一个`Status`对象，表示后台线程中发生的错误。
  // `DBImpl`类使用`bg_error_`成员来记录后台线程（如Compaction线程）中发生的错误，以便在前台线程中检查和处理这些错误。
  // Have we encountered a background error in paranoid mode?
  Status bg_error_ GUARDED_BY(mutex_);

  // 一个`Stats`对象，表示数据库的统计信息。`DBImpl`类使用`stats_`成员来收集数据库的运行时信息，如Compaction操作的数量、读写延迟等。
  CompactionStats stats_[config::kNumLevels] GUARDED_BY(mutex_);
};

// Sanitize db options.  The caller should delete result.info_log if
// it is not equal to src.info_log.
Options SanitizeOptions(const std::string& db,
                        const InternalKeyComparator* icmp,
                        const InternalFilterPolicy* ipolicy,
                        const Options& src);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_
