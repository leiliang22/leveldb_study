#include "include/leveldb/db.h"
#include <iostream>
#include <string.h>
#include <assert.h>

void test_leveldb() {
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options,"/tmp/test", &db)
    assert(status.ok());

    std::string key="leil";
    std::string value = "666";

    status = db->Put(leveldb::WriteOptions(), key,value);
    assert(status.ok());

    status = db->Get(leveldb::ReadOptions(), key, &value);
    assert(status.ok());

    std::cout << key <<"'s value is:" << value << std::endl;
    delete db;
}

int main() {
    std::cout << "Hello levelDB !" << std::endl;
    //test_leveldb();
    return 0;
}
