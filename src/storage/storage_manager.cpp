#include "storage/storage_manager.hpp"

#include "catalog/catalog.hpp"
#include "catalog/catalog_entry/schema_catalog_entry.hpp"
#include "common/exception.hpp"
#include "common/file_system.hpp"
#include "common/fstream_util.hpp"
#include "common/serializer.hpp"
#include "common/types/hash.hpp"
#include "function/function.hpp"
#include "main/client_context.hpp"
#include "main/database.hpp"
#include "transaction/transaction_manager.hpp"

constexpr const char *DATABASE_INFO_FILE = "meta.info";
constexpr const char *DATABASE_TEMP_INFO_FILE = "meta.info.tmp";
constexpr const char *STORAGE_FILES[] = {"data-a", "data-b"};
constexpr const char *WAL_FILES[] = {"duckdb-a.wal", "duckdb-b.wal"};
constexpr const char *SCHEMA_FILE = "schemas.csv";
constexpr const char *TABLE_LIST_FILE = "tables.csv";
constexpr const char *TABLE_FILE = "tableinfo.duck";

using namespace duckdb;
using namespace std;

StorageManager::StorageManager(DuckDB &database, string path) : path(path), database(database), wal(database) {
}

void StorageManager::Initialize() {
	bool in_memory = path.empty() || path == ":memory:";

	// first initialize the base system catalogs
	// these are never written to the WAL
	auto transaction = database.transaction_manager.StartTransaction();

	// create the default schema
	CreateSchemaInformation info;
	info.schema = DEFAULT_SCHEMA;
	database.catalog.CreateSchema(*transaction, &info);

	// initialize default functions
	BuiltinFunctions::Initialize(*transaction, database.catalog);

	// commit transactions
	database.transaction_manager.CommitTransaction(transaction);

	if (!in_memory) {
		// create or load the database from disk, if not in-memory mode
		LoadDatabase();
	}
}

void StorageManager::LoadDatabase() {
	int iteration = 0;
	// first check if the database exists
	if (!DirectoryExists(path)) {
		// have to create the directory
		CreateDirectory(path);
	} else {
		// load from the main storage if it exists
		iteration = LoadFromStorage();
		// directory already exists
		// verify that it is an existing database
		string wal_path = JoinPath(path, WAL_FILES[iteration]);
		if (FileExists(wal_path)) {
			// replay the WAL
			wal.Replay(wal_path);
			// switch the iteration target to the other one
			iteration = 1 - iteration;
			// checkpoint the WAL
			CreatePersistentStorage(iteration);
		}
	}
	// initialize the WAL file
	string wal_path = JoinPath(path, WAL_FILES[iteration]);
	wal.Initialize(wal_path);
}

