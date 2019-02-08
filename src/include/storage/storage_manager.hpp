//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/storage_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/helper.hpp"
#include "storage/block_manager.hpp"
#include "storage/data_block.hpp"
#include "storage/data_table.hpp"
#include "storage/write_ahead_log.hpp"

namespace duckdb {

constexpr const int64_t STORAGE_VERSION = 1;
// Size of a memory slot managed by the StorageManager. This is the quantum of allocation for DataBlocks. 1 MB is the
// large page size on x86.
constexpr const size_t DATA_BLOCK_SIZE = 0x100000;
// The storage works as tree where the root node should be fetched easily. Therefore the root size is 4k which is the
// basic hardware page size
constexpr const size_t ROOT_BLOCK_SIZE = 0x1000;

class Catalog;
class DuckDB;
class TransactionManager;

//! StorageManager is responsible for managing the physical storage of the
//! database on disk
class StorageManager {
public:
	StorageManager(DuckDB &database, string path);
	//! Initialize a database or load an existing database from the given path
	void Initialize();
	//! Get the WAL of the StorageManager, returns nullptr if in-memory
	WriteAheadLog *GetWriteAheadLog() {
		return wal.IsInitialized() ? &wal : nullptr;
	}

private:
	//! Load the database from a directory
	void LoadDatabase();
	//! Load the initial database from the main storage (without WAL). Returns which alternate storage to write to.
	int LoadFromStorage();
	//! Checkpoint the current state of the WAL and flush it to the main storage. This should be called BEFORE any
	//! connection is available because right now the checkpointing cannot be done online. (TODO)
	void CreateCheckpoint(int iteration);
	//! Creates and stores the data blocks for physical storage
	void CreatePersistentStorage(int iteration);
	void CreatePersistentStorage_(int iteration);

	//! The path of the database
	string path;
	//! The database this storagemanager belongs to
	DuckDB &database;
	//! The WriteAheadLog of the storage manager
	WriteAheadLog wal;
	//! The BlockManager to read/store meta information and data in blocks
	unique_ptr<BlockManager> block_manager;
	//! Maps row offsets to blocks TODO: We are gonna move this to the buffer manager
	vector<BlockEntry> rows_to_block;
};

} // namespace duckdb
