#include "storage/data_block.hpp"

// size_t required_storage = DataBlock::getSizeInBytes<BlockHeader>(header);
using namespace duckdb;
using namespace std;

void DataBlock::Append(DataChunk &chunk) {
	char *dataptr = data_buffer.get();
	// write the data
	for (size_t col_idx = 0; col_idx < chunk.column_count; col_idx++) {
		auto &vec = chunk.data[col_idx];
		auto type = chunk.data[col_idx].type;
		if (TypeIsConstantSize(type)) {
			// constant size type: simple memcpy
			// VectorOperations::CopyToStorage(chunk.data[i], data_buffer, offset); TODO: Vector primitive for copy
			VectorOperations::CopyToStorage(vec, dataptr + offset);
			auto data_length = (GetTypeIdSize(type) * vec.count);
			offset += data_length;
		} else {
			assert(type == TypeId::VARCHAR);
			// strings are inlined into the block
			// we use null-padding to store them
			const char **strings = (const char **)vec.data;
			VectorOperations::Exec(vec, [&](size_t i, size_t k) {
				auto source = vec.nullmask[i] ? strings[k] : NullValue<const char *>();
				size_t str_length = strlen(source);
				memcpy(dataptr + offset, source, str_length);
				offset += str_length;
			});
		}
		// we need to store the offset for each column data inside the header
		header.data_offset.push_back(offset);
	}

	header.amount_of_tuples += chunk.size();
	// now we check whether we can still append data within this data block
	if (!HasSpace(offset, chunk.size())) {
		// we can't fit any more elements
		// then, we need to compute the data size
		header.data_size = offset - sizeof(DataBlockHeader);
		// and now we copy the header to our buffer
		memcpy(dataptr, &header, sizeof(DataBlockHeader));
		// finally we set the block as full to a create new block in the next iteration
		is_full = true;
	}
}

bool DataBlock::HasSpace(size_t offset, size_t chunk_size) {
	auto offset_next_chunk = offset + (header.tuple_size * STANDARD_VECTOR_SIZE);
	return (offset_next_chunk < max_block_size);
}

void DataBlock::FlushOnDisk(string &path_to_file, size_t block_id) {
	auto block_name = JoinPath(path_to_file, to_string(block_id) + ".duck");
	auto block_file = FstreamUtil::OpenFile(block_name, ios_base::out); // ios_base::binary | ios_base::out);
	block_file.write(data_buffer.get(), offset);
	FstreamUtil::CloseFile(block_file);
}

void DataBlock::ReadFromDisk(string &path_to_file) {
	auto block_file = FstreamUtil::OpenFile(path_to_file, ios_base::out); // ios_base::binary | ios_base::out);
	offset = sizeof(DataBlockHeader);
	block_file.read(data_buffer.get(), offset);

	FstreamUtil::CloseFile(block_file);
}

DataBlock DataBlock::Builder::Build(const size_t tuple_size) {
	// Checks input consistency and builds DataBlockHeader and the DataBlock itself if the DataBlock is buildable
	// from given information
	if (!tuple_size) {
		throw Exception("Cannot create block from empty tuple");
	}
	assert(block_count >= 0);
	header.tuple_size = tuple_size;
	header.block_id = block_count;
	header.amount_of_tuples = 0;
	header.data_size = 0;
	header.data_offset.push_back(sizeof(DataBlockHeader));
	block_count++;
	return DataBlock(header);
}
