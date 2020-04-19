#include "leveldb/db.h"
#include "db/db_impl.h"
#include "db/version_set.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"
#include <iostream>

namespace leveldb {

struct DBImpl::Writer {
    explicit DBImpl::Writer(port::Mutex* mu)
        : batch(nullptr), sync(false), done(false), cv(mu) {}
    
    Status status;
    WriteBatch* batch;
    bool sync;
    bool done;
    port::CondVar cv;
};

DBImpl::DBImpl(const Options& raw_options, const std::string& dbname)
    : env_(raw_options.env)  
    {}

Status DBImpl::Recover(VersionEdit* edit, bool* save_manifest) {
    return Status::OK();
}

DB::~DB() = default;

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
    *dbptr = nullptr;
    *dbptr = new DBImpl();
    Status s;
    std::cout << "OPEN" << std::endl;
    return s;

    // real impl
    *dbptr = nullptr;

    DBImpl* impl = new DBImpl(options, dbname);
    impl->mutex_.Lock();
    VersionEdit edit;
    // Recover handles create_if_missing, error_if_exists
    bool save_manifest = false;
    Status s = impl->Recover(&edit, &save_manifest);
    if (s.ok() && impl->mem_ == nullptr) {
        // create new log & a corresponding memtable.
        uint64_t new_log_number = impl->versions_->NewFileNumber();
        WriteableFile* lfile;
        s = options.env->NewWritableFile(LogFileName(dbname, new_log_number),
                                         &lfile);
        if (s.ok()) {
            edit.SetLogNumber(new_log_number);
            impl->logfile_ = lfile;
            impl->logfile_number_ = new_log_number;
            impl->log_ = new log::Writer(lfile);
            impl->mem_ = new MemTable(impl->internal_comparator_);
            impl->mem_->Ref();
        }
    }

    if (s.ok() && save_manifest) {
        edit.SetPrevLogNumber(0);  // No older logs needed after recovery
        edit.SetLogNumber(impl->logfile_number_);
        //s = impl->versions_->LogAndApply(&edit, &impl->mutex_);
    }
    if (s.ok()) {
        assert(impl->mem_ != nullptr);
        *dbptr = impl;
    } else {
        delete impl;
    }
    return s;
}

Status DB::Put(const WriteOptions& opt, const Slice& key, const Slice& value) {
    WriteBatch batch;
    batch.Put(key, value);
    return Write(opt, &batch);
}

Status DB::Get(const ReadOptions& opt, const Slice& key, std::string* value) {
    Status s;
    std::cout << "GET" << std::endl;
    return s;
}

Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& val) {
    return DB::Put(o, key, val);
}

Status DBImpl::WriteLevel0Table(Memtable* mem, VersionEdit* edit, Version* base) {
    mutex_.AssertHeld();
    const uint64_t start_micros = env_->NowMicros();
    FileMetaData meta;
    meta.number = versions_->NewFileNumber();
    pending_outputs_.insert(meta.number);
    Iterator iter = mem->NewIterator();
    Log(options_.info_log, "Level-0 table #%llu: started",
        (unsigned long long)meta.number);
    
    Status s;
    {
        mutex_.Unlock();
        s = BuildTable(dbname_, env_, options_, table_cache_, iter, &meta);
        mutex_.Lock();
    }

    Log(options_.info_log, "Level-0 table #%llu: %lld bytes %s",
        (unsigned long long)meta.number, (unsigned long long)meta.file_size,
        s.ToString().c_str());
    delete iter;
    pending_outputs_.erase(meta.number);

    // Note that if file_size is zero, the file has been deleted and
    // should not be added to the manifest
    int level = 0;
    if (s.ok() && meta.file_size > 0) {
        const Slice min_user_key = meta.smallest.user_key();
        const Slice max_user_key = meta.largest.user_key();
        if (base != nullptr) {
            level = base->PickLevelForMemtableOutput(min_user_key, max_user_key);
        }
        edit->AddFile(level, meta.number, meta.file_size, meta.smallest,
                      meta.largest);
    }

    CompactionStats stats;
    stats.micros = env_->NowMicros() - start_micros;
    stats.bytes_written = meta.file_size;
    stats_[level].Add(stats);
    return s;
}

