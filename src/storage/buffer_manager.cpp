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
}
