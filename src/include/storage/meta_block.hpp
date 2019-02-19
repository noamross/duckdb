//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/meta_block.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "storage/block.hpp"
#include "storage/block_manager.hpp"

namespace duckdb {

//! This is a structure to keep the location(Block) of a determined row in memory.
struct BlockEntry {
	uint64_t row_offset;
	unique_ptr<Block> block;
};

//! This struct is responsible for organizing the writing of the data within a block
struct MetaBlockWriter {
	MetaBlockWriter(BlockManager &manager) : manager(manager) {
		current_block = manager.CreateBlock();
		buffered_size = sizeof(block_id_t);
	}
	~MetaBlockWriter() {
		Flush();
	}

	BlockManager &manager;
	char buffer[BLOCK_SIZE];
	size_t buffered_size;
	unique_ptr<Block> current_block;

	void Flush() {
		if (buffered_size > 0) {
			current_block->Write(buffer, buffered_size);
		}
	}

	void Write(const char *data, size_t data_size) {
		while (buffered_size + data_size >= BLOCK_SIZE) {
			// we need to make a new block
			// first copy what we can
			size_t copy_amount = BLOCK_SIZE - buffered_size;
			assert(copy_amount < data_size);
			memcpy(buffer + buffered_size, data, copy_amount);
			data += copy_amount;
			data_size -= copy_amount;
			// now we need to get a new block
			auto new_block = manager.CreateBlock();
			memcpy(buffer, &new_block->id, sizeof(block_id_t));
			// first flush the old block
			Flush();
			// then replace the old block by the new block
			current_block = move(new_block);
			buffered_size = sizeof(block_id_t);
		}
		memcpy(buffer + buffered_size, data, data_size);
		buffered_size += data_size;
	}

	template <class T> void Write(T element) {
		Write((const char *)&element, sizeof(T));
	}

	void WriteString(string &str) {
		Write<uint32_t>(str.size());
		Write(str.c_str(), str.size());
	}
};
//! This struct is responsible for organizing the reading of the data from disk to blocks
struct MetaBlockReader {
	MetaBlockReader(unique_ptr<Block> bl) : block(move(bl)) {
		ReadNewBlock(*block);
	}
	unique_ptr<Block> block;
	char buffer[BLOCK_SIZE];
	size_t size;
	size_t pos;
	block_id_t next_block;

	void ReadNewBlock(Block &block) {
		size = block.Read(buffer);
		pos = sizeof(block_id_t);
		next_block = 0;
	}

	bool Finished() {
		// finished reading
		return pos == size && next_block == 0;
	}

	void Read(char *data, size_t data_size) {
		while (pos + data_size > size) {
			// read what we can from this block
			// move to the next block
			// read remainder there
			assert(0);
		}
		// we can just read from the stream
		memcpy(data, buffer + pos, data_size);
		pos += data_size;
	}

	template <class T> T Read() {
		T element;
		Read((char *)&element, sizeof(T));
		return element;
	}

	string ReadString() {
		uint32_t size = Read<uint32_t>();
		char buffer[size + 1];
		buffer[size + 1] = '\0';
		Read(buffer, size);
		return string(buffer, size);
	}
};
} // namespace duckdb
