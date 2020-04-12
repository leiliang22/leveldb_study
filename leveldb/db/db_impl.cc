#include "leveldb/db.h"
#include "db/db_impl.h"
#include "leveldb/status.h"
#include <iostream>

namespace leveldb {

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
        s = impl->versions_->LogAndApply(&edit, &impl->mutex_);
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
    Status s;
    std::cout << "PUT" << std::endl;
    return s;
}

Status DB::Get(const ReadOptions& opt, const Slice& key, std::string* value) {
    Status s;
    std::cout << "GET" << std::endl;
    return s;
}

Status DBImpl::Put(const WriteOptions& options, const Slice& key, const Slice& value) {
    Status s;
    std::cout << "IMPL PUT" << std::endl;
    return s;
}

Status DBImpl::Get(const ReadOptions& options, const Slice& key, std::string* value) {
    Status s;
    std::cout << "IMPL GET" << std::endl;
    return s;
}

}
