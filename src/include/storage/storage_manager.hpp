//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/storage_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/helper.hpp"
#include "storage/data_block.hpp"
#include "storage/data_table.hpp"
#include "storage/directory_block_manager.hpp"
#include "storage/meta_block_writer.hpp"
#include "storage/write_ahead_log.hpp"

namespace duckdb {

constexpr const int64_t STORAGE_VERSION = 1;

class Catalog;
class DuckDB;
class TransactionManager;

//! StorageManager is responsible for managing the physical storage of the
//! database on disk
class StorageManager {
public:
	StorageManager(DuckDB &database, const string &path);
	//! Initialize a database or load an existing database from the given path
	void Initialize();
	//! Get the WAL of the StorageManager, returns nullptr if in-memory
	WriteAheadLog *GetWriteAheadLog() {
		return wal.IsInitialized() ? &wal : nullptr;
	}
	//! The BlockManager to read/store meta information and data in blocks
	unique_ptr<BlockManager> block_manager;

private:
	//! Load the database from a directory
	void LoadDatabase();
	//! Load the initial database from the main storage (without WAL). Returns which alternate storage to write to.
	int LoadFromStorage();
	//! Checkpoint the current state of the WAL and flush it to the main storage. This should be called BEFORE any
	//! connection is available because right now the checkpointing cannot be done online. (TODO)
	void CreateCheckpoint();
	void LoadCheckpoint();
	//! Creates and stores the data blocks for physical storage
	// void CreatePersistentStorage(int iteration);
	// void CreatePersistentStorage_(int iteration);

	//! The path of the database
	string database_path;
	//! The database this storagemanager belongs to
	DuckDB &database;
	//! The WriteAheadLog of the storage manager
	WriteAheadLog wal;
};

} // namespace duckdb
