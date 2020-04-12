#include "leveldb/env.h"
#include "util/logging.h"

namespace leveldb {

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

}  // namespace leveldb