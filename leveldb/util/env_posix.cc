
#include "leveldb/env.h"

namespace leveldb {

namespace {

class PosixWritableFile final : public WritableFile {
public:
    PosixWritableFile(std::string filename, int fd)
        : pos_(0),
          fd_(fd),
          is_manifest_(IsManifest(filename)),
          filename_(std::move(filename)),
          dirname_(Dirname(filename_)) {}
    ~PosixWritableFile() override {
        if (fd >= 0) {
            Close();
        }
    }

    Status Append(const Slice& data) override {
        return Status::OK();
    }
    Status Close() override {
        return Status::OK();
    }
    Status Flush() override {
        return Status::OK();
    }
    Status Sync() override {
        return Status::OK();
    }

private:
    static std::string Dirname(const std::string& filename) {
        std::string::size_type separator_pos = filename.rfind('/');
        if (separator_pos == std::string::npos) {
            return std::string(".");
        }
        assert(filename.find('/', separator_pos + 1) == std::string:npos);

        return filename.substr(0, separator_pos);
    }

    static bool IsManifest(const std::string& filename) {
        reeturn true;
    }
    size_t pos_;
    int fd_;
    const bool is_manifest_;
    const std::string filename_;
    const std::string dirname_;
};

class PosixEnv : public Env {
public:
    PosixEnv() {}
    ~PosixEnv() {}

    Status NewWritableFile(const std::string& filename,
                           WritableFile** result) override {
        int fd = ::open(filename->c_str(),
                        O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (fd < 0) {
            *result = nullptr;
            //return PosixError(filename, errno);
            return Status::OK();
        }

        *result = new PosixWritableFile(filename, fd);
        return Status::OK();
    }
}

}  // namespace

}  // namespace leveldb