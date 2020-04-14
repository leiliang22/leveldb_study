#ifndef STORAGE_LEVELDB_DB_MEMTABLE_H_
#define STORAGE_LEVELDB_DB_MEMTABLE_H_

#include <string>

#include "db/dbformat.h"
#include "leveldb/db.h"

namespace leveldb {

class InternalKeyCompartor;
class MemTableIterator;

class MemTable {
public:
    explicit MemTable(const InternalKeyCompartor& compartor);

    MemTable(MemTable&) = delete;
    MemTable& operator=(MemTable&) = delete;

    void Ref() { ++refs_; }

    void Unref() {
        --refs_;
        assert(refs_ >= 0);
        if (refs_ <= 0) {
            delete this;
        }
    }

    // returns an estimate of the number of bytes of data in use by this
    // data structure. It is safe to call when MemTable is being modified
    size_t ApproximateMemoryUsage();

    // todo NewIterator

    // add an entry into memtable that maps key to value at the
    // specified sequence number and with the specified type.
    // Typically value will be empty if type = kTypeDeletion.
    void Add(SequenceNumber seq, ValueType type, const Slice& key,
             const Slice& value);

    // If memtable contains a value for key, store it in *value and return true.
    // If memtable contains a deletion for key, store a NotFound() error
    // in *status and return true.
    // Else return false
    bool Get(const LookupKey& key, std::string* value, Status* s);

private:
    friend class MemTableIterator;
    friend class MemTableBackwardIterator;

    struct KeyCompartor {
        const InternalKeyCompartor comparator;
        explicit KeyCompartor(const InternalKeyComparator& c) : comparator(c) {}
        int operator()(const char* a, const char* b);
    };

    typedef SkipList<const char*, KeyCompartor> Table;

    ~MemTable();  // private since only Unref() should be used to delete it

    KeyCompartor compartor_;
    int refs_;
    Arena arena_;
    Table table_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_MEMTABLE_H_