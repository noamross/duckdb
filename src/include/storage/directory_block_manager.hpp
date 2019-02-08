//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/directory_block_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "storage/block.hpp"
#include "storage/block_manager.hpp"

namespace duckdb {

//! DirectoryBlockManager is a implementation for a BlockManager which manages blocks from multiple file
class DirectoryBlockManager : public BlockManager {
public:
	//! Constructor gets the path to read/store the block
	DirectoryBlockManager(const string &path) : path(path), free_id(0) {
	}
	//! Returns a pointer to the block of the given id
	unique_ptr<Block> GetBlock(block_id_t id) override;
	//! Creates a new Block and returns a pointer
	unique_ptr<Block> CreateBlock() override;
	//! Flushes the given block to disk, in this case we use the path and stores this block on its file.
	void Flush(Block &block) override;
	//! Gets the path to the block of the givend id
	string GetBlockPath(block_id_t id) {
		return path + "/" + to_string(id) + ".duck";
	}

private:
	//! The directory where blocks are stored
	const string path;
	//! Keeps track of the next avalible id for a new block
	block_id_t free_id;
};
} // namespace duckdb
