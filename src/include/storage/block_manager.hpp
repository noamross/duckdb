//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/block_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "storage/block.hpp"

namespace duckdb {

struct BlockEntry {
	uint64_t row_offset;
	unique_ptr<Block> block;
};

//! BlockManager is an abstract representation to manage blocks on duckDB. When writing or reading blocks, the
//! blockManager creates and accessess blocks. The concrete types implements how blocks are stored.
class BlockManager {
public:
	virtual ~BlockManager() {
	}
	//! Fetches an existing block by its ID
	virtual unique_ptr<Block> GetBlock(block_id_t id) = 0;
	//! Creates a new block inside the block manager
	virtual unique_ptr<Block> CreateBlock() = 0;

	//! Flushes a block to disk
	virtual void Flush(Block &block) = 0;
};
} // namespace duckdb
