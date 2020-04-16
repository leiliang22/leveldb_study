#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <string>
#include <deque>

#include "leveldb/db.h"
#include "db/memtable.h"

namespace leveldb {

class DBImpl : public DB {
public:
    DBImpl(const Options& options, const std::string& dbname);

    DBImpl(DBImpl&) = delete;
    DBImpl& operator=(DBImpl&) = delete;

    Status Put(const WriteOptions&, const Slice& key, const Slice& value) override;
    Status Write(const WriteOptions& options, WriteBatch* updates) override;
    Status Get(const ReadOptions& options, const Slice& key, std::string* value) override;

private:
    friend class DB;
    struct Writer;

    // constant after construction
    const InternalKeyComparator internal_comparator_;

    port::Mutex mutex_;
    MemTable* mem_;
    MemTable* imm_ GUARDED_BY(mutex_);
    WritableFile* logfile_;
    uint64_t logfile_number_ GUARDED_BY(mutex_);
    log::Writer* log_;
    VersionSet* const versions_ GUARDED_BY(mutex_);

    // queue of writers.
    std::deque<Writer*> writers_ GUARDED_BY(mutex_);
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_
