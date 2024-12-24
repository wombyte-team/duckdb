#include "duckdb/parser/statement/export_statement.hpp"
#include "duckdb/parser/transformer.hpp"

namespace duckdb {

unique_ptr<ExportStatement> Transformer::TransformExport(duckdb_libpgquery::PGExportStmt &stmt) {
	bool is_temporary = stmt.persistence == duckdb_libpgquery::PGPostgresRelPersistence::PG_RELPERSISTENCE_TEMP;

	auto info = make_uniq<CopyInfo>();
	info->file_path = stmt.filename;
	info->format = "csv";
	info->is_from = false;
	info->temporary = is_temporary;
	// handle export options
	TransformCopyOptions(*info, stmt.options);

	auto result = make_uniq<ExportStatement>(std::move(info));
	if (stmt.database) {
		result->database = stmt.database;
	}

	if (is_temporary) {
		result->database = TEMP_CATALOG;
	}

	return result;
}

} // namespace duckdb
