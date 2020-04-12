#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <stddef.h>
#include "leveldb/export.h"



namespace leveldb {

struct LEVELDB_EXPORT Options {
    Options();

    bool create_if_missing = false;
};

struct LEVELDB_EXPORT WriteOptions {
    WriteOptions() = default;
};

struct LEVELDB_EXPORT ReadOptions {
    ReadOptions() = default;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
