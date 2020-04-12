#ifndef STORAGE_LEVELDB_INCLUDE_SLICE_H_
#define STORAGE_LEVELDB_INCLUDE_SLICE_H_

#include <string>
#include "leveldb/export.h"

namespace leveldb {

class LEVELDB_EXPORT Slice {
public:
    Slice() : data_(""), size_(0) {}
    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

private:
    const char* data_;
    size_t size_;
};
    
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_SLICE_H_
