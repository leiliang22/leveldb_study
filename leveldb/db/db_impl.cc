#include "leveldb/db.h"
#include "db/db_impl.h"
#include "leveldb/status.h"
#include <iostream>

namespace leveldb {

DB::~DB() = default;

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
    *dbptr = nullptr;
    *dbptr = new DBImpl();
    Status s;
    std::cout << "OPEN" << std::endl;
    return s;
}

Status DB::Put(const WriteOptions& opt, const Slice& key, const Slice& value) {
    Status s;
    std::cout << "PUT" << std::endl;
    return s;
}

Status DB::Get(const ReadOptions& opt, const Slice& key, std::string* value) {
    Status s;
    std::cout << "GET" << std::endl;
    return s;
}

Status DBImpl::Put(const WriteOptions& options, const Slice& key, const Slice& value) {
    Status s;
    std::cout << "IMPL PUT" << std::endl;
    return s;
}

Status DBImpl::Get(const ReadOptions& options, const Slice& key, std::string* value) {
    Status s;
    std::cout << "IMPL GET" << std::endl;
    return s;
}

}
