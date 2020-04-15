#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_

#include <atomic>
#include <cstdlib>
#include <cassert>

#include "util/arena.h"

namespace leveldb {

class Arena; 

template <typename Key, class Comparator>
class SkipList {
private:
    struct Node;

public:
    // create a new SkipList object that will use "cmp" for comparing keys,
    // and will allocate memory using "*arena". Objects allocated in the arena
    // must remain allocated for the lifetime of the skiplist object.
    explicit SkipList(Comparator cmp, Arena* arena);

    SkipList(SkipList&) = delete;
    SkipList& operator=(SkipList&) = delete;

    // Insert key into the list
    // REQUIRES: nothing that compares equal to key is currently in the list
    void Insert(const Key& key);

    bool Contains(const Key& key) const;

    class Iterator {
    public:
        // the return iterator is not valid.
        explicit Iterator(const SkipList* list);

        bool Valid() const;
        const Key& key() const;
        void Next();
        void Prev();
        // Advance to the first entry with a key >= target
        void Seek(const Key& target);
        // final state of iterator is Valid() iff list is not empty
        void SeekToFirst();
        // final state of iterator is Valid() iff list is not empty
        void SeekToLast();

    private:
        const SkipList* list_;
        Node* node_;
        // Intentionally copyable
    };

private:
    enum { kMaxHeight = 12 };

    inline int GetMaxHeight() const {
        return max_height_.load(std::memory_order_relaxed);
    }

    Node* NewNode(const Key& key, int height);
    int RandomHeight();
    bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

    // return true if the key is greater than the data store in n
    bool KeyIsAfterNode(const Key& key, Node* n) const;

    // return the earliest node that comes at or after key.
    // return nullptr if there is no such node
    //
    // if prev is non-null, fills prev[level] with pointer to previous
    // node at "level" for every level in [0 .. max_height_-1].
    Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

    // return the latest node with a key < key
    // return head_ if there is no such node
    Node* FindLessThan(const Key& key) const;

    // return head_ if list is empty
    Node* FindLast() const;

    Comparator const compare_;
    Arena* const arena_;

    Node* head_;

    std::atomic<int> max_height_;  // Height of the entire list
    Random rnd_;
};

// Implementation detail follow

/***** Node *****/
template <typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node {
    explicit Node(const Key& k) : key(k) {}
    Key const key;

    // accessors/mutators for links. Wrapped in methods so we can add 
    // the appropriate barriers as necessary.
    Node* Next(int n) {
        assert(n >= 0);
        // use an 'acquire load' so that we observe a fully initialized
        // version of the returned Node
        return next_[n].load(std::memory_order_acquire);
    }

    void SetNext(int n, Node* x) {
        assert(n >= 0);
        // use a 'release store' so that anybody who reads through this
        // pointer observes a fully initialized version of the inserted node.
        next_[n].store(x, std::memory_order_release);
    }

    Node* NoBarrier_Next(int n) {
        assert(n >= 0);
        return next_[n].load(std::memory_order_relaxed);
    }
    void NoBarrier_SetNext(int n, Node* x) {
        assert(n >= 0);
        next_[n].store(x, std::memory_order_relaxed);
    }

private:
    std::atomic<Node*> next_[1];
};

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key,Comparator>::NewNode(
        const Key& key, int height) {
    char* const node_memory = arena_->AllocateAligned(
            sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
    return new (node_memory) Node(key);
}

/***** Iterator *****/
template <typename Key, class Comparator>
inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list) {
    list_ = list;
    node_ = nullptr;
}

template <typename Key, class Comparator>
inline bool SkipList<Key, Comparator>::Iterator::Valid() const {
    return node_ != nullptr;
}

template <typename Key, class Comparator>
inline const Key& SkipList<Key, Comparator>::Iterator::key() const {
    assert(Valid());
    return node_->key;
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Next() {
    assert(Valid());
    node_ = node_->Next(0);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Prev() {
    assert(Valid());
    node_ = list_->FindLessThan(node_->key);
    if (node_ == list_->head_) {
        node_ = nullptr;
    }
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Seek(const Key& target) {
    // assert(Valid());  why no need ?
    node_ = list_->FindGreaterOrEqual(target, nullptr);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
    node_ = list_->head_->Next(0);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToLast() {
    node_ = list_->FindLast();
    if (node_ == list_->head_) {
        node_ = nullptr;
    }
}

/***** Others *****/
template <typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() {
    // increase height with probability 1 in kBranching
    static const unsigned int kBranching = 4;
    int height = 1;
    while (height < kMaxHeight && ((rnd_.Next() % kBranching) == 0)) {
        height++;
    }
    assert(height > 0);
    assert(height <= kMaxHeight);
    return height;
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
    // null n is considered infinite
    return (n != nullptr) && (compare_(n->key, key) < 0);
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key, Node** prev) const {
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        Node* next = x->Next(level);
        if (KeyIsAfterNode(key, next)) {
            // keep searching in this list
            x = next;
        } else {
            if (prev != nullptr) prev[level] = x;
            if (level == 0) {
                return next;
            } else {
                // switch to next list
                level--;
            }
        }
    }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindLessThan(const Key& key) const {
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        assert(x == head_ || compare_(x->key, key) < 0);
        Node* next = x->Next(level);
        if (next == nullptr || compare_(next->key, key) > 0) {
            if (level == 0) {
                return x;
            } else {
                level--;
            }
        } else {
            x = next;
        }
    }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindLast() const {
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        Node* next = x->Next(level);
        if (next == nullptr) {
            if (level == 0) {
                return x; 
            } else {
                level--;
            }
        } else {
            x = next;
        }
    }
}

template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena arena)
    : compare_(cmp),
      arena_(arena),
      head_(NewNode(0/* any key will do */, kMaxHeight)),
      max_height_(1),
      rnd_(0xdeadbeef) {
    for (int i = 0; i < kMaxHeight; i++) {
        head_->SetNext(i, nullptr);
    }
}

// INSERT
template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key) {
    Node* prev[kMaxHeight];
    Node* x = FindGreaterOrEqual(key, prev);

    // our data structure does not allow duplicate insertion
    assert(x == nullptr || !Equal(key, x->key));

    int height = RandomHeight();
    if (height > GetMaxHeight()) {
        for (int i = GetMaxHeight(); i < height; i++) {
            prev[i] = head_;
        }
        max_height_.store(height, std::memory_order_relaxed);
    }

    x = NewNode(key, height);
    for (int i = 0; i < height; i++) {
        x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
        prev[i]->SetNext(i, x);
    }
}

// CONTAINS
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const {
    Node* x = FindGreaterOrEqual(key, nullptr);
    if (x != nullptr && Equal(key, x->key)) {
        return true;
    } else {
        return false;
    }
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_SKIPLIST_H_
