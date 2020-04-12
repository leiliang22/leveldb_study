#ifndef STORAGE_LEVELDB_DB_LOG_WRITER_H_
#define STORAGE_LEVELDB_DB_LOG_WRITER_H_

#include <stdint.h>

namespace leveldb {

class WritableFile;

namespace log {

class Writer {
public:
    explicit Writer::Writer(WritableFile* dest);

private:
    WritableFile* dest_;
    int block_offset_;
};

}  // namespace log

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_LOG_WRITER_H_