int StorageManager::LoadFromStorage() {
	ClientContext context(database);

	auto meta_info_path = JoinPath(path, DATABASE_INFO_FILE);
	// read the meta information, if there is any
	if (!FileExists(meta_info_path)) {
		// no meta file to read: skip the loading
		return 0;
	}

	context.transaction.BeginTransaction();
	// first read the meta information
	auto meta_info = FstreamUtil::OpenFile(meta_info_path, ios_base::in);
	int64_t storage_version;
	int iteration;

	meta_info >> storage_version;
	meta_info >> iteration;

	// now we can start by actually reading the files
	auto storage_path_base = JoinPath(path, STORAGE_FILES[iteration]);
	auto schema_path = JoinPath(storage_path_base, SCHEMA_FILE);

	// read the list of schemas
	auto schema_file = FstreamUtil::OpenFile(schema_path, ios_base::in);

	string schema_name;
	while (getline(schema_file, schema_name)) {
		// create the schema in the catalog
		CreateSchemaInformation info;
		info.schema = schema_name;
		info.if_not_exists = true;
		database.catalog.CreateSchema(context.ActiveTransaction(), &info);
		// now read the list of the tables belonging to this schema
		auto hashed_schema_name = to_string(HashStr(schema_name.c_str()));
		auto schema_directory_path = JoinPath(storage_path_base, hashed_schema_name);
		auto table_list_path = JoinPath(schema_directory_path, TABLE_LIST_FILE);

		// read the list of tables
		auto table_list_file = FstreamUtil::OpenFile(table_list_path, ios_base::in);
		string table_name;
		while (getline(table_list_file, table_name)) {
			// get the information of the table
			auto hashed_table_name = to_string(HashStr(table_name.c_str()));
			auto table_directory_path = JoinPath(schema_directory_path, hashed_table_name);
			auto table_meta_name = JoinPath(table_directory_path, TABLE_FILE);

			auto table_meta_file = FstreamUtil::OpenFile(table_meta_name, ios_base::binary | ios_base::in);
			auto result = FstreamUtil::ReadBinary(table_meta_file);

			// deserialize the CreateTableInformation
			auto table_meta_file_size = FstreamUtil::GetFileSize(table_meta_file);
			Deserializer source((uint8_t *)result.get(), table_meta_file_size);
			auto info = TableCatalogEntry::Deserialize(source);
			// create the table inside the catalog
			database.catalog.CreateTable(context.ActiveTransaction(), info.get());

			// Read the data_to_file information
			auto file_count = source.Read<size_t>();
			unordered_map<uint32_t, string> data_to_file;
			for (size_t i = 0; i != file_count; ++i) {
				data_to_file.insert(make_pair(source.Read<uint32_t>(), source.Read<string>()));
			}

			// now load the actual data
			auto *table = database.catalog.GetTable(context.ActiveTransaction(), info->schema, info->table);
			auto types = table->GetTypes();
			DataChunk chunk;
			chunk.Initialize(types);
			for (const auto &[key, value] : data_to_file) {
				cout << key << ": " << value << endl;
			}
			auto file_iterator = data_to_file.cbegin();
			// First, we create an object reponsible to create DataBlocks
			DataBlock::Builder builder;
			// Then, we build a Data block object using the table information
			DataBlock dataBlock = builder.Build(table->storage->tuple_size);
			size_t chunk_count = 1;
			while (true) {
				auto data_file = file_iterator->second;
				auto chunk_name = JoinPath(table_directory_path, data_file);
				if (!FileExists(chunk_name)) {
					break;
				}
				dataBlock.ReadFromDisk(chunk_name);
				auto chunk_file = FstreamUtil::OpenFile(chunk_name, ios_base::binary | ios_base::in);
				auto result = FstreamUtil::ReadBinary(chunk_file);

				// deserialize the chunk
				DataChunk insert_chunk;
				auto chunk_file_size = FstreamUtil::GetFileSize(chunk_file);
				Deserializer source((uint8_t *)result.get(), chunk_file_size);
				insert_chunk.Deserialize(source);
				// insert the chunk into the table
				table->storage->Append(*table, context, insert_chunk);
				chunk_count++;
			}
		}
	}
	context.transaction.Commit();
	return iteration;
}

