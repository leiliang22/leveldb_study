#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

#include <string>

#include "leveldb/slice.h"

namespace leveldb {


typedef uint64_t SequenceNumber;

// we leave 8bits empty at the bottom so a type and sequence# can be packed together into 64-bits/
static const SequenceNumber kMaxSequenceNumber = ((0x1ull << 56) - 1);


class InternalKey {
private:
    std::string rep_;

public:
    InternalKey() {}  // leave rep_ as empty to indicate it is invalid
    InternalKey(const Slice& user_key, SequenceNumber s, ValueType t) {
        AppendInternalKey(&rep_, ParsedInternalKey(user_key, s, t));
    }

    bool DecodeFrom(const Slice& s) {
        rep_.assign(s.data(), s.size());
        return !rep_.empty();
    }

    Slice Encode() const {
        assert(!rep_.empty());
        return rep_;
    }

    Slice user_key() const { return ExtractUserKey(rep_); }

    void SetFrom(const ParsedInternalKey& p) {
        rep_.clear();
        AppendInternalKey(&rep_, p);
    }

    void Clear() { rep_.clear(); }
    std::string DebugString() const;
};


class InternalKeyComparator {
public:
    InternalKeyComparator() {}

private:
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DBFORMAT_H_