#ifndef STORAGE_LEVELDB_DB_VERSION_SET_H_
#define STORAGE_LEVELDB_DB_VERSION_SET_H_

namespace leveldb {

class VersionSet {
public:
    VersionSet();

    VersionSet(const VersionSet&) = delete;
    VersionSet& operator=(const VersionSet&) = delete;

    uint64_t NewFileNumber() { return next_file_number_++; }

private:
    uint64_t next_file_number_;
};
}  // namespace leveldb
#endif  // STORAGE_LEVELDB_DB_VERSION_SET_H_