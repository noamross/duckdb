//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/buffer_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "storage/page.hpp"
#include "storage/page_queue.hpp"

namespace duckdb {

using block_id_t = size_t;

//! BufferManager implemented as Singleton Pattern
class BufferManager {
public:
	~BufferManager();
	//! Get instance of BufferManager -> Singleton Pattern
	static BufferManager &GetInstance();
	void LoadPage(block_id_t page_identifier);
	bool PageReplacement();
	mutex loading_lock;
	unordered_map<block_id_t, Page *> loading_pages;
	PageQueue cooling_queue;

private:
	size_t max_size;
	static size_t current_counter;
	//! Private constructor for Singleton Pattern
	BufferManager();
	Page *CreatePage(string &fname);
	//! Writes all dirty frames to disk and free all resources.
};
} // namespace duckdb
