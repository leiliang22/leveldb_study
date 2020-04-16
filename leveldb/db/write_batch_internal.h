#ifndef STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
#define STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_

#include "leveldb/write_batch.h"

namespace leveldb {

class Memtable;

// WriteBatchInternal provides static methods for manipulating a
// writebatch that we don't want in the public WriteBatch interface
class WriteBatchInternal {
public:
    // return the number of entries in the batch
    static int Count(const WriteBatch* batch);

    static void SetCount(const WriteBatch* batch);

    // return the sequence number for the start of this batch
    static SequenceNumber Sequence(const WriteBatch* batch);

    static void SetSequenceNumber(const WriteBatch* batch, SequenceNumber seq);

    static Slice Contents(const WriteBatch* batch) { return Slice(batch->rep_); } 

    static size_t ByteSize(const WriteBatch* batch) { return batch->rep_.size(); }

    static void SetContents(WriteBatch* batch, const Slice& contents);

    static Status InsertInto(const WriteBatch* batch, Memtable* memtable);

    static void Append(WriteBatch* dst, WriteBatch* src);
};

}  // namespace leveldb


#endif  // STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_