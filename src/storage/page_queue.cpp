#include "storage/page_queue.hpp"

#include "common/exception.hpp"

using namespace duckdb;
using namespace std;

PageQueue::PageQueue() {
}

void PageQueue::Delete(block_id_t page_identifier) {
	assert(page_identifier >= 0);
	//! check whether key is in the map
	if (map_to_queue.find(page_identifier) == map_to_queue.end()) {
		// key is not in the map
		throw Exception("Page does not exist!");
	}
	//! the key is in the map so we need the position within the queue
	auto entry_to_delete = map_to_queue[page_identifier];
	//! then we delete the entry
	cooling_queue.remove_entry(entry_to_delete);
	// and erase the reference in the hash map
	map_to_queue.erase(page_identifier);
}

void PageQueue::Insert(block_id_t page_identifier, Page *page) {
	assert(page != nullptr && page_identifier >= 0);
	//! we create a new entry which points to the page
	ListEntry *new_entry = nullptr;
	new_entry->page_address = page;
	//! and add it to the map
	map_to_queue[page_identifier] = new_entry;
	//! then we add the page address to our queue
	cooling_queue.insert_entry(new_entry);
}
