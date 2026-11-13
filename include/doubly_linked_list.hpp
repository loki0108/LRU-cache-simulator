#pragma once

#include <cstddef>
#include <utility>
#include <vector>

// A doubly linked list that keeps items in "recency order":
//   front (right after head_) = most recently used
//   back  (right before tail_) = least recently used
//
// head_ and tail_ are sentinel (dummy) nodes. They never hold real data,
// they just guarantee every real node always has a prev and a next,
// so insert/remove never need special cases for "empty list" or "first node".
template <typename K, typename V>
class DoublyLinkedList {
public:
    struct Node {
        K key;     // stored so that on eviction we know which map entry to erase
        V value;
        Node* prev = nullptr;
        Node* next = nullptr;

        Node(K k, V v) : key(std::move(k)), value(std::move(v)) {}
    };

    DoublyLinkedList() : head_(new Node(K{}, V{})), tail_(new Node(K{}, V{})) {
        head_->next = tail_;
        tail_->prev = head_;
    }

    ~DoublyLinkedList() {
        Node* cur = head_;
        while (cur != nullptr) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
    }

    // The list owns raw pointers, so copying it would make two lists
    // delete the same nodes. Disable copying (rule of three).
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    // O(1): create a node and put it at the front (most recently used).
    Node* push_front(K key, V value) {
        Node* node = new Node(std::move(key), std::move(value));
        link_after_head(node);
        ++size_;
        return node;  // caller (the cache) keeps this pointer in its hash map
    }

    // O(1): an existing node was just used, so move it to the front.
    void move_to_front(Node* node) {
        unlink(node);
        link_after_head(node);
    }

    // O(1): remove the least recently used node and return its key.
    // Precondition: list is not empty.
    K pop_back() {
        Node* lru = tail_->prev;
        K key = std::move(lru->key);
        unlink(lru);
        delete lru;
        --size_;
        return key;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // Keys from most to least recently used. O(n), only for debugging/tests.
    std::vector<K> keys() const {
        std::vector<K> out;
        out.reserve(size_);
        for (Node* cur = head_->next; cur != tail_; cur = cur->next) {
            out.push_back(cur->key);
        }
        return out;
    }

private:
    // Detach node from its neighbours (does not free it).
    void unlink(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    // Insert node between head_ and the current first node.
    void link_after_head(Node* node) {
        node->prev = head_;
        node->next = head_->next;
        head_->next->prev = node;
        head_->next = node;
    }

    Node* head_;
    Node* tail_;
    std::size_t size_ = 0;
};