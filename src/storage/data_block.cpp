#include "storage/data_block.hpp"

// size_t required_storage = DataBlock::getSizeInBytes<BlockHeader>(header);
using namespace duckdb;
using namespace std;

void DataBlock::Append(char *buffer, DataChunk &chunk) {
	int i = 0;
	// write the data
	for (size_t i = 0; i < chunk.column_count; i++) {
		auto type = chunk.data[i].type;
		if (TypeIsConstantSize(type)) {
			// constant size type: simple memcpy
			VectorOperations::CopyToStorage(chunk.data[i], buffer, offset);
		} else {
			assert(type == TypeId::VARCHAR);
			// strings are inlined into the block
			// we use null-padding to store them
			const char **strings = (const char **)chunk.data[i].data;
			for (size_t j = 0; j < chunk.size(); j++) {
				auto source = strings[j] ? strings[j] : NullValue<const char *>();
				string str(source);
				memcpy(buffer + offset, &source[0], str.size());
				offset += str.size();
			}
		}
		offset += sizeof(type);
	}

	header.amount_of_tuples += chunk.size();
	// now we check whether we can still append data within this data block
	if (offset + (sizeof(int) * chunk.size()) >= max_block_size) {
		// we can't fit any more elements
		// first write the header
		header.data_size = offset - sizeof(DataBlockHeader);
		memcpy(buffer, &header, sizeof(DataBlockHeader));

		// create new block
		is_full = true;
	}
}

void DataBlock::FlushOnDisk(char *buffer, string &path_to_file, size_t block_id) {

	auto block_name = JoinPath(path_to_file, to_string(block_id) + ".duck");
	auto block_file = FstreamUtil::OpenFile(block_name, ios_base::binary | ios_base::out);
	block_file.write(buffer, offset);
	FstreamUtil::CloseFile(block_file);
}

DataBlock DataBlock::Builder::Build(const size_t tuple_size) {
	// Checks input consistency and builds DataBlockHeader and the DataBlock itself if the DataBlock is buildable
	// from given information
	header.block_id = block_count;
	header.amount_of_tuples = 0;
	header.data_size = tuple_size * header.amount_of_tuples;
	header.data_offset.push_back(sizeof(DataBlockHeader));
	block_count++;
	return DataBlock(header);
}
