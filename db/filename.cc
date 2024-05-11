// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/filename.h"

#include <cassert>
#include <cstdio>

#include "db/dbformat.h"
#include "leveldb/env.h"
#include "util/logging.h"

namespace leveldb {

// A utility routine: write "data" to the named file and Sync() it.
Status WriteStringToFileSync(Env* env, const Slice& data,
                             const std::string& fname);

static std::string MakeFileName(const std::string& dbname, uint64_t number,
                                const char* suffix) {
  char buf[100];
  std::snprintf(buf, sizeof(buf), "/%06llu.%s",
                static_cast<unsigned long long>(number), suffix);
  return dbname + buf;
}

std::string LogFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "log");
}

std::string TableFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "ldb");
}

std::string SSTTableFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "sst");
}

/**
 * 生成MANIFEST文件名
 * @param dbname 数据库名
 * @param number 版本号
 */
std::string DescriptorFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  char buf[100];
  std::snprintf(buf, sizeof(buf), "/MANIFEST-%06llu",
                static_cast<unsigned long long>(number));
  return dbname + buf;
}

/**
 * CURRENT文件是一个非常重要的元数据文件，它的作用是记录当前活跃的数据库版本。具体来说，/CURRENT文件包含对当前活跃的数据库MANIFEST文件的引用
 *
 * 使用MANIFEST文件来保存数据库的元数据和状态。随着数据库的更新（例如插入、删除操作）
 * ，LevelDB会创建、合并或删除SSTable文件。在这个过程中，LevelDB会将这些更改记录在MANIFEST文件中。
 * 为了确保数据库在崩溃或重新启动后能够恢复到一致的状态，LevelDB需要知道在任何给定时刻哪个MANIFEST文件是活跃的
 */
std::string CurrentFileName(const std::string& dbname) {
  return dbname + "/CURRENT";
}

std::string LockFileName(const std::string& dbname) { return dbname + "/LOCK"; }

std::string TempFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "dbtmp");
}

std::string InfoLogFileName(const std::string& dbname) {
  return dbname + "/LOG";
}

// Return the name of the old info log file for "dbname".
std::string OldInfoLogFileName(const std::string& dbname) {
  return dbname + "/LOG.old";
}


// Owned filenames have the form:
//    dbname/CURRENT  当前活跃的数据库版本
//    dbname/LOCK   用于数据库的文件锁
//    dbname/LOG   用于记录数据库的日志
//    dbname/LOG.old  用于记录数据库的旧日志
//    dbname/MANIFEST-[0-9]+  用于记录数据库的元数据和状态
//    dbname/[0-9]+.(log|sst|ldb) 用于存储数据库的数据
bool ParseFileName(const std::string& filename, uint64_t* number,
                   FileType* type) {
  Slice rest(filename);
  if (rest == "CURRENT") {
    *number = 0;
    *type = kCurrentFile;
  } else if (rest == "LOCK") {
    *number = 0;
    *type = kDBLockFile;
  } else if (rest == "LOG" || rest == "LOG.old") {
    *number = 0;
    *type = kInfoLogFile;
  } else if (rest.starts_with("MANIFEST-")) {
    rest.remove_prefix(strlen("MANIFEST-"));
    uint64_t num;
    if (!ConsumeDecimalNumber(&rest, &num)) {
      return false;
    }
    if (!rest.empty()) {
      return false;
    }
    *type = kDescriptorFile;
    *number = num;
  } else {
    // Avoid strtoull() to keep filename format independent of the
    // current locale
    uint64_t num;
    if (!ConsumeDecimalNumber(&rest, &num)) {
      return false;
    }
    Slice suffix = rest;
    if (suffix == Slice(".log")) {
      *type = kLogFile;
    } else if (suffix == Slice(".sst") || suffix == Slice(".ldb")) {
      *type = kTableFile;
    } else if (suffix == Slice(".dbtmp")) {
      *type = kTempFile;
    } else {
      return false;
    }
    *number = num;
  }
  return true;
}

/**
 * 设置当前活跃的数据库版本
 * 首先生成临时文件，将MANIFEST文件名写入临时文件，然后将临时文件重命名为CURRENT文件
 * 这样做的作用：
 *  保证CURRENT文件的原子性和一致性
 *  创建文件和写入内容是在临时文件上进行的，只有在写入成功后才会将临时文件重命名为CURRENT文件
 *  重命名操作是原子的，这样可以保证CURRENT文件的一致性
 */
Status SetCurrentFile(Env* env, const std::string& dbname,
                      uint64_t descriptor_number) {
  // Remove leading "dbname/" and add newline to manifest file name
  // 生成MANIFEST文件名
  std::string manifest = DescriptorFileName(dbname, descriptor_number);
  Slice contents = manifest;
  // 断言contents以dbname + "/"开头
  assert(contents.starts_with(dbname + "/"));
  // 移除dbname + "/"前缀
  contents.remove_prefix(dbname.size() + 1);
  // 生成临时文件名: 000001.dbtmp
  std::string tmp = TempFileName(dbname, descriptor_number);
  // 将MANIFEST文件名写入临时文件
  Status s = WriteStringToFileSync(env, contents.ToString() + "\n", tmp);
  if (s.ok()) {
    // 将临时文件重命名为CURRENT文件
    s = env->RenameFile(tmp, CurrentFileName(dbname));
  }
  if (!s.ok()) {
    env->RemoveFile(tmp);
  }
  return s;
}

}  // namespace leveldb
