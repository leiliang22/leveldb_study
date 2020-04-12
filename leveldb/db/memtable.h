#ifndef STORAGE_LEVELDB_DB_MEMTABLE_H_
#define STORAGE_LEVELDB_DB_MEMTABLE_H_

#include <string>

namespace leveldb {

class MemTable {
public:
    explicit MemTable(const InternalKeyCompartor& compartor);

    MemTable(MemTable&) = delete;
    MemTable& operator=(MemTable&) = delete;

    void Ref() { ++refs_; }

private:
    KeyCompartor compartor_;
    int refs_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_MEMTABLE_H_