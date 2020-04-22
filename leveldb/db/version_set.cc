#include "db/version_set.h"
#include "leveldb/status.h"

namespace leveldb {

// a helper 
class VersionSet::Builder {
public:

private:
};

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


}

}  // namespace leveldb