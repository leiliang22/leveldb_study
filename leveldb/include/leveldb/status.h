#ifndef STORAGE_LEVELDB_INCLUDE_STATUS_H_
#define STORAGE_LEVELDB_INCLUDE_STATUS_H_

#include <algorithm>
#include <string>

namespace leveldb {
class LEVELDB_EXPORT Status {
public:
    Status() noexcept : state_(nullptr) {}
    ~Status() { delete[] state_; }

    bool ok() const { return true; }

private:
    const char* state_;
};
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_STATUS_H_
