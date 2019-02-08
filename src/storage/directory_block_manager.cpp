#include "storage/directory_block_manager.hpp"

using namespace std;
using namespace duckdb;

unique_ptr<Block> DirectoryBlockManager::GetBlock(block_id_t id) {
	// load an existing block
	// the file should exist, if it doesn't we throw an exception
	auto block_path = GetBlockPath(id);
	return unique_ptr<DirectoryBlock>(new DirectoryBlock(id, block_path));
}

unique_ptr<Block> DirectoryBlockManager::CreateBlock() {
	while (FileExists(GetBlockPath(free_id))) {
		free_id++;
	}
	auto block = unique_ptr<DirectoryBlock>(new DirectoryBlock(free_id, GetBlockPath(free_id)));
	free_id++;
	return move(block);
}

void DirectoryBlockManager::Flush(Block &block) {
}
