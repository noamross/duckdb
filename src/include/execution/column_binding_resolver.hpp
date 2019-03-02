//===----------------------------------------------------------------------===//
//                         DuckDB
//
// execution/column_binding_resolver.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "planner/table_binding_resolver.hpp"

namespace duckdb {

//! The ColumnBindingResolver resolves ColumnBindings into base tables
//! (table_index, column_index) into physical indices into the DataChunks that
//! are used within the execution engine
class ColumnBindingResolver : public TableBindingResolver {
public:
	ColumnBindingResolver() : 
		TableBindingResolver(true) {
	}
protected:
	using SQLNodeVisitor::Visit;
	void Visit(ColumnRefExpression &expr) override {
		throw Exception(
		    "ColumnRefExpression is not allowed here, should have been converted to BoundColumnRefExpression already!");
	}
	unique_ptr<Expression> VisitReplace(BoundColumnRefExpression &expr, unique_ptr<Expression> *expr_ptr) override;
};
} // namespace duckdb
