#include <iostream>
#include <string>
#include <vector>

#include "doubly_linked_list.hpp"

// Print keys from most recently used (left) to least recently used (right).
void print(const std::string& label, const std::vector<int>& keys) {
    std::cout << label << ": [ ";
    for (int k : keys) std::cout << k << ' ';
    std::cout << "]  (MRU -> LRU)\n";
}

int main() {
    DoublyLinkedList<int, std::string> list;

    auto* n1 = list.push_front(1, "one");
    list.push_front(2, "two");
    list.push_front(3, "three");
    print("after pushing 1, 2, 3", list.keys());

    list.move_to_front(n1);
    print("after using 1        ", list.keys());

    int evicted = list.pop_back();
    print("after pop_back       ", list.keys());
    std::cout << "evicted key: " << evicted << ", size now: " << list.size() << '\n';

    return 0;
}