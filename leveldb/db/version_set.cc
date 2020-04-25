#include "db/version_set.h"
#include "leveldb/status.h"

namespace leveldb {


namespace {
enum SaverState {
    kNotFound,
    kFound,
    kDeleted,
    kCorrupt,
};
struct Saver {
    SaverState state;
    const Comparator* ucmp;
    Slice user_key;
    std::string* value;
};
}  // namespace

Status Version::Get(const ReadOptions& options, const LookupKey& k,
                    std::string* value, GetStats* stats) {
    stats->seek_file = nullptr;
    stats->seek_file_level = -1;

    struct State {
        Saver saver;
        GetStats* stats;
        const ReadOptions* options;
        Slice ikey;
        FileMetaData* last_file_read;
        int last_file_read_level;

        VersionSet* vset;
        Status s;
        bool found;

        static bool Match(void* arg, int level, FileMetaData* f) {
            State* state = reinterpret_cast<State*>(arg);

            if (state->stats->seek_file == nullptr &&
                    state->last_file_read != nullptr) {
                // we have had more than one seek for this read. charge the 1st file
                state->stats->seek_file = state->stats->last_file_read;
                state->stats->seek_file_level = state->last_file_read_level;
            }

            state->last_file_read = f;
            state->last_file_read_level = level;

            state->s = state->vset->table_cache_->Get(*state->options, f->number,
                                                      f->file_size, state->ikey,
                                                      &state->saver, SaveValue);
            if (!state->s.ok()) {
                state->found = true;
                return false;
            }
            switch (state->saver.state) {
                case kNotFound:
                    return true;  // keep search in other files
                case kFound:
                    state->found = true;
                    return false;
                case kDeleted:
                    return false;
                case kCorrupt:
                    state->s = Status::Corruption("corrupted key for ", state->saver.user_key);
                    state->found = true;
                    return false;
            }

            // not reached. added to avoid false compilation warnings of 
            // "control reaches end of non-void function".
            return false;
        }
    };

    State state;
    state.found = false;
    state.stats = stats;
    state.last_file_read = nullptr;
    satte.last_file_read_level = -1;

    state.options = &options;
    state.ikey = k.internal_key();
    state.vset = vset_;

    state.saver.state = kNotFound;
    state.saver.ucmp = vset_->icmp_.user_comparator();
    state.saver.user_key = k.user_key();
    state.saver.value = value;

    ForEachOverlapping(state.saver.user_key, state.ikey, &state, &State::Match);

    return state.found ? state.s : Status::NotFound(Slice());
}

// a helper class so we can efficiently apply a whole sequence of
// edits to a particular state without creating intermdiate Versions 
// that contain full copies of the in
class VersionSet::Builder {
private:
    // helper to sort by v->files_[file_number].smallest
    struct BySmallestKey {
        const InternalKeyComparator* internal_comparator;

        bool operator()(FileMetaData* f1, FileMetaData* f2) const {
            int r = internal_comparator->Compare(f1->smallest, f2->smallest);
            if (r != 0) {
                return (r < 0);
            } else {
                // break ties by file number
                return (f1->number < f2->number);
            }
        }
    };

    typedef std::set<FileMetaData*, BySmallestKey> FileSet;
    struct LevelState {
        std::set<uint64_t> deleted_files;
        FileSet* added_files;
    };

    VersionSet* vset_;
    Version* base_;
    LevelState levels_[config::kNumLevels];

public:
    // initialize a builder with the files *base and other info from *vset
    Builder(VersionSet* vset, Version* base) : vset_(vset), base_(base) {
        base_->Ref();
        BySmallestKey cmp;
        cmp.internal_comparator = &vset_->icmp_;
        for (int level = 0; level < config::kNumLevels; level++) {
            levels_[level].added_files = new FileSet(cmp);
        }
    }

