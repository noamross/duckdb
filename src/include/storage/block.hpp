//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/block.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"

namespace duckdb {

using block_id_t = size_t;
using version_t = size_t;
//! Block is an abstract representation for memory chunks on duckdb, it may have different concrete implementations.
class Block {
public:
	Block(block_id_t id) : id(id) {
	}
	virtual ~Block() {
	}

	//! Writes new contents to the block
	virtual void Write(char *buffer, size_t count) = 0;
	//! Read the entire contents of the block into a buffer
	virtual size_t Read(char *buffer) = 0;
	//! Read a set amount of bytes from a block into a buffer
	virtual void Read(char *buffer, size_t offset, size_t count) = 0;

	block_id_t id;
};
} // namespace duckdb
