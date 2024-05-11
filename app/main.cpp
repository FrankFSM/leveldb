#include <cassert>
#include <iostream>
#include "include/leveldb/db.h"
#include <string>
void new_iterator(leveldb::DB* pDb);
void get_approximate_sizes(leveldb::DB* db);

void EncodeFixed32(char* dst, uint32_t value);
int main() {
  std::cout << (1<< 21) << ' ';

//  char buffer[4];
//  uint32_t value = 123456789;
//
//  EncodeFixed32(buffer, value);
//
//  for (int i = 0; i < 4; i++) {
//    std::cout << static_cast<int>(buffer[i]) << ' ';
//  }

  leveldb::DB *db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "/Users/ralaphao/tmp/leveldb/t_user", &db);
  assert(status.ok());
  std::cout << "leveldb open success!" << std::endl;
////  std::string value;
////  std::string key = "testkey1";
////  leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
//
  for (int i = 0; i < 1000000; ++i) {
    std::string key = "test-key-" + std::to_string(i);
    std::string value = "test-value-" + std::to_string(i);
    db->Put(leveldb::WriteOptions(), key, value);
    std::cout << "put success, key=" << key << ", value=" << value << std::endl;
  }

//  new_iterator(db);

//  get_approximate_sizes(db);

//  std::string value;
//  if (db->GetProperty("leveldb.stats", &value)) {
//    std::cout << "out: " << value << std::endl;
//  }

  //  if (s.IsNotFound()) {
//    std::cout << "can not found for key:" << key1 << std::endl;
//    db->Put(leveldb::WriteOptions(), key1, "testvalue1");
//  }
//  s = db->Get(leveldb::ReadOptions(), key1, &value);
//  if (s.ok()) {
//    std::cout << "found key:" << key1 << ",value:" << value << std::endl;
//  }
//  s = db->Delete(leveldb::WriteOptions(), key1);
//  if (s.ok()) {
//    std::cout << "delete key success which key:" << key1 << std::endl;
//  }
//  s = db->Get(leveldb::ReadOptions(), key1, &value);
//  if (s.IsNotFound()) {
//    std::cout << "can not found after delete for key:" << key1 << std::endl;
//  }
  delete db;
  return 0;
}



void new_iterator(leveldb::DB* pDb) {
  // 创建迭代器
  leveldb::ReadOptions read_options;
  leveldb::Iterator* it = pDb->NewIterator(read_options);

  // 遍历数据库中的所有键值对
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    std::cout << "Key: " << it->key().ToString() << ", Value: " << it->value().ToString() << std::endl;
  }

  // 检查迭代过程中是否发生错误
  if (!it->status().ok()) {
    std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
  }

  // 释放迭代器资源
  delete it;
}


void get_approximate_sizes(leveldb::DB* db) {

  // 插入一些数据
  db->Put(leveldb::WriteOptions(), "key1", "value1");
  db->Put(leveldb::WriteOptions(), "key2", "value2");
  db->Put(leveldb::WriteOptions(), "key3", "value3");
  db->Put(leveldb::WriteOptions(), "key4", "value4");
  db->Put(leveldb::WriteOptions(), "key5", "value5");

  // 获取键范围[key1, key5]的近似大小
  leveldb::Range ranges[1];
  ranges[0] = leveldb::Range("key1", "key5");
  uint64_t sizes[1];
  db->GetApproximateSizes(ranges, 1, sizes);

  // 输出结果
  std::cout << "Approximate size of range [key1, key5]: " << sizes[0] << std::endl;

  // 关闭数据库
  delete db;
}



void EncodeFixed32(char* dst, uint32_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  buffer[0] = static_cast<uint8_t>(value);
  buffer[1] = static_cast<uint8_t>(value >> 8);
  buffer[2] = static_cast<uint8_t>(value >> 16);
  buffer[3] = static_cast<uint8_t>(value >> 24);
}
