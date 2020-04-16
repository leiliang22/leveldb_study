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
    void UnRef();

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
                   InternalKeys* smallest, InternalKeys* largest);
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
};

class Compaction {

};
}  // namespace leveldb
#endif  // STORAGE_LEVELDB_DB_VERSION_SET_H_