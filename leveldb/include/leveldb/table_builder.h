#ifndef STORAGE_LEVELDB_INCLUDE_TABLE_BUILDER_H_
#define STORAGE_LEVELDB_INCLUDE_TABLE_BUILDER_H_

#include <stdint.h>

#include <leveldb/export.h>
#include <leveldb/options.h>
#include <leveldb/status.h>

namespace leveldb {

class WritableFile;

class LEVELDB_EXPORT TableBuilder {
public:
    // Create a builder that will store the contents of the table it is
    // building in *file. Does not close file. it is up to the 
    // caller to close the file after calling Finish().
    TableBuilder(const Options& options, WritableFile* file);

    TableBuilder(const TableBuilder&) = delete;
    TableBuilder& operator=(const TableBuilder&) = delete;

    // REQUIRES: Ethier FInish or Abandon has been called
    ~TableBuilder();

    // change the options used by this builder.  Note: only some of the
    // option fields can be changed after construction. If a field is not 
    // allowed to change dynamically and its value in the structure passed to 
    // the constructor id different from its value passed to this method, this
    // method will return an error without change any fields.
    Status ChangeOptions(const Options& options);

    // REQUIRES : key is after any previously added key according to comparator.
    // REQUIRES : Finish() and Abandon() have not been called
    void Add(const Slice& key, const Slice& value);

    void Flush();

    Status status() const;

    Status Finish();

    void Abandon();

    // Number of calls to Add() so far.
    uint64_t NumEntries() const;

    // size of the file generated so far. If invoked after a successful 
    // Finish() call, returns the size of the final generated file
    uint64_t FileSize() const;

private:
    bool ok() const { return status().ok(); }
    void WriteBlock(BlockBuilder* block, BlockHandle* handle);
    void WriteRawBlock(const Slice& data, CompressionType, BlockHandle* handle);

    struct Rep;
    Rep* rep_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_TABLE_BUILDER_H_