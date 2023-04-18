#include "duckdb/optimizer/column_binding_replacer.hpp"

#include "duckdb/planner/expression/bound_columnref_expression.hpp"

namespace duckdb {

ReplaceBinding::ReplaceBinding() : replace_type(false) {
}

ReplaceBinding::ReplaceBinding(ColumnBinding old_binding, ColumnBinding new_binding)
    : old_binding(old_binding), new_binding(new_binding), replace_type(false) {
}

ReplaceBinding::ReplaceBinding(ColumnBinding old_binding, ColumnBinding new_binding, LogicalType new_type)
    : old_binding(old_binding), new_binding(new_binding), replace_type(true), new_type(std::move(new_type)) {
}

ColumnBindingReplacer::ColumnBindingReplacer() {
}

void ColumnBindingReplacer::VisitOperator(LogicalOperator &op) {
	if (stop_operator && stop_operator == &op) {
		return;
	}
	VisitOperatorChildren(op);
	VisitOperatorExpressions(op);
}

void ColumnBindingReplacer::VisitExpression(unique_ptr<Expression> *expression) {
	auto &expr = *expression;
	if (expr->expression_class == ExpressionClass::BOUND_COLUMN_REF) {
		auto &bound_column_ref = expr->Cast<BoundColumnRefExpression>();
		for (const auto &replace_binding : replace_bindings) {
			if (bound_column_ref.binding == replace_binding.old_binding) {
				bound_column_ref.binding = replace_binding.new_binding;
				if (replace_binding.replace_type) {
					bound_column_ref.return_type = replace_binding.new_type;
				}
			}

			if (column_binding_callback) {
				column_binding_callback(bound_column_ref, replace_binding);
			}
		}
	}

	VisitExpressionChildren(**expression);
}

} // namespace duckdb
