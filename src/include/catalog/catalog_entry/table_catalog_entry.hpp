//===----------------------------------------------------------------------===//
//                         DuckDB
//
// catalog/catalog_entry/table_catalog_entry.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "catalog/catalog_entry.hpp"
#include "common/types/statistics.hpp"
#include "common/unordered_map.hpp"
#include "parser/column_definition.hpp"
#include "parser/constraint.hpp"
#include "parser/parsed_data.hpp"

namespace duckdb {

class ColumnStatistics;
class DataTable;
class SchemaCatalogEntry;

//! A table catalog entry
class TableCatalogEntry : public CatalogEntry {
public:
	//! Create a real TableCatalogEntry and initialize storage for it
	TableCatalogEntry(Catalog *catalog, SchemaCatalogEntry *schema, CreateTableInformation *info);
	TableCatalogEntry(Catalog *catalog, SchemaCatalogEntry *schema, CreateTableInformation *info,
	                  std::shared_ptr<DataTable> storage);

	//! The schema the table belongs to
	SchemaCatalogEntry *schema;
	//! A reference to the underlying storage unit used for this table
	std::shared_ptr<DataTable> storage;
	//! A list of columns that are part of this table
	vector<ColumnDefinition> columns;
	//! A list of constraints that are part of this table
	vector<unique_ptr<Constraint>> constraints;
	//! A map of column name to column index
	unordered_map<string, column_t> name_map;

	unique_ptr<CatalogEntry> AlterEntry(AlterInformation *info);
	//! Returns whether or not a column with the given name exists
	bool ColumnExists(const string &name);
	//! Returns the statistics of the oid-th column. Throws an exception if the
	//! access is out of range.
	ColumnStatistics &GetStatistics(column_t oid);
	//! Returns a reference to the column of the specified name. Throws an
	//! exception if the column does not exist.
	ColumnDefinition &GetColumn(const string &name);
	//! Returns a list of types of the table
	vector<TypeId> GetTypes();
	//! Returns a list of types of the specified columns of the table
	vector<TypeId> GetTypes(const vector<column_t> &column_ids);

	//! Serialize the meta information of the TableCatalogEntry a serializer
	virtual void Serialize(Serializer &serializer);
	//! Deserializes to a CreateTableInfo
	static unique_ptr<CreateTableInformation> Deserialize(Deserializer &source);

	//! Returns true if other objects depend on this object
	virtual bool HasDependents(Transaction &transaction);
	//! Function that drops all dependents (used for Cascade)
	virtual void DropDependents(Transaction &transaction);

private:
	void Initialize(CreateTableInformation *info);
};
} // namespace duckdb