    ~Builder() {
        for (int level = 0; level < config::kNumLevels; level++) {
            const FileSet* added = levels_[level].added_files;
            std::vector<FileMetaData*> to_unref;
            to_unref.reserve(added->size());
            for (FileSet::const_iterator it = added->begin(); it != added->end(); ++it) {
                to_unref.push_back(*it);
            }
            delete added;
            for (int i = 0; i < to_unref.size(); ++i) {
                FileMetaData* f = to_unref[i];
                f->refs--;
                if (f->refs <= 0) {
                    delete f;
                }
            }
        }
        base_->Unref();
    }

    // apply all of the edits in *edit to the current state
    void Apply(VersionEdit* edit) {
        // update compaction pointers
        for (size_t i = 0; i < edit->compact_pointers_.size(); i++) {
            const int level = edit->compact_pointers_[i].first;
            vset_->compact_pointer_[level] = 
                edit->compact_pointers_[i].second.Encode().ToString();
        }

        // delete files
        for (const auto& deleted_file_set_kvp : edit->deleted_files_) {
            const int level = deleted_file_set_kvp.first;
            const uint64_t number = deleted_file_set_kvp.second;
            levels_[level].deleted_files.insert(number);
        }

        // add new files
        for (int i = 0; i < edit->new_files_.size(); i++) {
            const int level = edit->new_files_[i].first;
            FileMetaData* f = new FileMetaData(edit->new_files_[i].second);
            f->refs = 1;

            f->allowed_seeks = static_cast<int>((f->file_size / 16384U));
            if (f->allowed_seeks < 100) f->allowed_seeks = 100;

            levels_[level].deleted_files.erase(f->number);
            levels_[level].added_files.insert(f);
        }
    }

    // save the current state in *v
    void SaveTo(Version* v) {
        BySmallestKey cmp;
        cmp.internal_comparator = &vset_->icmp_;
        for (int level = 0; level < config::kNumLevels; level++) {
            // merge the set of added files with the set of pre-existing files
            // drop any deleted files. store the result in *v
            const std::vector<FileMetaData*>& base_file = base_->files_[level];
            std::vector<FileMetaData*>::const_iterator base_iter = base_file.begin();
            std::vector<FileMetaData*>::const_iterator base_end = base_file.end();
            const FileSet* added_files = levels_[level].added_files;
            v->files_[level].reserve(base_file.size() + added_files->size());
            for (const auto& added_file : *added_files) {
                // add all smaller files listed in base_
                for (std::vector<FileMetaData*>::const_iterator bpos = 
                        std::upper_bound(base_iter, base_end, added_file, cmp);
                     base_iter != bpos; ++base_iter) {
                    MaybeAddFile(v, level, *base_iter);
                }
                MaybeAddFile(v, level, added_file);
            }

            // add remaining base files
            for (; base_iter != base_end; ++base_iter) {
                MaybeAddFile(v, level, *base_iter);
            }
        }
    }

