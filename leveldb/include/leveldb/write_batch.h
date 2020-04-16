#ifndef STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_
#define STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_

#include <string>

#include "leveldb/export.h"
#include "leveldb/status.h"

namespace leveldb {

class Slice;

class LEVELDB_EXPORT WriteBatch {
public:
    class LEVELDB_EXPORT Handler {
    public:
        virtual ~Handler();
        virtual void Put(const Slice& key, const Slice& value) = 0;
        virtual void Delete(const Slice& key) = 0;
    };

    WriteBatch();

    // Intentionally copyable
    WriteBatch(const WriteBatch&) = default;
    WriteBatch& operator=(const WriteBatch&) = default;

    ~WriteBatch();

    void Put(const Slice& key, const Slice& value);

    // if db contains key, erase it, else do noting
    void Delete(const Slice& key);

    // clear all buffer in this batch
    void Clear();

    // the size of the db changes caused by this batch
    size_t ApproximateSize() const;

    // copies the operations in "source" to this batch.
    // this runs O(source size) time. 
    void Append(const WriteBatch& source);

    Status Iterate(Handler* handler) const;

private:
    friend class WriteBatchInternal;
    std::string rep_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_