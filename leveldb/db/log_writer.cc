#include "db/log_writer.h"

namespace leveldb {

namespace log {

Writer::Writer(WritableFile* dest) : dest_(dest), block_offset_(0) {
    ;
}

}  // namespace log

}  // namespace leveldb