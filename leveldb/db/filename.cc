#include "leveldb/env.h"
#include "util/logging.h"

namespace leveldb {

// a utility routine: write "data" to the named file and Sync() it.
Status WriteStringToFileSync(Env* env, const Slice& data, const std::string& fname);

static std::string MakeFileName(const std::string& dbname, uint64_t number
                                const char* suffix) {
    char buf[100];
    snprintf(buf, sizeof(buf), "/%06llu.%s",
             static_cast<unsigned long long>(number), suffix);
    return dbname + buf;
}

std::string LogFileName(const std::string& dbname, uint64_t number) {
    assert(number > 0);
    return MakeFileName(dbname, number, "log");
}

std::string DescriptorFileName(std::string& dbname, uint64_t number) {
    assert(number > 0);
    char buf[100];
    snprintf(buf, sizeof(buf), "/MANIFEST-%06llu",
             static_cast<unsigned long long>(number));
    return dbname + buf;
}

std::string TempFileName(const std::string& dbname, uint64_t number) {
    assert(number > 0);
    return MakeFileName(dbname, number, "dbtmp");
}

std::string CurrentFileName(const std::string& dbname) {
    return dbname + "/CURRENT";
}

Status SetCurrentFile(Env* env, const std::string& dbname, uint64_t descriptor_number) {
    // 
    std::string manifest = DescriptorFileName(dbname, descriptor_number);
    Slice contents = manifest;
    assert(contents.start_with(dbname + '/'));
    contents.remove_prefix(dbname.size() + 1);
    std::string tmp = TempFileName(dbname, descriptor_number);
    Status s = WriteStringToFileSync(env, contents.ToString() + "\n", tmp);
    if (s.ok()) {
        s = env->RenameFile(tmp, CurrentFileName(dbname));
    }
    if (!s.ok()) {
        env->RemoveFile(tmp);
    }
    return s;
}

}  // namespace leveldb