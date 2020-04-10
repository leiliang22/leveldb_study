#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <string>

#include "leveldb/db.h"

namespace leveldb {

class DBImpl : public DB {
public:
    DBImpl() = default;

    DBImpl(DBImpl&) = delete;
    DBImpl& operator=(DBImpl&) = delete;

    Status Put(const WriteOptions& options, const Slice& key, const Slice& value) override;
    Status Get(const ReadOptions& options, const Slice& key, std::string* value) override;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_
