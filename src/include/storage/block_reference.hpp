//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/block_reference.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"

namespace duckdb {

using block_id_t = size_t;
//! Blockreference is an abstract representation of block on duckdb, it may have different concrete implementations.
class BlockReference {
public:
	virtual block_id_t GetId() const = 0;
	virtual size_t GetSize() const = 0;
	BlockReference *next_block{nullptr};
};
} // namespace duckdb
