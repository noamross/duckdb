#include "storage/page_queue.hpp"

#include "common/exception.hpp"

using namespace duckdb;
using namespace std;

void PageQueue::Delete(block_id_t page_identifier) {
	assert(page_identifier >= 0);
	//! check whether key is in the map
	if (map_to_queue.find(page_identifier) == map_to_queue.end()) {
		// key is not in the map
		throw Exception("Page does not exist!");
	}
	//! the key is in the map so we need the position within the queue
	auto position_on_queue = map_to_queue[page_identifier];
	//! then we set it to null
	cooling_queue[position_on_queue] = nullptr;
	// and erase the entry in the hash map
	map_to_queue.erase(page_identifier);
}

void PageQueue::Insert(block_id_t page_identifier, Page *page) {
	assert(page != nullptr && page_identifier >= 0);
	//! we create an entry using the page identifier as key and the position within the queue as value
	auto entry = unique_ptr<Node>;
	map_to_queue[page_identifier] = entry.get();
	//! then we add the page address to our queue
	cooling_queue.insert(entry);
}
