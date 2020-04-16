#ifndef STORAGE_LEVELDB_DB_LOG_WRITER_H_
#define STORAGE_LEVELDB_DB_LOG_WRITER_H_

#include <stdint.h>

#include "db/log_format.h"
#include "leveldb/status.h"
#include "leveldb/slice.h"

namespace leveldb {

class WritableFile;

namespace log {

class Writer {
public:
    // create a writer that will append data to "*dest"
    // *dest must remain live while this writer is inuse
    explicit Writer(WritableFile* dest);
    Writer(WritableFile* dest, uint64_t dest_length);

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    ~Writer();

    Status AddRecord(const Slice& slice);

private:
    WritableFile* dest_;
    int block_offset_;  // current offset in block

    // crc32 values for all supported record types. These are 
    // pre-computed to reduce the overhead of computing the crc of the
    // record type stored in the header.
    uint32_t type_crc_[kMaxRecordType + 1];
};

}  // namespace log

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_LOG_WRITER_H_