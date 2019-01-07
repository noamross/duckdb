#include "storage/data_block.hpp"

// size_t required_storage = DataBlock::getSizeInBytes<BlockHeader>(header);
using namespace duckdb;
using namespace std;

void DataBlock::Append(DataChunk &chunk) {
	char *dataptr = data_buffer.get();
	// write the data
	for (size_t i = 0; i < chunk.column_count; i++) {
		auto type = chunk.data[i].type;
		if (TypeIsConstantSize(type)) {
			// constant size type: simple memcpy
			// VectorOperations::CopyToStorage(chunk.data[i], data_buffer, offset); TODO: Vector primitive for copy
			auto data_length = (GetTypeIdSize(type) * chunk.size());
			memcpy(dataptr + offset, &chunk.data[i].data, data_length);
			offset += data_length;
		} else {
			assert(type == TypeId::VARCHAR);
			// strings are inlined into the block
			// we use null-padding to store them
			const char **strings = (const char **)chunk.data[i].data;
			for (size_t j = 0; j < chunk.size(); j++) {
				auto source = strings[j] ? strings[j] : NullValue<const char *>();
				string str(source);
				memcpy(dataptr + offset, &source[0], str.size());
				offset += str.size();
			}
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
	auto offset_next_chunk = offset + (header.tuple_size * chunk_size);
	return (offset_next_chunk < max_block_size);
}

void DataBlock::FlushOnDisk(string &path_to_file, size_t block_id) {
	auto block_name = JoinPath(path_to_file, to_string(block_id) + ".duck");
	auto block_file = FstreamUtil::OpenFile(block_name, ios_base::out); // ios_base::binary | ios_base::out);
	block_file.write(data_buffer.get(), offset);
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
