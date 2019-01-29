//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/buffer_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/buffer_frame.hpp"
#include "storage/page.hpp"

#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

namespace duckdb {

using block_id_t = size_t;
constexpr size_t QUEUE_CAPACITY = 10;
class PageQueue {
public:
	PageQueue() {
		position_queue = 0;
		capacity = QUEUE_CAPACITY;
	}
	unordered_map<block_id_t, size_t> hash_cooling;
	vector<Page *> cooling_queue;
	void Insert(block_id_t page_identifier, Page *page) {
		hash_cooling[page_identifier] = ++current_position;
		cooling_queue.push_back(page);
	}

	void Delete(block_id_t page_identifier) {
		position_on_queue cooling_queue.at() hash_cooling.erase(page_identifier);
	}

private:
	static size_t current_position;
	static size_t capacity;
}
//! BufferManager implemented as Singleton Pattern
class BufferManager {
private:
	size_t max_size;
	static size_t current_counter;
	//! Private constructor for Singleton Pattern
	BufferManager();
	Page *CreatePage(string &fname);
	//! Writes all dirty frames to disk and free all resources.
	~BufferManager();

public:
	std::shared_ptr<BufferManager> bm_instance;
	void LoadPage(block_id_t page_identifier);
	//! Get instance of BufferManager -> Singleton Pattern
	static BufferManager *GetInstance();
	bool PageReplacement();
	mutex loading_lock;
	unordered_map<block_id_t, Page *> loading_pages;
	unordered_map<block_id_t, Page *> loading_pages;
	PageQueue cooling_queue;
};
} // namespace duckdb