void DBImpl::CompactMemtable() {
    mutex_.AssertHeld();
    assert(imm_ != nullptr);

    // save the contents of the memtable as a new Table
    VersionEdit edit;
    Version* base = versions_->current();
    base->Ref();
    Status s = WriteLevel0Table(imm_, &edit, base);
    base->Unref();

    if (s.ok() && shutting_down_.load(std::memory_order_acquire)) {
        s = Status::IOError("Delete DB during memtable compaction");
    }

    // Replace immutable memtable with the generated Table
    if (s.ok()) {
        edit.SetPrevLogNumber(0);
        edit.SetLogNumber(logfile_number_);  // Earlier logs no longer needed
        s = versions_->LogAndApply(&edit, &mutex_);
    }

    if (s.ok()) {
        imm_->Unref();
        imm_ = nullptr;
        has_imm_.store(false, std::memory_order_acquire);
        RemoveObsoleteFiles();
    } else {
        RecordBackgroundError(s);
    }
}

void DBImpl::MaybeScheduleCompaction() {
    mutex_.AssertHeld();
    if (background_compaction_scheduled_) {
        // already scheduled
    } else if (shutting_down_.load(std::memory_order_acquire)) {
        // DB is being deleted; no more background compactions
    } else if (!bg_error_.ok()) {
        // already got an error; no more changes
    } else if (imm_ == nullptr && manual_compaction_ == nullptr &&
               !versions_->NeedsCompaction()) {
        // no work to be done
    } else {
        background_compaction_scheduled_ = true;
        env_->Schedule(&DbImpl::BGWork, this);
    }
}

void DBImpl::BGWork(void* db) {
    reinterpret_cast<DBImpl*>(db)->BackgroundCall();
}

void DBImpl::BackgroundCall() {
    MutexLock l(&mutex_);
    assert(background_compaction_scheduled_);
    if (shutting_down_.load(std::memory_order_acquire)) {
        // no more background work when shutting down
    } else if (!bg_error_.ok()) {
        // no more background work after a background error
    } else {
        BackgroundCompaction();
    }

    background_compaction_scheduled_ = false;

    // previous compaction may have produced too many files in a level,
    // so reschedule another compaction if needed. 
    MaybeScheduleCompaction();
    background_work_finish_signal_.SignalAll();
}

void DBImpl::BackgroundCompaction() {
    mutex_.AssertHeld();
    if (imm_ != nullptr) {
        CompactMemtable();
        return;
    }

    Compaction* c;
    bool is_manual = (manual_compaction_ != nullptr);
    InternalKey manual_end;
    if (is_manual) {

    } else {
        c = versions_->PickCompaction();
    }

    Status status;
    if (c == nullptr) {
        // nothing to do
    } else if (!is_manual && c->IsTrivialMove()) {
        // move file to next level
        assert(c->num_input_files(0) == 1);
        FileMetaData* f = c->input(0, 0);
        c->edit()->RemoveFile(c->level(), f->number);
        c->edit()->AddFile(c->level() + 1, f->number, f->file_size, f->smallest,
                           f->largest);
        status = versions_->LogAndApply(c->edit(), &mutex_);
        if (!status.ok()) {
            RecordBackgroundError(status);
        }
        VersionSet::LevelSummaryStorage tmp;
        Log(options_.info_log, "Move #%lld to level-%d %lld bytes %s: %s\n",
            static_cast<unsigned long long>(f->number), c->level() + 1,
            static_cast<unsigned long long>(f->file_size),
            status.ToString().c_str(), versions_->LevelSummary(&tmp));
    } else {
        CompactionState* compact = new CompactionState(c);
        status = DoCompactionWork(compact);
        if (!status.ok()) {
            RecordBackgroundError(status);
        }
        CleanupCompaction(compact);
        c->ReleaseInputs();
        RemoveObsoleteFiles();
    }
    delete c;

    if (status.ok()) {
        // done
    } else if (shutting_down_.load(std::memory_order_acquire)) {
        // ignore compaction errors found during shutting down
    } else {
        Log(options_.info_log, "Compaction error: %s", status.ToString().c_str());
    }

    if (is_manual) {}
}

