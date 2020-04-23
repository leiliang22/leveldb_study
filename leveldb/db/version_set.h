#ifndef STORAGE_LEVELDB_DB_VERSION_SET_H_
#define STORAGE_LEVELDB_DB_VERSION_SET_H_

#include <map>
#include <set>
#include <vector>

#include "db/dbformat.h"
#include "db/version_edit.h"

namespace leveldb {


class Version {
public:

    struct GetStats {
        FileMetaData* seek_file;
        int seek_file_level;
    };

    void AddIterators(const ReadOptions&, std::vector<Iterator*>* iters);

    Status Get(const ReadOption&, const LookupKey& key, std::string* val,
               GetStats* stats);

    bool UpdateStats(const GetStats& stats);

    bool RecordReadSample(Slice key);

    void Ref();
    void Unref();

    void GetOverlappingInputs(int level,
            const InternalKey* begin,  // nullptr means before all keys
            const InternalKey* end,    // nullptr means after all keys
            std::vector<FileMetaData*>* inputs);

    bool OverlapInLevel(int level, const Slice* smallest_user_key,
                        const Slice* largest_user_key);

    int PickLevelForMemTableOutput(const Slice& smallest_user_key,
                                   const Slice& largest_user_key);

    int NumFiles(int level) const { return files_[level].size(); }

    std::string DebugString() const;

private:
    friend class Compaction;
    friend class VersionSet;

    class LevelFileNumIterator;

    explicit Version(VersionSet* vset)
        : vset_(vset),
          next_(this),
          prev_(this),
          refs_(0),
          file_to_compact_(nullptr),
          file_to_compact_level_(-1),
          compaction_score_(-1),
          compaction_level_(-1) {}

    Version(const Version&) = delete;
    Version& operator=(const Version&) = delete;

    ~Version();

    Iterator* NewConcatenatingIterator(const ReadOptions&, int level) const;

    void ForEachOverlapping(Slice user_key, Slice internal_key, void* args,
                            bool (*func)(void*, int, FileMetaData*));

    VersionSet* vset_;
    Version* next_;
    Version* prev_;
    int refs_;

    std::vector<FileMetaData*> files_[config::kNumLevels];

    FileMetaData* file_to_compact_;
    int file_to_compact_level_;

    double compaction_score_;
    int compaction_level_;
};

class VersionSet {
public:
    VersionSet();

    VersionSet(const VersionSet&) = delete;
    VersionSet& operator=(const VersionSet&) = delete;

    uint64_t NewFileNumber() { return next_file_number_++; }

    Status LogAndApply(VersionEdit* edit, port::Mutex* mu);

private:
    class Builder;

    friend class Compaction;
    friend class Version;

    bool ReuseManifest(const std::string& dscname, const std::string& dscbase);
    void Finalize(Version* v);
    void GetRange(const std::vector<FileMetaData*>& inputs, InternalKey* smallest,
                  InternalKey* largest);
    void GetRange2(const std::vector<FileMetaData*>& inputs1,
                   const std::vector<FileMetaData*>& inputs2,
                   InternalKey* smallest, InternalKey* largest);
    void SetupOtherInputs(Compaction* c);

    // Save current contents to *log
    Status WriteSnapshot(log::Writer* log);
    void AppendVersion(Version* v);

    Env* const env_;
    const std::string dbname_;
    const Options* const options_;
    TableCache* const table_cache_;
    const InternalKeyComparator icmp_;
    uint64_t next_file_number_;
    uint64_t manifest_file_number_;
    uint64_t last_sequence_;
    uint64_t log_number_;
    uint64_t prev_log_number_;  // 0 or backing store for memtable being compacted

    // open lazy
    WritableFile* descriptor_file_;
    log::Writer* descriptor_log_;
    Version dummy_versions_;  // Head of circular doubly-linked list of versions
    Version* current_;        // == dummy_versions_.prev_

    // per-level key at which the next compaction at that level should start
    // either an empty string, or a valid internalKey
    std::string compact_pointer_[config::kNumLevels];
};

class Compaction {
public:
    ~Compaction();

    // return the level that is being compacted. inputs from "level"
    // and "level + 1" will be merged to produce a set of "level+1" files
    int level() const { return level_; }

    VersionEdit* edit() { return &edit_; }

    // which must be either 0 or 1
    int num_input_files(int which) const { return inputs_[which].size(); }
    FileMetaData* input(int which, int i) const { return inputs_[which][i]; }
    uint64_t MaxOutputFileSize() const { return max_output_file_size; }

    // is this a trivial compaction that can be implemented by just
    // moving a single input file to the next level (no merging or splitting)
    bool IsTrivialMove() const;

    // add all inputs to this compaction as delete operations to *edit
    void AddInputDeletions(VersionEdit* edit);

    // return true iff the information we have available guarantees that 
    // the compaction is producing data in "level+1" for which no data exists
    // in levels greater than "level + 1".
    bool IsBaseLevelForKey(const Slice& user_key);

    // return true if  we should stop building the current output 
    // before processing "internal_key"
    bool ShouldStopBefore(const Slice& internal_key);

    // release the input version for the compaction, once the compaction is succ.
    void ReleaseInputs();

private:
    friend class Version;
    friend class VersionSet;

    Compaction(const Option* options, int level);

    int level_;
    uint64_t max_output_file_size;
    Version* input_version_;
    VersionEdit edit_;

    // Each compaction reads inputs from "level_" and "level_ + 1"
    std::vector<FileMetaData*> inputs_[2];

    // state used to check for number of overlapping grandparent files
    // (parent == level_+1, grandparent == level_ + 2)
    std::vector<FileMetaData*> grandparents_;
    size_t grandparent_index_;  // Index in grandparent files
    bool seen_key_;             // Some output key has been seen
    int64_t overlapped_bytes_;  // Bytes of overlap between current output
                                // and grandparent files

    // state for implementing IsBaseLevelForKey

    // level_ptrs_ holds indices into input_version_->levels_: our state
    // is that we are positioned at one of the file ranges for each 
    // higher level than the ones involved in this compaction 
    // (i.e. for all L >= level_ + 2)
    size_t level_ptrs_[config::kNumLevels];
};
}  // namespace leveldb
#endif  // STORAGE_LEVELDB_DB_VERSION_SET_H_