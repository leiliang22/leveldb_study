#ifndef STORAGE_LEVELDB_TABLE_FORMAT_H_
#define STORAGE_LEVELDB_TABLE_FORMAT_H_

#include <stdint.h>

#include "leveldb/status.h"
#include "leveldb/slice.h"

namespace leveldb {

class Block;
class RandomAccessFile;
struct ReadOptions;

// BlockHandle is a pointer to the extent of a file that stores a data
// block or a meta block
class BlockHandle {
public:
    // maximum encoding length of a BlockHandle
    enum { kMaxEncodedLength = 10 + 10 };

    BlockHandle();

    // the offset of the block in the file
    uint64_t offset() const { return offset_; }
    void set_offset(uint64_t offset) { offset_ = offset; } 

    // the size of the stored block
    uint64_t size() const { return size_; }
    void set_size(uint64_t size) { size_ = size; }

    void EncodeTo(std::string* dst) const;
    Status DecodeFrom(Slice* input);

private:
    uint64_t offset_;
    uint64_t size_;
};

// footer encapsulates the fixed information stored at the tail
// end of every table file.
class Footer {
public:
    // Encoded length of a Footer. Note that the serialization of a
    // footer will always occupy exactly this many bytes. It consists
    // of two block handles and a magic number.
    enum { kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8 };

    Footer() = default;
};

struct BlockContents {
    Slice data;             // Actual contents of data
    bool cachable;          // True if data can be cached  
    bool heap_allocated;    // True if caller should delete[] data.data()
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FORMAT_H_