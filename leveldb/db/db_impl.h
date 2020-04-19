#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <string>
#include <deque>

#include "leveldb/db.h"
#include "db/memtable.h"
#include "db/table_cache.h"
#include "db/log_writer.h"

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


    void CompactMemtable() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    Status WriteLevel0Table(Memtable* mem, VersionEdit* edit, Version* base)
           EXCLUSIVE_LOCKS_REQUIRED(mutex_); 

    Status MakeRoomForWrite(bool force) EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void MaybeScheduleCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    static void BGWork(void* db);
    void BackgroundCall();
    void BackgroundCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    // constant after construction
    Env* const env_;
    const InternalKeyComparator internal_comparator_;
    const Options options_;  // options_.comparator == &internal_comparator_
    const std::string dbname_;

    // table_cache_ provides its own synchronization
    TableCache* const table_cache_;

    port::Mutex mutex_;
    std::atomic<bool> shutting_down_;
    MemTable* mem_;
    MemTable* imm_ GUARDED_BY(mutex_);
    WritableFile* logfile_;
    uint64_t logfile_number_ GUARDED_BY(mutex_);
    log::Writer* log_;
    VersionSet* const versions_ GUARDED_BY(mutex_);

    // queue of writers.
    std::deque<Writer*> writers_ GUARDED_BY(mutex_);

    std::set<uint64_t> pending_outputs_ GUARDED_BY(mutex_);
    bool background_compaction_scheduled_ GUARDED_BY(mutex_);
    Status bg_error_ GUARDED_BY(mutex_);
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_