void StorageManager::CreateCheckpoint(int iteration) {
	auto transaction = database.transaction_manager.StartTransaction();

	assert(iteration == 0 || iteration == 1);
	auto storage_path_base = JoinPath(path, STORAGE_FILES[iteration]);
	if (DirectoryExists(storage_path_base)) {
		// have a leftover directory
		// remove it
		RemoveDirectory(storage_path_base);
	}
	// create the directory
	CreateDirectory(storage_path_base);

	// first we have to access the schemas
	auto schema_path = JoinPath(storage_path_base, SCHEMA_FILE);

	auto schema_file = FstreamUtil::OpenFile(schema_path, ios_base::out);

	vector<SchemaCatalogEntry *> schemas;
	// scan the schemas and write them to the schemas.csv file
	database.catalog.schemas.Scan(*transaction, [&](CatalogEntry *entry) {
		schema_file << entry->name << '\n';
		schemas.push_back((SchemaCatalogEntry *)entry);
	});
	FstreamUtil::CloseFile(schema_file);

	// now for each schema create a directory
	for (auto &schema : schemas) {
		// FIXME: schemas can have unicode, do something for file systems, maybe hash?
		auto schema_directory_path = JoinPath(storage_path_base, schema->name);
		assert(!DirectoryExists(schema_directory_path));
		// create the directory
		CreateDirectory(schema_directory_path);
		// create the file holding the list of tables for the schema
		auto table_list_path = JoinPath(schema_directory_path, TABLE_LIST_FILE);
		auto table_list_file = FstreamUtil::OpenFile(table_list_path, ios_base::out);

		// create the list of tables for the schema
		vector<TableCatalogEntry *> tables;
		schema->tables.Scan(*transaction, [&](CatalogEntry *entry) {
			table_list_file << entry->name << '\n';
			tables.push_back((TableCatalogEntry *)entry);
		});
		FstreamUtil::CloseFile(table_list_file);

		// now for each table, write the column meta information and the actual data
		for (auto &table : tables) {
			// first create a directory for the table information
			// FIXME: same problem as schemas, unicode and file systems may not agree
			auto table_directory_path = JoinPath(schema_directory_path, table->name);
			assert(!DirectoryExists(table_directory_path));
			// create the directory
			CreateDirectory(table_directory_path);

			auto table_meta_name = JoinPath(table_directory_path, TABLE_FILE);
			auto table_file = FstreamUtil::OpenFile(table_meta_name, ios_base::binary | ios_base::out);

			// serialize the table information to a file
			Serializer serializer;
			table->Serialize(serializer);
			auto data = serializer.GetData();
			table_file.write((char *)data.data.get(), data.size);
			FstreamUtil::CloseFile(table_file);

			// now we have to write the actual binary
			// we do this by performing a scan of the table
			ScanStructure ss;
			table->storage->InitializeScan(ss);

			vector<column_t> column_ids;
			for (size_t i = 0; i < table->columns.size(); i++) {
				column_ids.push_back(i);
			}
			DataChunk chunk;
			auto types = table->GetTypes();
			chunk.Initialize(types);

			size_t chunk_count = 1;
			while (true) {
				chunk.Reset();
				table->storage->Scan(*transaction, chunk, column_ids, ss);
				if (chunk.size() == 0) {
					break;
				}
				auto chunk_name = JoinPath(table_directory_path, "chunk-" + to_string(chunk_count) + ".bin");
				auto chunk_file = FstreamUtil::OpenFile(chunk_name, ios_base::binary | ios_base::out);

				Serializer serializer;
				chunk.Serialize(serializer);
				auto data = serializer.GetData();
				chunk_file.write((char *)data.data.get(), data.size);
				FstreamUtil::CloseFile(chunk_file);

				chunk_count++;
			}
		}
	}
	// all the writes have been flushed and the entire database has been written
	// now we create the temporary meta information file
	auto meta_path = JoinPath(path, DATABASE_TEMP_INFO_FILE);
	auto meta_file = FstreamUtil::OpenFile(meta_path, ios_base::out);

	//! Write information to the meta file
	meta_file << STORAGE_VERSION << '\n';
	meta_file << iteration << '\n';
	FstreamUtil::CloseFile(meta_file);

	// now we move the meta information file over the old meta information file
	// this signifies a "completion" of the checkpoint
	auto permanent_meta_path = JoinPath(path, DATABASE_INFO_FILE);
	MoveFile(meta_path, permanent_meta_path);

	// we are now done writing
	// we can delete the directory for the other iteration because we do not need it anymore for consistency
	auto other_storage_path = JoinPath(path, STORAGE_FILES[1 - iteration]);
	auto other_wal_path = JoinPath(path, WAL_FILES[1 - iteration]);
	if (DirectoryExists(other_storage_path)) {
		RemoveDirectory(other_storage_path);
	}
	if (FileExists(other_wal_path)) {
		RemoveDirectory(other_wal_path);
	}
	transaction->Rollback();
}

