#ifndef STORAGE_LEVELDB_DB_VERSION_EDIT_H_
#define STORAGE_LEVELDB_DB_VERSION_EDIT_H_

#include <set>
#include <utility>
#include <vector>

namespace leveldb {

struct FileMetaData {
    FileMetaData() : refs(0), allowed_seeks(1 << 30), file_size(0) {}

    int refs;
    int allowed_seeks;  // seeks allowed until compaction
    uint64_t number;
    uint64_t file_size;  // File size in bytes
    InternalKey smallest;
    InternalKey largest; 
};

class VersionEdit {
public:
    VersionEdit() = default;

    void SetLogNumber(uint64_t num) {
        has_log_number_ = true;
        log_number_ = num;
    }

    void SetPrevLogNumber(uint64_t num) {
        has_prev_log_number_ = true;
        prev_log_number_ = num;
    }

private:
    uint64_t log_number_;
    uint64_t prev_log_number_;
    bool has_log_number_;
    bool has_prev_log_number_;
};


}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_VERSION_EDIT_H_
