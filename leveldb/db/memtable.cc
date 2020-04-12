#include "db/memtable.h"

namespace leveldb {

MemTable::MemTable(const InternalKeyCompartor& compartor)
    : compartor_(compartor), refs_(0), table_(compartor_, &arena_) {}

}  // namespace leveldb