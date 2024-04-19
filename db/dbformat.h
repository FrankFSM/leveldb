// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/slice.h"
#include "leveldb/table_builder.h"
#include "util/coding.h"
#include "util/logging.h"

namespace leveldb {

// Grouping of constants.  We may want to make some of these
// parameters set via options.
namespace config {
static const int kNumLevels = 7;

// Level-0 compaction is started when we hit this many files.
static const int kL0_CompactionTrigger = 4;

// Soft limit on number of level-0 files.  We slow down writes at this point.
static const int kL0_SlowdownWritesTrigger = 8;

// Maximum number of level-0 files.  We stop writes at this point.
static const int kL0_StopWritesTrigger = 12;

// Maximum level to which a new compacted memtable is pushed if it
// does not create overlap.  We try to push to level 2 to avoid the
// relatively expensive level 0=>1 compactions and to avoid some
// expensive manifest file operations.  We do not push all the way to
// the largest level since that can generate a lot of wasted disk
// space if the same key space is being repeatedly overwritten.
static const int kMaxMemCompactLevel = 2;

// Approximate gap in bytes between samples of data read during iteration.
static const int kReadBytesPeriod = 1048576;

}  // namespace config

class InternalKey;

// Value types encoded as the last component of internal keys.
// DO NOT CHANGE THESE ENUM VALUES: they are embedded in the on-disk
// data structures.
enum ValueType { kTypeDeletion = 0x0, kTypeValue = 0x1 };
// kValueTypeForSeek defines the ValueType that should be passed when
// constructing a ParsedInternalKey object for seeking to a particular
// sequence number (since we sort sequence numbers in decreasing order
// and the value type is embedded as the low 8 bits in the sequence
// number in internal keys, we need to use the highest-numbered
// ValueType, not the lowest).
static const ValueType kValueTypeForSeek = kTypeValue;

typedef uint64_t SequenceNumber;

// We leave eight bits empty at the bottom so a type and sequence#
// can be packed together into 64-bits.
static const SequenceNumber kMaxSequenceNumber = ((0x1ull << 56) - 1);

struct ParsedInternalKey {
  Slice user_key;
  SequenceNumber sequence;
  ValueType type;

  ParsedInternalKey() {}  // Intentionally left uninitialized (for speed)
  ParsedInternalKey(const Slice& u, const SequenceNumber& seq, ValueType t)
      : user_key(u), sequence(seq), type(t) {}
  std::string DebugString() const;
};

// Return the length of the encoding of "key".
inline size_t InternalKeyEncodingLength(const ParsedInternalKey& key) {
  return key.user_key.size() + 8;
}

// Append the serialization of "key" to *result.
void AppendInternalKey(std::string* result, const ParsedInternalKey& key);

// Attempt to parse an internal key from "internal_key".  On success,
// stores the parsed data in "*result", and returns true.
//
// On error, returns false, leaves "*result" in an undefined state.
bool ParseInternalKey(const Slice& internal_key, ParsedInternalKey* result);

// Returns the user key portion of an internal key.
inline Slice ExtractUserKey(const Slice& internal_key) {
  assert(internal_key.size() >= 8);
  return Slice(internal_key.data(), internal_key.size() - 8);
}

// A comparator for internal keys that uses a specified comparator for
// the user key portion and breaks ties by decreasing sequence number.
class InternalKeyComparator : public Comparator {
 private:
  const Comparator* user_comparator_;

 public:
  explicit InternalKeyComparator(const Comparator* c) : user_comparator_(c) {}
  const char* Name() const override;
  int Compare(const Slice& a, const Slice& b) const override;
  void FindShortestSeparator(std::string* start,
                             const Slice& limit) const override;
  void FindShortSuccessor(std::string* key) const override;

  const Comparator* user_comparator() const { return user_comparator_; }

  int Compare(const InternalKey& a, const InternalKey& b) const;
};

// Filter policy wrapper that converts from internal keys to user keys
class InternalFilterPolicy : public FilterPolicy {
 private:
  const FilterPolicy* const user_policy_;

