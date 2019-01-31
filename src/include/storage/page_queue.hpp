//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/page_queue.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/page.hpp"

namespace duckdb {

using block_id_t = size_t;
constexpr const size_t QUEUE_CAPACITY = 10;

struct ListEntry {
	ListEntry *next;
	ListEntry *previous;
	Page *page_address;
};

class List {
public:
	List() : head(nullptr), tail(nullptr) {
	}
	void insert_entry(ListEntry *entry) {
		//! check whether head is null.
		if (head == nullptr) {
			//! The head is null. Just assign the Node and exit.
			head = tail = entry;
			return;
		}
		//! Include the new entry after the tail and update the tail.
		tail->next = entry;
		entry->previous = tail;
		tail = entry;
	}

	void remove_entry(ListEntry *entry) {
		//! The indirect address will point to the address of the node we want to remove
		auto indirect = &head;
		//! we walk the list to find the entry which points to the one to be removed
		while ((*indirect) != entry) {
			indirect = &(*indirect)->next;
		}
		//! and we just remove it by making the previous one point to the next
		*indirect = entry->next;
	}

private:
	ListEntry *head;
	ListEntry *tail;
};

class PageQueue {
public:
	PageQueue();
	void Insert(block_id_t page_identifier, Page *page);
	void Delete(block_id_t page_identifier);

private:
	unordered_map<block_id_t, ListEntry *> map_to_queue;
	List cooling_queue;
	constexpr static size_t current_position = 0;
	const static size_t capacity = QUEUE_CAPACITY;
};
} // namespace duckdb
