#ifndef STORAGE_LEVELDB_DB_FILENAME_H_
#define STORAGE_LEVELDB_DB_FILENAME_H_

#include <string>

namespace leveldb {

std::string LogFileName(const std::string& dbname, uint64_t number);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_FILENAME_H_