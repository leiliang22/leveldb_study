#include "leveldb/db.h"
#include <iostream>

namespace leveldb {

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
    *dbptr = nullptr;
    Status s;
    std::cout << "OPEN" << std::endl;
    return s;
}

Status DB::Put(const WriteOptions& opt, const Slice& key, const Slice& value) {
    status s;
    std::cout << "PUT" << std::endl;
    return s;
}

Status DB::Get(const ReadOptions& opt, const Slice& key, const Slice& value) {
    status s;
    std::cout << "GET" << std::endl;
    return s;
}

}
