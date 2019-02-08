#include "storage/directory_block"

using namespace std;
using namespace duckdb;

void DirectoryBlock::Write(char *buffer, size_t count) {
	assert(count <= BLOCK_SIZE);
	ofstream ofs(path.c_str(), ios::binary);
	ofs.write(buffer, count);
	ofs.close();
}

size_t DirectoryBlock::Read(char *buffer) {
	ifstream file(path.c_str(), ios::binary | ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, ios::beg);
	assert(size <= BLOCK_SIZE);
	file.read(buffer, size);
	return size;
}

void DirectoryBlock::Read(char *buffer, size_t offset, size_t count) {
	ifstream file(path.c_str(), ios::binary);
	file.seekg(offset, ios::beg);
	file.read(buffer, count);
	assert(file.good());
}
