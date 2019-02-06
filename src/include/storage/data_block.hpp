//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/data_block.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "common/file_system.hpp"
#include "common/fstream_util.hpp"
#include "common/types/data_chunk.hpp"
#include "common/types/null_value.hpp"
#include "common/types/vector.hpp"
#include "common/vector_operations/vector_operations.hpp"
#include "storage/block_reference.hpp"

#include <stddef.h>
#include <string>

namespace duckdb {

class DataTable;
//! Block Header Size = 48 bytes
//! Max Block Size = 2^16 Bytes =  64kb
constexpr const size_t max_block_size = 1024 * 1024; // 1MB
//! Representation of a Page (content stored within a file). Each page holds data from multiuple columns in a PAX way
//! (https://dl.acm.org/citation.cfm?id=641271).  The Block stored in a data block Stores the header of each data block
struct DataBlockHeader {
	//! Id used to identify the block
	block_id_t block_id;
	//! Data size in bytes
	size_t data_size;
	//! Amount of tuples within the block
	size_t amount_of_tuples;
	//! size of tuples stored in this block in bytes
	size_t tuple_size;
	//! The offset for each column data
	vector<size_t> data_offset;
};

//! The DataBlock is the physical unit to store data. It has a physical block which is stored in a file with multiple
//! blocks
class DataBlock : public BlockReference {
public:
	//! This class constructs the Data Block
	class Builder;

private:
	DataBlockHeader header;
	//! The data of the block-> multiple columns (each column has the same type).
	unique_ptr<char[]> data_buffer;

	//! Only one simple constructor - rest is handled by Builder
	DataBlock(const DataBlockHeader created_header) : header(created_header), offset(sizeof(DataBlockHeader)){};

public:
	block_id_t GetId() const override {
		return header.block_id;
	};
	size_t GetSize() const override {
		return header.data_size;
	};
	void Append(DataChunk &chunk);
	void FlushToDisk(string &path_to_file, size_t block_id);
	void ReadFromDisk(string &path_to_file);
	bool HasSpace(size_t offset, size_t chunk_size);
	bool is_full = false;
	size_t offset;
};

//! The intent of the Builder design pattern is to separate the construction of a complex object from its representation
class DataBlock::Builder {
private:
	DataBlockHeader header;
	size_t block_count = 0;

public:
	//! Produces a new DataBlock
	DataBlock Build(const size_t tuple_size);
	inline size_t GetCurrentBlockId() const {
		return header.block_id;
	}
};

} // namespace duckdb
