#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <stddef.h>
#include "leveldb/export.h"

namespace leveldb {

struct LEVEL_EXPORT Options {
    Options();

    bool create_if_missing = false;
};

struct LEVEL_EXPORT WriteOptions {
    WriteOptions() = default;
};

struct LEVEL_EXPORT ReadOptions {
    ReadOptions() = default;
};

};  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
