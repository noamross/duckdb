//===----------------------------------------------------------------------===//
//                         DuckDB
//
// function/function.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/types/data_chunk.hpp"
#include "parser/column_definition.hpp"

namespace duckdb {
class BoundFunctionExpression;
class Catalog;
class ClientContext;
class Transaction;

struct TableFunctionData {
	virtual ~TableFunctionData() {
	}
};

//! Type used for initialization function
typedef TableFunctionData *(*table_function_init_t)(ClientContext &);
//! Type used for table-returning function
typedef void (*table_function_t)(ClientContext &, DataChunk &input, DataChunk &output, TableFunctionData *dataptr);
//! Type used for final (cleanup) function
typedef void (*table_function_final_t)(ClientContext &, TableFunctionData *dataptr);

//! The type used for scalar functions
typedef void (*scalar_function_t)(Vector inputs[], size_t input_count, BoundFunctionExpression &expr, Vector &result);
//! Type used for checking if a function matches the input arguments
typedef bool (*matches_argument_function_t)(vector<SQLType> &arguments);
//! Gets the return type of the function given the types of the input argument
typedef SQLType (*get_return_type_function_t)(vector<SQLType> &arguments);

class BuiltinFunctions {
public:
	//! Initialize a catalog with all built-in functions
	static void Initialize(Transaction &transaction, Catalog &catalog);
};

} // namespace duckdb
