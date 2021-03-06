//===----------------------------------------------------------------------===//
//                         DuckDB
//
// function/scalar_function/year.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/types/data_chunk.hpp"
#include "function/function.hpp"

namespace duckdb {
namespace function {

void year_function(Vector inputs[], size_t input_count, BoundFunctionExpression &expr, Vector &result);
bool year_matches_arguments(vector<SQLType> &arguments);
SQLType year_get_return_type(vector<SQLType> &arguments);

class YearFunction {
public:
	static const char *GetName() {
		return "year";
	}

	static scalar_function_t GetFunction() {
		return year_function;
	}

	static matches_argument_function_t GetMatchesArgumentFunction() {
		return year_matches_arguments;
	}

	static get_return_type_function_t GetReturnTypeFunction() {
		return year_get_return_type;
	}
};

} // namespace function
} // namespace duckdb