 public:
  explicit InternalFilterPolicy(const FilterPolicy* p) : user_policy_(p) {}
  const char* Name() const override;
  void CreateFilter(const Slice* keys, int n, std::string* dst) const override;
  bool KeyMayMatch(const Slice& key, const Slice& filter) const override;
};


/**
 * `InternalKey` 是 LevelDB 中的一个关键数据结构，用于表示内部键。内部键是由用户键和元数据（如序列号、值类型）组成的键，用于在数据库内部进行键值对的组织和管理。`InternalKey` 结构体的定义位于 `db/dbformat.h` 文件中。

以下是 `InternalKey` 结构体的详细说明：

    1. 结构体的作用：`InternalKey` 结构体的主要作用是存储和管理内部键，以便在数据库内部进行键值对的查找、插入、删除等操作。通过使用 `InternalKey` 结构体，LevelDB 可以有效地组织和管理键值对，从而提高数据库的性能和可靠性。此外，`InternalKey` 结构体还提供了一组辅助函数，用于比较、解析和序列化内部键。

    2. 结构体的成员：

    - `rep_`：一个字符串，表示内部键的二进制表示。`InternalKey` 结构体使用 `rep_` 成员来存储内部键的二进制数据。

    3. 结构体的成员函数：

    - `DecodeFrom`：从一个字符串中解码内部键。这个函数接收一个字符串参数，并将其解码为内部键。

    - `Encode`：将内部键编码为一个字符串。这个函数返回一个字符串，表示内部键的二进制表示。

    - `Clear`：清除内部键的数据。这个函数会将 `rep_` 成员设置为空字符串。

    - `user_key`：获取用户键。这个函数返回一个 `Slice` 对象，表示内部键中的用户键部分。

    - `sequence`：获取序列号。这个函数返回一个整数，表示内部键中的序列号部分。

    - `type`：获取值类型。这个函数返回一个枚举值，表示内部键中的值类型部分（如 `kTypeValue`、`kTypeDeletion` 等）。

    - `SetFrom`：从一个用户键和元数据设置内部键。这个函数接收一个用户键、序列号和值类型作为参数，并将它们组合成一个内部键。

    - `SetMaxPossibleForUserKey`：将内部键设置为用户键的最大可能值。这个函数接收一个用户键作为参数，并将内部键设置为该用户键的最大可能值（即序列号和值类型均为最大值）。

    - `SetMinPossibleForUserKey`：将内部键设置为用户键的最小可能值。这个函数接收一个用户键作为参数，并将内部键设置为该用户键的最小可能值（即序列号和值类型均为最小值）。

          通过这些成员变量和成员函数，`InternalKey` 结构体实现了对内部键的存储和管理。在实际应用中，`InternalKey` 结构体在 LevelDB 的各种操作中起到了关键作用，如查询、插入、删除等。



结构体可以带来以下好处：

1. 更清晰的语义：`InternalKey` 结构体明确表示了内部键的概念，有助于理解 LevelDB 的代码和数据结构。直接使用 `std::string` 可能会使代码的意图变得模糊，降低代码的可读性。

2. 高效的操作：`InternalKey` 结构体提供了一组针对内部键的辅助函数，如 `DecodeFrom`、`Encode`、`user_key`、`sequence` 等。这些函数允许我们高效地对内部键进行解析、比较和序列化，而无需手动处理二进制数据。如果直接使用 `std::string`，我们可能需要为这些操作编写额外的代码，增加了实现的复杂性。

3. 更好的封装：`InternalKey` 结构体将内部键的表示和操作封装在一个单独的类中，有助于保持代码的整洁和模块化。这样，当需要修改内部键的实现时，我们只需修改 `InternalKey` 类，而无需修改其他部分的代码。如果直接使用 `std::string`，内部键的操作可能会分散在整个代码库中，导致代码难以维护和扩展。

4. 更好的可扩展性：`InternalKey` 结构体为未来的功能扩展提供了基础。例如，如果我们需要为内部键添加新的元数据（如时间戳、版本号等），只需修改 `InternalKey` 类即可。如果直接使用 `std::string`，可能需要对整个代码库进行较大的修改，以支持新的功能。

综上所述，虽然直接使用 `std::string` 可以表示内部键，但引入 `InternalKey` 结构体可以带来更清晰的语义、高效的操作、更好的封装和可扩展性。这些优点有助于提高 LevelDB 的性能、可靠性和易用性。
 */
// Modules in this directory should keep internal keys wrapped inside
// the following class instead of plain strings so that we do not
// incorrectly use string comparisons instead of an InternalKeyComparator.
class InternalKey {
 private:
  std::string rep_;

 public:
  InternalKey() {}  // Leave rep_ as empty to indicate it is invalid
  InternalKey(const Slice& user_key, SequenceNumber s, ValueType t) {
    AppendInternalKey(&rep_, ParsedInternalKey(user_key, s, t));
  }

  bool DecodeFrom(const Slice& s) {
    rep_.assign(s.data(), s.size());
    return !rep_.empty();
  }

  Slice Encode() const {
    assert(!rep_.empty());
    return rep_;
  }

  Slice user_key() const { return ExtractUserKey(rep_); }

  void SetFrom(const ParsedInternalKey& p) {
    rep_.clear();
    AppendInternalKey(&rep_, p);
  }

  void Clear() { rep_.clear(); }

  std::string DebugString() const;
};

inline int InternalKeyComparator::Compare(const InternalKey& a,
                                          const InternalKey& b) const {
  return Compare(a.Encode(), b.Encode());
}

inline bool ParseInternalKey(const Slice& internal_key,
                             ParsedInternalKey* result) {
  const size_t n = internal_key.size();
  if (n < 8) return false;
  uint64_t num = DecodeFixed64(internal_key.data() + n - 8);
  uint8_t c = num & 0xff;
  result->sequence = num >> 8;
  result->type = static_cast<ValueType>(c);
  result->user_key = Slice(internal_key.data(), n - 8);
  return (c <= static_cast<uint8_t>(kTypeValue));
}

// A helper class useful for DBImpl::Get()
class LookupKey {
 public:
  // Initialize *this for looking up user_key at a snapshot with
  // the specified sequence number.
  LookupKey(const Slice& user_key, SequenceNumber sequence);

  LookupKey(const LookupKey&) = delete;
  LookupKey& operator=(const LookupKey&) = delete;

  ~LookupKey();

  // Return a key suitable for lookup in a MemTable.
  Slice memtable_key() const { return Slice(start_, end_ - start_); }

  // Return an internal key (suitable for passing to an internal iterator)
  Slice internal_key() const { return Slice(kstart_, end_ - kstart_); }

  // Return the user key
  Slice user_key() const { return Slice(kstart_, end_ - kstart_ - 8); }

 private:
  // We construct a char array of the form:
  //    klength  varint32               <-- start_
  //    userkey  char[klength]          <-- kstart_
  //    tag      uint64
  //                                    <-- end_
  // The array is a suitable MemTable key.
  // The suffix starting with "userkey" can be used as an InternalKey.
  const char* start_;
  const char* kstart_;
  const char* end_;
  char space_[200];  // Avoid allocation for short keys
};

inline LookupKey::~LookupKey() {
  if (start_ != space_) delete[] start_;
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DBFORMAT_H_
