#include "db/version_edit.h"

#include "db/version_set.h"

namespace leveldb {

enum Tag {
    kComparator = 1,
    kLogNumber = 2,
    kNextFileNumber = 3,
    kLastSequence = 4,
    kCompactPointer = 5,
    kDeleteFile = 6,
    kNewFile = 7,
    // 8 was used for large value refs
    kPrevLogNumber = 9
};

void VersionEdit::Clear() {
    comparator_.clear();
    log_number_ = 0;
    prev_log_number_ = 0;
    last_sequence_ = 0;
    next_file_number_ = 0;
    has_last_sequence_ = false;
    has_log_number_ = false;
    has_prev_log_number_ = false;
    has_next_file_number_ = false;
    has_comparator_ = false;
    deleted_files_.clear();
    new_files_.clear();
}

void VersionEdit::EncodeTo(std::string* dst) {

}

void VersionEdit::DecodeFrom(const Slice& src) {
    
}

}  // namespace leveldb