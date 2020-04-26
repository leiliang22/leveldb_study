#ifndef STORAGE_LEVELDB_INCLUDE_TABLE_H_
#define STORAGE_LEVELDB_INCLUDE_TABLE_H_

#include <stdint.h>

#include "leveldb/export.h"
#include "leveldb/iterator.h"

namespace leveldb {

class Block;
class BlockHandle;
class Footer;
class Options;
class RandomAccessFile;
struct ReadOptions;
class TableCache;

// a table is a sorted map from strings to strings. Tables are
// immutable and persistent. A Table may be safely accessed from 
// multi threads without external synchronization.
class LEVELDB_EXPORT Table {
public:
    // attempt to open the table that is stored in bytes [0, file_size] of "file",
    // and read the metadata entries necessary to allow retrieving data from the table
    //
    // if sucessful, return ok and sets "*table" to the newly opened table.
    // the client should delete "*table" when no longer needed. If there was a error while 
    // initializing the table, set "*table" to nullptr and returns a non-ok status. Does not 
    // take owership of "*source" but the client must ensure that "source" remains live
    // for the duration of the returned table's lifetime
    //
    // *file must remain live while this Table is in use
    static Status Open(const Options& options, RandomAccessFile* file,
                       uint64_t file_size, Table** table);

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;

    ~Table();

    // return a new iterator over the table contents.
    // the result of NewIterator is initially invalid (caller 
    // must call one Seek on the iterator before using it)
    Iterator* NewIterator(const ReadOptions&) const;

    // given a key, return an approximate byte offset in the file where the data
    // for that key begins (or would begin if the key were present in the file). the 
    // returned value is in terms of file bytes, and so includes effects like compression of
    // the underlying data
    uint64_t ApproximateOffsetOf(const Slice& key) const;

private:
    friend class TableCache;
    struct Rep;

    static Iterator* BlockReader(void*, const ReadOptions&, const Slice&);

    explicit Table(Rep* rep) : rep_(rep) {}

    // calls (*handle_result)(arg, ...) with the entry found after a call to Seek(key).
    // may not make such a call if filter policy  says that key is not present
    Status InternalGet(const ReadOptions&, const Slice& key, void* arg,
                       void (*handle_result)(void* arg, const Slice& k, const Slice& v));
    void ReadMeta(const Footer& footer);
    void ReadFilter(const Slice& filter_handle_value);

    Rep* const rep_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_TABLE_H_