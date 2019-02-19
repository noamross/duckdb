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

void DirectoryBlockManager::Flush(unique_ptr<Block> &block) {
	assert(block->offset > 0);
	// the amount of data to be flushed
	auto buffered_size = block->offset;
	// delegates the writing to disk to block
	block->Write(data_buffer.get(), buffered_size);
	// calls deleter for the old buffer and reset it
	data_buffer.reset(new char[BLOCK_SIZE]);
}

void DirectoryBlockManager::AppendDataToBlock(DataChunk &chunk, unique_ptr<Block> &block,
                                              MetaBlockWriter &meta_writer) {
	char *dataptr = data_buffer.get();
	for (size_t col_idx = 0; col_idx < chunk.column_count; col_idx++) {
		auto &vec = chunk.data[col_idx];
		auto type = chunk.data[col_idx].type;
		if (TypeIsConstantSize(type)) {
			// constant size type: simple memcpy
			VectorOperations::CopyToStorage(vec, dataptr + block->offset);
			auto data_length = (GetTypeIdSize(type) * vec.count);
			block->offset += data_length;
		} else {
			assert(type == TypeId::VARCHAR);
			// strings are inlined into the block
			// we use null-padding to store them
			const char **strings = (const char **)vec.data;
			VectorOperations::Exec(vec, [&](size_t i, size_t k) {
				auto source = vec.nullmask[i] ? strings[k] : NullValue<const char *>();
				size_t str_length = strlen(source);
				memcpy(dataptr + block->offset, source, str_length);
				block->offset += str_length;
			});
		}
		// we need to store the offset for each column data inside the header
	}
}