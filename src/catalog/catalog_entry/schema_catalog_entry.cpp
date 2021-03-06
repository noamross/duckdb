#include "catalog/catalog_entry/schema_catalog_entry.hpp"

#include "catalog/catalog.hpp"
#include "catalog/catalog_entry/index_catalog_entry.hpp"
#include "common/exception.hpp"
#include "parser/expression/function_expression.hpp"

#include <algorithm>

using namespace duckdb;
using namespace std;

SchemaCatalogEntry::SchemaCatalogEntry(Catalog *catalog, string name)
    : CatalogEntry(CatalogType::SCHEMA, catalog, name) {
}

void SchemaCatalogEntry::CreateTable(Transaction &transaction, CreateTableInformation *info) {
	auto table = make_unique_base<CatalogEntry, TableCatalogEntry>(catalog, this, info);
	if (!tables.CreateEntry(transaction, info->table, move(table))) {
		if (!info->if_not_exists) {
			throw CatalogException("Table or view with name \"%s\" already exists!", info->table.c_str());
		}
	}
}

void SchemaCatalogEntry::CreateView(Transaction &transaction, CreateViewInformation *info) {
	auto view = make_unique_base<CatalogEntry, ViewCatalogEntry>(catalog, this, info);
	auto old_view = tables.GetEntry(transaction, info->view_name);
	if (info->replace && old_view) {
		if (old_view->type != CatalogType::VIEW) {
			throw CatalogException("Existing object %s is not a view", info->view_name.c_str());
		}
		tables.DropEntry(transaction, info->view_name, false);
	}

	if (!tables.CreateEntry(transaction, info->view_name, move(view))) {
		throw CatalogException("T with name \"%s\" already exists!", info->view_name.c_str());
	}
}

void SchemaCatalogEntry::DropView(Transaction &transaction, DropViewInformation *info) {
	auto existing_view = tables.GetEntry(transaction, info->view_name);
	if (existing_view && existing_view->type != CatalogType::VIEW) {
		throw CatalogException("Existing object %s is not a view", info->view_name.c_str());
	}
	if (!tables.DropEntry(transaction, info->view_name, false)) {
		if (!info->if_exists) {
			throw CatalogException("View with name \"%s\" does not exist!", info->view_name.c_str());
		}
	}
}

bool SchemaCatalogEntry::CreateIndex(Transaction &transaction, CreateIndexInformation *info) {
	auto index = make_unique_base<CatalogEntry, IndexCatalogEntry>(catalog, this, info);
	if (!indexes.CreateEntry(transaction, info->index_name, move(index))) {
		if (!info->if_not_exists) {
			throw CatalogException("Index with name \"%s\" already exists!", info->index_name.c_str());
		}
		return false;
	}
	return true;
}

void SchemaCatalogEntry::DropIndex(Transaction &transaction, DropIndexInformation *info) {
	if (!indexes.DropEntry(transaction, info->name, false)) {
		if (!info->if_exists) {
			throw CatalogException("Index with name \"%s\" does not exist!", info->name.c_str());
		}
	}
}

void SchemaCatalogEntry::DropTable(Transaction &transaction, DropTableInformation *info) {
	auto old_table = tables.GetEntry(transaction, info->table);
	if (info->if_exists && old_table) {
		if (old_table->type != CatalogType::TABLE) {
			throw CatalogException("Existing object %s is not a table", info->table.c_str());
		}
	}
	if (!tables.DropEntry(transaction, info->table, info->cascade)) {
		if (!info->if_exists) {
			throw CatalogException("Table with name \"%s\" does not exist!", info->table.c_str());
		}
	}
}

void SchemaCatalogEntry::AlterTable(Transaction &transaction, AlterTableInformation *info) {
	if (!tables.AlterEntry(transaction, info->table, info)) {
		throw CatalogException("Table with name \"%s\" does not exist!", info->table.c_str());
	}
}

TableCatalogEntry *SchemaCatalogEntry::GetTable(Transaction &transaction, const string &table_name) {
	auto table_or_view = GetTableOrView(transaction, table_name);
	if (table_or_view->type != CatalogType::TABLE) {
		throw CatalogException("%s is not a table", table_name.c_str());
	}
	return (TableCatalogEntry *)table_or_view;
}

