#ifndef STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_
#define STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_

#include <vector>

#include "leveldb/slice.h"

namespace leveldb {

struct Options;

class BlockBuilder {
public:
    explicit BlockBuilder(const Options* options);

    BlockBuilder(const BlockBuilder&) = delete;
    BlockBuilder& operator=(const BlockBuilder&) = delete;

    // reset the contents as if the BlockBuilder was just constructed
    void Reset();

    void Add(const Slice& key, const Slice& value);

    // finish building the block and return a slice that refers to the
    // block contents. the returned slice will remain valid for the
    // lifetime of this builder or until Reset() is called.
    Slice Finish();

    size_t CurrentSizeEstimate() const;

    bool empty() const { return buffer_.empty(); }
private:
    const Options* options_;
    std::string buffer_;                 // Destination buffer
    std::vector<uint32_t> restarts_;    // Restart points
    int counter_;                       // Number of entries emitted since restart
    bool finished_;                     // Has Finish() been called?
    std::string last_key_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_