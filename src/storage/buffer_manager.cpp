#include "storage/buffer_manager.hpp"

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
	map_to_queue[page_identifier] = ++current_position;
	//! then we add the page address to our queue
	cooling_queue.push_back(page);
}

BufferManager::BufferManager() {
}

void BufferManager::LoadPage(block_id_t page_identifier) {
}

Page *BufferManager::CreatePage(string &fname) {
	/*block_id_t id = current_counter++;
	block_mapping[id] = fname;*/
	return nullptr;
}

BufferManager::~BufferManager() {
}

BufferManager &BufferManager::GetInstance() {
	static BufferManager bm_instance;
	return bm_instance;
	/*if (bm_instance == nullptr) {
	    bm_instance = shared_ptr<BufferManager>(new BufferManager());
	}
	return bm_instance.get();*/
}
/*! Private constructor for Singleton Pattern
Page *CreatePage();

public:
void LoadPage(int64_t page_identifier);
//! Get instance of BufferManager -> Singleton Pattern
static BufferManager *GetInstance() {
    return
}
bool PageReplacement();
mutex loading_lock;
unordered_map<int64_t, Page *> loading_pages;
unordered_map<block_id_t, unique_ptr<char[]>> block_cache;
PageQueue cooling_queue;*/
