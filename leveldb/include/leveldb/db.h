#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDB_INCLUDE_DB_H_

#include <stdint.h>
#include <stdio.h>

#include "leveldb/options.h"
#include "leveldb/export.h"
#include "leveldb/status.h"
#include "leveldb/slice.h"

namespace leveldb {

// 需要更新CMakeLists.txt,如果改变版本
static const int kMajorVersion = 1;
static const int kMinorVersion = 22;

struct Options;
struct ReadOptions;
struct WriteOptions;

class LEVELDB_EXPORT DB {
public:
    static Status Open(const Options& options, const std::string& name,
		       DB** dbptr);
    
    DB() = default;

    DB(const DB&) = delete;
    DB& operator=(const DB&) =delete;

    virtual ~DB();

    // return OK on success,
    // and a non-OK status on error.
    // Note: consider setting options.sync = true.
    virtual Status Put(const WriteOptions& options, const Slice& key,
		       const Slice& value) = 0;
    // if hit, value in *value and return OK.
    //
    // if not, *value unchanged and return a status
    // for which Status::IsNotFound() returns true. 
    //
    // may return some other Status on a error.
    virtual Status Get(const ReadOptions& options, const Slice& key,
		       std::string* value) = 0;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_DB_H_