void StorageManager::CreatePersistentStorage(int iteration) {
	auto transaction = database.transaction_manager.StartTransaction();
	assert(iteration == 0 || iteration == 1);
	auto storage_path_base = JoinPath(path, STORAGE_FILES[iteration]);
	if (DirectoryExists(storage_path_base)) {
		// have a leftover directory
		// remove it
		RemoveDirectory(storage_path_base);
	}
	// create the directory
	CreateDirectory(storage_path_base);

	// first we have to access the schemas
	auto schema_path = JoinPath(storage_path_base, SCHEMA_FILE);

	auto schema_file = FstreamUtil::OpenFile(schema_path, ios_base::out);

	vector<SchemaCatalogEntry *> schemas;
	// scan the schemas and write them to the schemas.csv file
	database.catalog.schemas.Scan(*transaction, [&](CatalogEntry *entry) {
		schema_file << entry->name << '\n';
		schemas.push_back((SchemaCatalogEntry *)entry);
	});
	FstreamUtil::CloseFile(schema_file);

	// now for each schema create a directory
	for (auto &schema : schemas) {
		auto hashed_schema_name = to_string(HashStr(schema->name.c_str()));
		auto schema_directory_path = JoinPath(storage_path_base, hashed_schema_name);
		assert(!DirectoryExists(schema_directory_path));
		// create the directory
		CreateDirectory(schema_directory_path);
		// create the file holding the list of tables for the schema
		auto table_list_path = JoinPath(schema_directory_path, TABLE_LIST_FILE);
		auto table_list_file = FstreamUtil::OpenFile(table_list_path, ios_base::out);

		// create the list of tables for the schema
		vector<TableCatalogEntry *> tables;
		schema->tables.Scan(*transaction, [&](CatalogEntry *entry) {
			table_list_file << entry->name << '\n';
			tables.push_back((TableCatalogEntry *)entry);
		});
		FstreamUtil::CloseFile(table_list_file);

		// now for each table, write the column meta information and the actual data
		for (auto &table : tables) {
			// first create a directory for the table information
			auto hashed_table_name = to_string(HashStr(table->name.c_str()));
			auto table_directory_path = JoinPath(schema_directory_path, hashed_table_name);
			assert(!DirectoryExists(table_directory_path));
			// create the directory
			CreateDirectory(table_directory_path);

			auto table_meta_name = JoinPath(table_directory_path, TABLE_FILE);

			// serialize the table information to a file
			Serializer serializer;
			table->Serialize(serializer);

			// now we have to write the actual binary
			// we do this by performing a scan of the table

			// First, we create an object reponsible to create DataBlocks
			DataBlock::Builder builder;
			// Then, we build a Data block object using the table information
			DataBlock dataBlock = builder.Build(table->storage->tuple_size);
			// now we initialize the scan
			ScanStructure ss;
			table->storage->InitializeScan(ss);
			// storing the column sizes
			vector<column_t> column_ids;
			for (size_t i = 0; i < table->columns.size(); i++) {
				column_ids.push_back(i);
			}
			// and column types
			DataChunk chunk;
			auto types = table->GetTypes();
			chunk.Initialize(types);
			size_t chunk_count = 1;
			unordered_map<uint32_t, string> data_to_file;
			// Then we iterate over the data to build the dataBlocks
			while (true) {
				chunk.Reset();
				// we scan the chunk
				table->storage->Scan(*transaction, chunk, column_ids, ss);
				if (chunk.size() == 0) {
					// chunk does not have data
					// When there is no more data to be stored we flush it to disk
					auto data_chunk = chunk.size() * builder.GetCurrentBlockId();
					auto data_file = to_string(builder.GetCurrentBlockId()) + ".duck";
					data_to_file.insert(make_pair(data_chunk, data_file));
					dataBlock.FlushOnDisk(table_directory_path, builder.GetCurrentBlockId());
					break;
				}

				// Now we can store the data chunk at a time
				if (!dataBlock.is_full) {
					// While DataBlock has space we append data
					dataBlock.Append(chunk);
				} else {
					// When the Data Block is full we flush it to disk
					auto data_chunk = chunk.size() * builder.GetCurrentBlockId();
					auto data_file = to_string(builder.GetCurrentBlockId()) + ".duck";
					data_to_file.insert(make_pair(data_chunk, data_file));
					dataBlock.FlushOnDisk(table_directory_path, builder.GetCurrentBlockId());
					// And create a new Data Block for the remaining data
					dataBlock = builder.Build(table->storage->tuple_size);
				}
				chunk_count++;
			}
			auto table_file = FstreamUtil::OpenFile(table_meta_name, ios_base::binary | ios_base::out);
			serializer.Write<size_t>(data_to_file.size());
			for (auto pair : data_to_file) {
				serializer.Write<uint32_t>(pair.first);
				serializer.WriteString(pair.second);
			}
			auto serilized_data = serializer.GetData();
			table_file.write((char *)serilized_data.data.get(), serilized_data.size);
			FstreamUtil::CloseFile(table_file);
			// TODO serilize data_to_file map inside tableinfo.duck
		}
	}
	// all the writes have been flushed and the entire database has been written
	// now we create the temporary meta information file
	auto meta_path = JoinPath(path, DATABASE_TEMP_INFO_FILE);
	auto meta_file = FstreamUtil::OpenFile(meta_path, ios_base::out);

	//! Write information to the meta file
	meta_file << STORAGE_VERSION << '\n';
	meta_file << iteration << '\n';
	FstreamUtil::CloseFile(meta_file);

	// now we move the meta information file over the old meta information file
	// this signifies a "completion" of the checkpoint
	auto permanent_meta_path = JoinPath(path, DATABASE_INFO_FILE);
	MoveFile(meta_path, permanent_meta_path);

	// we are now done writing
	// we can delete the directory for the other iteration because we do not need it anymore for consistency
	auto other_storage_path = JoinPath(path, STORAGE_FILES[1 - iteration]);
	auto other_wal_path = JoinPath(path, WAL_FILES[1 - iteration]);
	if (DirectoryExists(other_storage_path)) {
		RemoveDirectory(other_storage_path);
	}
	if (FileExists(other_wal_path)) {
		RemoveDirectory(other_wal_path);
	}
	transaction->Rollback();
}
