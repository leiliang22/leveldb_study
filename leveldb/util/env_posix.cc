
#include "leveldb/env.h"

namespace leveldb {

namespace {

class PosixEnv : public Env {
public:
    PosixEnv();
    ~PosixEnv();

    Status NewWritableFile(const std::string& filename,
                           WritableFile** result) override {
        int fd = ::open(filename->c_str(),
                        O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (fd < 0) {
            *result = nullptr;
            return PosixError(filename, errno);
        }

        *result = new PosixWritableFile(filename, fd);
        return Status::OK();
    }
}

}  // namespace

}  // namespace leveldb