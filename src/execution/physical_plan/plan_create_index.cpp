#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/execution/operator/schema/physical_create_index.hpp"
#include "duckdb/execution/physical_plan_generator.hpp"
#include "duckdb/planner/operator/logical_create_index.hpp"
#include "duckdb/execution/operator/scan/physical_table_scan.hpp"

namespace duckdb {

unique_ptr<PhysicalOperator> PhysicalPlanGenerator::CreatePlan(LogicalCreateIndex &op) {

	D_ASSERT(op.children.empty());

	unique_ptr<TableFilterSet> table_filters;
	op.types.emplace_back(LogicalType::ROW_TYPE);
	op.column_ids.emplace_back(COLUMN_IDENTIFIER_ROW_ID);

	auto table_scan = make_unique<PhysicalTableScan>(op.types, op.function, move(op.bind_data), op.column_ids, op.names,
	                                                 move(table_filters), op.estimated_cardinality);

	dependencies.insert(&op.table);
	op.types.pop_back();
	op.column_ids.pop_back();

	auto physical_create_index =
	    make_unique<PhysicalCreateIndex>(op, op.table, op.column_ids, move(op.expressions), move(op.info),
	                                     move(op.unbound_expressions), op.estimated_cardinality);
	physical_create_index->children.push_back(move(table_scan));
	return physical_create_index;
}

} // namespace duckdb
