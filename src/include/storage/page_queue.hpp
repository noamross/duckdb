//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/page_queue.hpp
//
//
//===----------------------------------------------------------------------===//

#include "storage/page.hpp"

#pragma once

namespace duckdb {

using block_id_t = size_t;
constexpr size_t QUEUE_CAPACITY = 10;

struct Node {
	Node *next;
	Node *previous;
	Page *page_address;
};

class List {
public:
	List() : head(nullptr), tail(nullptr) {
	}
	void insert_entry(Node *entry) {
		//! check whether head is null or not. If head is null, assign the Node and exit.
		if (head == nullptr) {
			head = tail = entry;
			return;
		}
		//! walk the List until you find
		tail->next = entry;
		entry->previous = tail;
		tail = entry;
	}

	void remove_entry(Node entry) {
		//! The indirect address will point to the node we want to remove
		auto indirect = &head;
		//! we walk the list to find the entry which points to the one to be removed
		while ((*indirect) != entry) {
			indirect = &(*indirect)->next;
		}
		//! and we just remove it by making the previous one point to the next
		*indirect = entry->next;
	}

private:
	Node *head;
	Node *tail;
};

class PageQueue {
public:
	PageQueue() {
		current_position = 0;
		capacity = QUEUE_CAPACITY;
	}
	unordered_map<block_id_t, Node *> map_to_queue;
	List cooling_queue;
	void Insert(block_id_t page_identifier, Page *page);
	void Delete(block_id_t page_identifier);

private:
	static size_t current_position;
	static size_t capacity;
};
} // namespace duckdb
