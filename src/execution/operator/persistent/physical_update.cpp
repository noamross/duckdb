#include "execution/operator/persistent/physical_update.hpp"

#include "common/vector_operations/vector_operations.hpp"
#include "execution/expression_executor.hpp"
#include "main/client_context.hpp"
#include "planner/expression/bound_reference_expression.hpp"
#include "storage/data_table.hpp"

using namespace duckdb;
using namespace std;

void PhysicalUpdate::_GetChunk(ClientContext &context, DataChunk &chunk, PhysicalOperatorState *state) {
	vector<TypeId> update_types;
	for (auto &expr : expressions) {
		update_types.push_back(expr->return_type);
	}
	DataChunk update_chunk;
	update_chunk.Initialize(update_types);

	int64_t updated_count = 0;
	while (true) {
		children[0]->GetChunk(context, state->child_chunk, state->child_state.get());
		if (state->child_chunk.size() == 0) {
			break;
		}
		// update data in the base table
		// the row ids are given to us as the last column of the child chunk
		auto &row_ids = state->child_chunk.data[state->child_chunk.column_count - 1];
		for (size_t i = 0; i < expressions.size(); i++) {
			if (expressions[i]->type == ExpressionType::VALUE_DEFAULT) {
				// default expression, set to the default value of the column
				auto &column = tableref.columns[columns[i]];
				update_chunk.data[i].count = state->child_chunk.size();
				update_chunk.data[i].sel_vector = state->child_chunk.sel_vector;
				VectorOperations::Set(update_chunk.data[i], column.default_value);
			} else {
				assert(expressions[i]->type == ExpressionType::BOUND_REF);
				// index into child chunk
				auto &binding = (BoundReferenceExpression &)*expressions[i];
				update_chunk.data[i].Reference(state->child_chunk.data[binding.index]);
			}
		}
		update_chunk.sel_vector = state->child_chunk.sel_vector;

		table.Update(tableref, context, row_ids, columns, update_chunk);
		updated_count += state->child_chunk.size();
	}

	chunk.data[0].count = 1;
	chunk.data[0].SetValue(0, Value::BIGINT(updated_count));

	state->finished = true;

	chunk.Verify();
}
