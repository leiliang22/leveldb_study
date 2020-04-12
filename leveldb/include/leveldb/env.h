#ifndef STORAGE_LEVELDB_INCLUDE_ENV_H_
#define STORAGE_LEVELDB_INCLUDE_ENV_H_

#include "leveldb/export.h"
#include "leveldb/status.h"

namespace leveldb {

class LEVELDB_EXPORT Env {
public:
    Env();

    Env(const Env&) = delete;
    Env& operator=(const Env&) = delete;
    
    virtual ~Env();

    virtual Status NewWritableFile(const std::string& fname,
                                   WriteFile** result) = 0;
};

// A file abstraction for sequential writing. The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class LEVELDB_EXPORT WriteableFile {
public:
    WriteableFile() = default;

    WriteableFile(const WriteableFile&) = delete;
    WriteableFile& operator=(const WriteableFile&) = delete;

    virtual ~WriteableFile();

    virtual Status Append(const Slice& data) = 0;
    virtual Status Close() = 0;
    virtual Status Flush() = 0;
    virtual Status Sync() = 0;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_ENV_H_