    void MaybeAddFile(Version* v, int level, FileMetaData* f) {
        if (levels_[level].deleted_files.count(f->number) > 0) {
            // file is deleted, do nothing 
        } else {
            std::vector<FileMetaData*>* files = &v->files_[level];
            if (level > 0 && !files->empty()) {
                // must not overlap
                assert(vset_->icmp_.Compare((*files)[files->size() - 1]->largest,
                                            f->smallest) < 0);
            }
            f->refs++;
            files->push_back(f);
        }
    }
};

void VersionSet::AppendVersion(Version* v) {
    // make "v" current
    assert(v->refs_ == 0);
    assert(v != current_);
    if (current_ != nullptr) {
        current_->Unref();
    }
    current_ = v;
    v->Ref();

    // append to linked list
    v->prev_ = dummy_versions_.prev_;
    v->next_ = &dummy_versions_;
    v->prev_->next_ = v;
    v->next_->prev_ = v;
}

void VersionSet::Finallize(Version* v) {
    // precompute best level for next compaction
    int best_level = -1;
    double best_score = -1;

    for (int level = 0; level < config::kNumLevels - 1; level++) {
        double score;
        if (level == 0) {
            score = v->files_[level].size() / static_cast<double>(config::kL0_CompactionTrigger);
        } else {
            // compute the ratio of current size to size limit
            const uint64_t level_bytes = TotalFileSize(v->files_[level]);
            score = static_cast<double>(level_bytes) / MaxByteForLevel(options_, level);
        }

        if (score > best_score) {
            best_level = level;
            best_score = score;
        }
    }
    v->compaction_level_ = best_level;
    v->compaction_score_ = best_score;
}

Status VersionSet::WriteSnapshot(log::Writer* log) {
    // save metadata
    VersionEdit edit;
    edit.SetComparatorName(icmp_.user_comparator()->Name());

    // save compaction pointers
    for (int level = 0; level < config::kNumLevels; level++) {
        if (!compact_pointer_[level].empty()) {
            InternalKey key;
            key.DecodeFrom(compact_pointer_[level]);
            edit.SetCompactPointer(level, key);
        }
    }

    // save files
    for (int level = 0; level < config::kNumLevels; level++) {
        const std::vector<FileMetaData*>& files = current_->files_[level];
        for (size_t i = 0; i < files.size(); i++) {
            const FileMetaData* f = files[i];
            edit.AddFile(level, f->number, f->file_size, f->smallest, f->largest);
        }
    }

    std::string record;
    edit.EncodeTo(&record);
    return log->AddRecord(record);
}

Status VersionSet::LogAndApply(VersionEdit* edit, port::Mutex* mu) {
    if (edit->has_log_number_) {
        assert(edit->log_number_ >= log_number_);
        assert(edit->log_number_ < next_file_number_);
    } else {
        edit->SetLogNumber(log_number_);
    }

    if (!edit->has_prev_log_number_) {
        edit->SetPrevLogNumber(prev_log_number_);
    }

    edit->SetNextFile(next_file_number_);
    edit->SetLastSequence(last_sequence_);

    Version* v = new Version(this);
    {
        Builder builder(this, current_);
        builder.Apply(edit);
        builder.SaveTo(v);
    }

    Finalize(v);

    // Initialize new descriptor log file if necessary by creating a
    // temporary file that contains a snapshot of the current version.
    std::string new_manifest_file;
    Status s;
    if (descriptor_file_ == nullptr) {
        // only open db can hit this so no need to unlock.
        assert(descriptor_file_ == nullptr);
        new_manifest_file = DescriptorFileName(dbname_, manifest_file_number_);
        edit->SetNextFile(next_file_number_);
        s = env_->NewWritableFile(new_manifest_file, &descriptor_file_);
        if (s.ok()) {
            descriptor_log_ = new log::Writer(descriptor_file_);
            s = WriteSnapshot(descriptor_log_);
        }
        // if we just create a new descriptor
    }

    {
        mu->Unlock();
        // write new record to MANIFEST log
        if (s.ok()) {
            std::string record;
            edit->EncodeTo(&record);
            s = descriptor_log_->AddRecord(record);
            if (s.ok()) {
                descriptor_file_->Sync();
            }
            if (!s.ok()) {
                Log(options_->info_log, "MANIFEST write: %s\n", s.ToString().c_str());
            }
        }
        // if we just created a new descriptor file, install it by writing a
        // new CURRENT file that points to it.
        if (s.ok() && !new_manifest_file.empty()) {
            s = SetCurrentFile(env_, dbname_, manifest_file_number_);
        }
        mu->Lock();
    }

    // install the new version
    if (s.ok()) {
        AppendVersion(v);
        log_number_ = edit->log_number_;
        prev_log_number_ = edit->prev_log_number_;
    } else {
        delete v;
        if (!new_manifest_file.empty()) {
            delete descriptor_log_;
            delete descriptor_file_;
            descriptor_log_ = nullptr;
            descriptor_file_ = nullptr;
            env_->RemoveFile(new_manifest_file);
        }
    }

    return s;
}

}  // namespace leveldb
