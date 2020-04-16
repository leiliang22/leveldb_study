#include "db/log_writer.h"

namespace leveldb {

namespace log {

static void InitTypeCrc(uint32_t* type_crc) {
    for (int i = 0; i <= kMaxRecordType; i++) {
        char t = static_cast<char>(i);
        type_crc[i] = crc32c::Value(&t, 1);
    }
}

Writer::Writer(WritableFile* dest) : dest_(dest), block_offset_(0) {
    InitTypeCrc(type_crc_);
}

Writer::Writer(WritableFile* dest, uint64_t dest_length)
    : dest_(dest), block_offset_(dest_length % kBlockSize) {
    InitTypeCrc(type_crc_);
}

Writer::~Writer() = default;

Writer::AddRecord(const Slice& slice) {
    const char* ptr = slice.data();
}



}  // namespace log

}  // namespace leveldb