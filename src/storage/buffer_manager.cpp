#include "storage/buffer_manager.hpp"

using namespace duckdb;
using namespace std;

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
