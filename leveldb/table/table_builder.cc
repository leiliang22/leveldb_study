#include "leveldb/table_builder.h"
#include "leveldb/options.h"

namespace leveldb {

struct TableBuilder::Rep {
    Rep(const Options& opt, WritableFile* f)
        : options(opt),
          index_block_options(opt),
          file(f),
          offset(0),
          data_block(&options),
          index_block(&options),
          num_entries(0),
          closed(false),
          filter_block(opt.filter_policy == nullptr
                       ? nullptr
                       : new FilterBlockBuilder(opt.filter_policy));
          pending_index_entry(false) {
        index_block_options.block_restart_interval = 1;
    }

    Options options;
    Options index_block_options;
    WritableFile* file;
    uint64_t offset;
    Status status;
    BlockBuilder data_block;
    BlockBuilder index_block;
    std::string last_key;
    int64_t num_entries;
    bool closed;
    FilterBlockBuilder* filter_block;

    // we don't emit the index entry for a block until we have seen the 
    // first key for the next data blcok. This allows us to use shorter
    // keys in the index block. for example, consider a block boundary 
    // between the keys "the quick brown fox" and "the who". we can use
    // "the r" as the key for the index block entry since it is >= all 
    // entries in the first block and < all entries in subsequent blocks
    //
    // Invariant: r->pending_index_entry is true only if data_block is empty.
    bool pending_index_entry;
    BlockHandle pending_handle;  // handle to add to index block

    std::string compressed_output;
};

TableBuilder::TableBuilder(const Options& options, WritableFile* file)
        : rep_(new Rep(options, file)) {
    if (rep_->filter_block != nullptr) {
        rep_->filter_block->StartBlock(0);
    }
}

TableBuilder::~TableBuilder() {
    assert(rep_->closed);  // catch errors where caller forgot to call Finish()
    delete rep_->filter_block;
    delete rep_;
}

Status TableBuilder::ChangeOptions(const Options& options) {
    return Status::OK();
}

void Add() {
    
}

}  // namespace leveldb