CatalogEntry *SchemaCatalogEntry::GetTableOrView(Transaction &transaction, const string &table_name) {
	auto entry = tables.GetEntry(transaction, table_name);
	if (!entry) {
		throw CatalogException("Table or view with name %s does not exist!", table_name.c_str());
	}
	return entry;
}

TableFunctionCatalogEntry *SchemaCatalogEntry::GetTableFunction(Transaction &transaction,
                                                                FunctionExpression *expression) {
	auto entry = table_functions.GetEntry(transaction, expression->function_name);
	if (!entry) {
		throw CatalogException("Table Function with name %s does not exist!", expression->function_name.c_str());
	}
	auto function_entry = (TableFunctionCatalogEntry *)entry;
	// check if the argument lengths match
	if (expression->children.size() != function_entry->arguments.size()) {
		throw CatalogException("Function with name %s exists, but argument length does not match! "
		                       "Expected %d arguments but got %d.",
		                       expression->function_name.c_str(), (int)function_entry->arguments.size(),
		                       (int)expression->children.size());
	}
	return function_entry;
}

void SchemaCatalogEntry::CreateTableFunction(Transaction &transaction, CreateTableFunctionInformation *info) {
	auto table_function = make_unique_base<CatalogEntry, TableFunctionCatalogEntry>(catalog, this, info);
	if (!table_functions.CreateEntry(transaction, info->name, move(table_function))) {
		if (!info->or_replace) {
			throw CatalogException("Table function with name \"%s\" already exists!", info->name.c_str());
		} else {
			auto table_function = make_unique_base<CatalogEntry, TableFunctionCatalogEntry>(catalog, this, info);
			// function already exists: replace it
			if (!table_functions.DropEntry(transaction, info->name, false)) {
				throw CatalogException("CREATE OR REPLACE was specified, but "
				                       "function could not be dropped!");
			}
			if (!table_functions.CreateEntry(transaction, info->name, move(table_function))) {
				throw CatalogException("Error in recreating function in CREATE OR REPLACE");
			}
		}
	}
}

void SchemaCatalogEntry::DropTableFunction(Transaction &transaction, DropTableFunctionInformation *info) {
	if (!table_functions.DropEntry(transaction, info->name, info->cascade)) {
		if (!info->if_exists) {
			throw CatalogException("Table function with name \"%s\" does not exist!", info->name.c_str());
		}
	}
}

void SchemaCatalogEntry::CreateScalarFunction(Transaction &transaction, CreateScalarFunctionInformation *info) {
	auto scalar_function = make_unique_base<CatalogEntry, ScalarFunctionCatalogEntry>(catalog, this, info);
	if (!scalar_functions.CreateEntry(transaction, info->name, move(scalar_function))) {
		if (!info->or_replace) {
			throw CatalogException("Scalar function with name \"%s\" already exists!", info->name.c_str());
		} else {
			auto scalar_function = make_unique_base<CatalogEntry, ScalarFunctionCatalogEntry>(catalog, this, info);
			// function already exists: replace it
			if (!scalar_functions.DropEntry(transaction, info->name, false)) {
				throw CatalogException("CREATE OR REPLACE was specified, but "
				                       "function could not be dropped!");
			}
			if (!scalar_functions.CreateEntry(transaction, info->name, move(scalar_function))) {
				throw CatalogException("Error in recreating function in CREATE OR REPLACE");
			}
		}
	}
}

ScalarFunctionCatalogEntry *SchemaCatalogEntry::GetScalarFunction(Transaction &transaction, const string &name) {
	auto entry = scalar_functions.GetEntry(transaction, name);
	if (!entry) {
		throw CatalogException("Scalar Function with name %s does not exist!", name.c_str());
	}
	return (ScalarFunctionCatalogEntry *)entry;
}

bool SchemaCatalogEntry::HasDependents(Transaction &transaction) {
	return !tables.IsEmpty(transaction) || !table_functions.IsEmpty(transaction) ||
	       !scalar_functions.IsEmpty(transaction);
}

void SchemaCatalogEntry::DropDependents(Transaction &transaction) {
	tables.DropAllEntries(transaction);
	table_functions.DropAllEntries(transaction);
	scalar_functions.DropAllEntries(transaction);
}