// REQUIRES : mutex_ is held
// REQUIRES : this thread is currently at the front of the writer queue
Status DBImpl::MakeRoomForWrite(bool force) {
    mutex_.AssertHeld();
    assert(!writes_.empty());
    bool allow_delay = !force;
    Status s;
    while (true) {
        if (bg_error_.ok()) {
            s = bg_error_;
            break;
        } else if (allow_delay && versions_->NumLevelFile(0) >=
                                      config::kL0_SlowdownWritesTrigger) {
            // we are getting close to hitting a hard limit on the number of
            // L0 files. Rather than delaying a single write by several seconds
            // when we hit the hard limit, start delaying each individual write
            // by 1ms to reduce latency varience. Also, this delay hands over some CPU 
            // to the compaction thread in case it is sharing the same core as the writer
            mutex_.Unlock();
            env_->SleepForMicroseconds(1000);
            allow_delay = false;
            mutex_.Lock();
        } else if (!force && (mem_->ApproximateMemoryUsage() <= options_.write_buffer_size)) {
            // there is room is current table
            break;
        } else if (imm_ != nullptr) {
            // we have filled up the current memtable, but the previous one 
            // is still being conpacted, so we wait.
            Log(options_->info_log, "Current memtable full; waiting...\n");
            background_work_finished_signal_.Wait();
        } else if (versions_->NumLevelFile(0) >= config::kL0_StopWritesTrigger){
            // there are too many level-0 files
            Log(options_->info_log, "Too many L0 files, waiting...\n");
            background_work_finished_signal_.Wait();
        } else {
            // Attempt to switch to a new memtable and trigger compaction of old
            assert(versions_->PrevLogNumber() == 0);
            uint64_t new_log_number = versions_->NewFileNumber();
            WritableFile* lfile = nullptr;
            s = env_->NewWritableFile(LogFileName(dbname_, new_log_number), &lfile);
            if (!s.ok()) {
                // avoid chewing througth file number space in a tight up
                versions_->ReuseFileNumber(new_log_number);
                break;
            }
            delete log_;
            delete logfile_;
            logfile_ = lfile;
            logfile_number_ = new_log_number;
            log_ = new log::Writer(lfile);
            imm_ = mem_;
            has_imm_.store(true, std::memory_order_release);
            mem_ = new MemTable(internal_comparator_);
            mem_.Ref();
            force = false;
            MaybeScheduleCompaction();
        }
    }
    return s;
}

Status DBImpl::Write(const WriteOptions& options, WriteBatch* updates) {
    Writer w(&mutex_);
    w.batch = updates;
    w.sync = options.sync;
    w.done = false;

    MutexLock l(&mutex_);
    writers_.push_back(&w);
    while (!w.done() && &w != writers_.front()) {
        w.cv.Wait();
    }
    if (w.done) {
        return w.status;
    }

    // May temporarily unlock and wait
    Status status = MakeRoomForWrite(updates == nullptr);
    uint64_t last_sequence = versions_->LastSequence();
    Writer* last_writer = &w;
    if (status.ok() && updates != nullptr) {  // nullptr batch is for compactions
        WriteBatch* write_batch = BuildBatchGroup(&last_sequence);
        WriteBatchInternal::SetSequence(write_batch, last_sequence + 1);
        last_sequence += WriteBatchInternal::Count(write_batch);
        {
            mutex_.Unlock();
            status = log_->AddRecord(WriteBatchInternal::Contents(write_batch));
            bool sync_error = false;
            if (status.ok() && options.sync) {
                status = logfile_->Sync()
                if (!status.ok()) {
                    sync_error = true;
                }
            }
            if (status.ok()) {
                status = WriteBatchInternal::InsertInto(write_batch, mem_);
            }
            mutex_.Lock();
            if (sync_error) {
                RecordBackgroundError(status);
            }
        }
        if (write_batch == tmp_batch_) tmp_batch_->Clear();

        versions_->SetLastSequence(last_sequence);
    }

    while (true) {
        Writer* ready = writers_.front();
        writers_.pop_front();
        if (ready != &w) {
            ready->status = status;
            ready->done = true;
            ready->cv.Signal()
        }
        if (ready == last_writer) break;
    }

    if (!writers_.empty()) {
        writers_.front()->cv.Signal();
    }

    return status;
}

Status DBImpl::Get(const ReadOptions& options, const Slice& key, std::string* value) {
    Status s;
    std::cout << "IMPL GET" << std::endl;
    return s;
}

}
