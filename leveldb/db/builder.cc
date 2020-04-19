#include "db/builder.h"

#include "db/dbformat.h"
#include "db/filename.h"
#include "db/table_cache.h"
#include "db/version_edit.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"

namespace leveldb {

Status BuildTable(const std::string& dbname, Env* env, Option& options,
                  TableCache* table_cache, Iterator* iter, FileMetaData* meta) {
    Status s;
    meta->file_size = 0;
    iter->SeekToFirst();

    std::string fname = TableFileName(dbname, meta->number);
    if (iter->Valid()) {
        WritableFile* file;
        s = env->NewWritableFile(fname, &file);
        if (!s.ok()) {
            return s;
        }

        TableBuilder* builder = new TableBuilder(options, file);
        meta->smallest.DecodeFrom(iter->key());
        for (; iter->Valid(); iter->Next()) {
            Slice key = iter->key();
            meta->largest.DecodeFrom(key);
            builder->Add(key, iter->value());
        }

        // finish and check for builder errors
        s = builder->Finish();
        if (s.ok()) {
            s = file->Sync();
        }
        if (s.ok()) {
            s = file->Close();
        }
        delete file;
        file = nullptr;

        if (s.ok()) {
            // verify that the table is usable
            Iterator* it = table_cache->NewIterator(ReadOptions(), meta->file_number,
                                                    meta->file_size);
            s = it->status();
            delete it;
        }
    }

    // check for input iterator errors
    if (!iter->status().ok()) {
        s = iter->status()
    }

    if (s.ok() && meta->file_size > 0) {
        // keep it
    } else {
        env_->RemoveFile(fname);
    }
    return s;
}

}  // namespace leveldb