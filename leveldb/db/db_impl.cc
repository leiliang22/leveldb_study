#include "leveldb/db.h"
#include "db/db_impl.h"
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
