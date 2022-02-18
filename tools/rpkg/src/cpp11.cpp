// Generated by cpp11: do not edit by hand
// clang-format off

#include "duckdb_types.hpp"
#include "cpp11/declarations.hpp"
#include <R_ext/Visibility.h>

// connection.cpp
duckdb::conn_eptr_t rapi_connect(duckdb::db_eptr_t db);
extern "C" SEXP _duckdb_rapi_connect(SEXP db) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_connect(cpp11::as_cpp<cpp11::decay_t<duckdb::db_eptr_t>>(db)));
  END_CPP11
}
// connection.cpp
void rapi_disconnect(duckdb::conn_eptr_t conn);
extern "C" SEXP _duckdb_rapi_disconnect(SEXP conn) {
  BEGIN_CPP11
    rapi_disconnect(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(conn));
    return R_NilValue;
  END_CPP11
}
// database.cpp
duckdb::db_eptr_t rapi_startup(std::string dbdir, bool readonly, cpp11::list configsexp);
extern "C" SEXP _duckdb_rapi_startup(SEXP dbdir, SEXP readonly, SEXP configsexp) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_startup(cpp11::as_cpp<cpp11::decay_t<std::string>>(dbdir), cpp11::as_cpp<cpp11::decay_t<bool>>(readonly), cpp11::as_cpp<cpp11::decay_t<cpp11::list>>(configsexp)));
  END_CPP11
}
// database.cpp
void rapi_shutdown(duckdb::db_eptr_t dbsexp);
extern "C" SEXP _duckdb_rapi_shutdown(SEXP dbsexp) {
  BEGIN_CPP11
    rapi_shutdown(cpp11::as_cpp<cpp11::decay_t<duckdb::db_eptr_t>>(dbsexp));
    return R_NilValue;
  END_CPP11
}
// register.cpp
void rapi_register_df(duckdb::conn_eptr_t conn, std::string name, cpp11::data_frame value);
extern "C" SEXP _duckdb_rapi_register_df(SEXP conn, SEXP name, SEXP value) {
  BEGIN_CPP11
    rapi_register_df(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(conn), cpp11::as_cpp<cpp11::decay_t<std::string>>(name), cpp11::as_cpp<cpp11::decay_t<cpp11::data_frame>>(value));
    return R_NilValue;
  END_CPP11
}
// register.cpp
void rapi_unregister_df(duckdb::conn_eptr_t conn, std::string name);
extern "C" SEXP _duckdb_rapi_unregister_df(SEXP conn, SEXP name) {
  BEGIN_CPP11
    rapi_unregister_df(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(conn), cpp11::as_cpp<cpp11::decay_t<std::string>>(name));
    return R_NilValue;
  END_CPP11
}
// register.cpp
void rapi_register_arrow(duckdb::conn_eptr_t conn, std::string name, cpp11::list export_funs, cpp11::sexp valuesexp);
extern "C" SEXP _duckdb_rapi_register_arrow(SEXP conn, SEXP name, SEXP export_funs, SEXP valuesexp) {
  BEGIN_CPP11
    rapi_register_arrow(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(conn), cpp11::as_cpp<cpp11::decay_t<std::string>>(name), cpp11::as_cpp<cpp11::decay_t<cpp11::list>>(export_funs), cpp11::as_cpp<cpp11::decay_t<cpp11::sexp>>(valuesexp));
    return R_NilValue;
  END_CPP11
}
// register.cpp
void rapi_unregister_arrow(duckdb::conn_eptr_t conn, std::string name);
extern "C" SEXP _duckdb_rapi_unregister_arrow(SEXP conn, SEXP name) {
  BEGIN_CPP11
    rapi_unregister_arrow(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(conn), cpp11::as_cpp<cpp11::decay_t<std::string>>(name));
    return R_NilValue;
  END_CPP11
}
// relational.cpp
SEXP rapi_expr_reference(std::string name, std::string table);
extern "C" SEXP _duckdb_rapi_expr_reference(SEXP name, SEXP table) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_expr_reference(cpp11::as_cpp<cpp11::decay_t<std::string>>(name), cpp11::as_cpp<cpp11::decay_t<std::string>>(table)));
  END_CPP11
}
// relational.cpp
SEXP rapi_expr_constant(sexp val);
extern "C" SEXP _duckdb_rapi_expr_constant(SEXP val) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_expr_constant(cpp11::as_cpp<cpp11::decay_t<sexp>>(val)));
  END_CPP11
}
// relational.cpp
SEXP rapi_expr_function(std::string name, list args);
extern "C" SEXP _duckdb_rapi_expr_function(SEXP name, SEXP args) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_expr_function(cpp11::as_cpp<cpp11::decay_t<std::string>>(name), cpp11::as_cpp<cpp11::decay_t<list>>(args)));
  END_CPP11
}
// relational.cpp
void rapi_expr_set_alias(duckdb::expr_extptr_t expr, std::string alias);
extern "C" SEXP _duckdb_rapi_expr_set_alias(SEXP expr, SEXP alias) {
  BEGIN_CPP11
    rapi_expr_set_alias(cpp11::as_cpp<cpp11::decay_t<duckdb::expr_extptr_t>>(expr), cpp11::as_cpp<cpp11::decay_t<std::string>>(alias));
    return R_NilValue;
  END_CPP11
}
// relational.cpp
std::string rapi_expr_tostring(duckdb::expr_extptr_t expr);
extern "C" SEXP _duckdb_rapi_expr_tostring(SEXP expr) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_expr_tostring(cpp11::as_cpp<cpp11::decay_t<duckdb::expr_extptr_t>>(expr)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_from_df(duckdb::conn_eptr_t con, data_frame df);
extern "C" SEXP _duckdb_rapi_rel_from_df(SEXP con, SEXP df) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_from_df(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(con), cpp11::as_cpp<cpp11::decay_t<data_frame>>(df)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_filter(duckdb::rel_extptr_t rel, list exprs);
extern "C" SEXP _duckdb_rapi_rel_filter(SEXP rel, SEXP exprs) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_filter(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<list>>(exprs)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_project(duckdb::rel_extptr_t rel, list exprs);
extern "C" SEXP _duckdb_rapi_rel_project(SEXP rel, SEXP exprs) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_project(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<list>>(exprs)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_aggregate(duckdb::rel_extptr_t rel, list groups, list aggregates);
extern "C" SEXP _duckdb_rapi_rel_aggregate(SEXP rel, SEXP groups, SEXP aggregates) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_aggregate(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<list>>(groups), cpp11::as_cpp<cpp11::decay_t<list>>(aggregates)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_order(duckdb::rel_extptr_t rel, list orders);
extern "C" SEXP _duckdb_rapi_rel_order(SEXP rel, SEXP orders) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_order(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<list>>(orders)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_inner_join(duckdb::rel_extptr_t left, duckdb::rel_extptr_t right, list conds);
extern "C" SEXP _duckdb_rapi_rel_inner_join(SEXP left, SEXP right, SEXP conds) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_inner_join(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(left), cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(right), cpp11::as_cpp<cpp11::decay_t<list>>(conds)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_limit(duckdb::rel_extptr_t rel, int64_t n);
extern "C" SEXP _duckdb_rapi_rel_limit(SEXP rel, SEXP n) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_limit(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<int64_t>>(n)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_to_df(duckdb::rel_extptr_t rel);
extern "C" SEXP _duckdb_rapi_rel_to_df(SEXP rel) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_to_df(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel)));
  END_CPP11
}
// relational.cpp
std::string rapi_rel_tostring(duckdb::rel_extptr_t rel);
extern "C" SEXP _duckdb_rapi_rel_tostring(SEXP rel) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_tostring(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_explain(duckdb::rel_extptr_t rel);
extern "C" SEXP _duckdb_rapi_rel_explain(SEXP rel) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_explain(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel)));
  END_CPP11
}
// relational.cpp
std::string rapi_rel_alias(duckdb::rel_extptr_t rel);
extern "C" SEXP _duckdb_rapi_rel_alias(SEXP rel) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_alias(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_set_alias(duckdb::rel_extptr_t rel, std::string alias);
extern "C" SEXP _duckdb_rapi_rel_set_alias(SEXP rel, SEXP alias) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_set_alias(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<std::string>>(alias)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_sql(duckdb::rel_extptr_t rel, std::string sql);
extern "C" SEXP _duckdb_rapi_rel_sql(SEXP rel, SEXP sql) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_sql(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel), cpp11::as_cpp<cpp11::decay_t<std::string>>(sql)));
  END_CPP11
}
// relational.cpp
SEXP rapi_rel_names(duckdb::rel_extptr_t rel);
extern "C" SEXP _duckdb_rapi_rel_names(SEXP rel) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_rel_names(cpp11::as_cpp<cpp11::decay_t<duckdb::rel_extptr_t>>(rel)));
  END_CPP11
}
// statement.cpp
void rapi_release(duckdb::stmt_eptr_t stmt);
extern "C" SEXP _duckdb_rapi_release(SEXP stmt) {
  BEGIN_CPP11
    rapi_release(cpp11::as_cpp<cpp11::decay_t<duckdb::stmt_eptr_t>>(stmt));
    return R_NilValue;
  END_CPP11
}
// statement.cpp
cpp11::list rapi_prepare(duckdb::conn_eptr_t conn, std::string query);
extern "C" SEXP _duckdb_rapi_prepare(SEXP conn, SEXP query) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_prepare(cpp11::as_cpp<cpp11::decay_t<duckdb::conn_eptr_t>>(conn), cpp11::as_cpp<cpp11::decay_t<std::string>>(query)));
  END_CPP11
}
// statement.cpp
cpp11::list rapi_bind(duckdb::stmt_eptr_t stmt, cpp11::list params, bool arrow);
extern "C" SEXP _duckdb_rapi_bind(SEXP stmt, SEXP params, SEXP arrow) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_bind(cpp11::as_cpp<cpp11::decay_t<duckdb::stmt_eptr_t>>(stmt), cpp11::as_cpp<cpp11::decay_t<cpp11::list>>(params), cpp11::as_cpp<cpp11::decay_t<bool>>(arrow)));
  END_CPP11
}
// statement.cpp
SEXP rapi_execute_arrow(duckdb::rqry_eptr_t qry_res, bool stream, int vec_per_chunk, bool return_table);
extern "C" SEXP _duckdb_rapi_execute_arrow(SEXP qry_res, SEXP stream, SEXP vec_per_chunk, SEXP return_table) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_execute_arrow(cpp11::as_cpp<cpp11::decay_t<duckdb::rqry_eptr_t>>(qry_res), cpp11::as_cpp<cpp11::decay_t<bool>>(stream), cpp11::as_cpp<cpp11::decay_t<int>>(vec_per_chunk), cpp11::as_cpp<cpp11::decay_t<bool>>(return_table)));
  END_CPP11
}
// statement.cpp
SEXP rapi_record_batch(duckdb::rqry_eptr_t qry_res, int approx_batch_size);
extern "C" SEXP _duckdb_rapi_record_batch(SEXP qry_res, SEXP approx_batch_size) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_record_batch(cpp11::as_cpp<cpp11::decay_t<duckdb::rqry_eptr_t>>(qry_res), cpp11::as_cpp<cpp11::decay_t<int>>(approx_batch_size)));
  END_CPP11
}
// statement.cpp
SEXP rapi_execute(duckdb::stmt_eptr_t stmt, bool arrow);
extern "C" SEXP _duckdb_rapi_execute(SEXP stmt, SEXP arrow) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_execute(cpp11::as_cpp<cpp11::decay_t<duckdb::stmt_eptr_t>>(stmt), cpp11::as_cpp<cpp11::decay_t<bool>>(arrow)));
  END_CPP11
}
// utils.cpp
cpp11::r_string rapi_ptr_to_str(SEXP extptr);
extern "C" SEXP _duckdb_rapi_ptr_to_str(SEXP extptr) {
  BEGIN_CPP11
    return cpp11::as_sexp(rapi_ptr_to_str(cpp11::as_cpp<cpp11::decay_t<SEXP>>(extptr)));
  END_CPP11
}

extern "C" {
static const R_CallMethodDef CallEntries[] = {
    {"_duckdb_rapi_bind",             (DL_FUNC) &_duckdb_rapi_bind,             3},
    {"_duckdb_rapi_connect",          (DL_FUNC) &_duckdb_rapi_connect,          1},
    {"_duckdb_rapi_disconnect",       (DL_FUNC) &_duckdb_rapi_disconnect,       1},
    {"_duckdb_rapi_execute",          (DL_FUNC) &_duckdb_rapi_execute,          2},
    {"_duckdb_rapi_execute_arrow",    (DL_FUNC) &_duckdb_rapi_execute_arrow,    4},
    {"_duckdb_rapi_expr_constant",    (DL_FUNC) &_duckdb_rapi_expr_constant,    1},
    {"_duckdb_rapi_expr_function",    (DL_FUNC) &_duckdb_rapi_expr_function,    2},
    {"_duckdb_rapi_expr_reference",   (DL_FUNC) &_duckdb_rapi_expr_reference,   2},
    {"_duckdb_rapi_expr_set_alias",   (DL_FUNC) &_duckdb_rapi_expr_set_alias,   2},
    {"_duckdb_rapi_expr_tostring",    (DL_FUNC) &_duckdb_rapi_expr_tostring,    1},
    {"_duckdb_rapi_prepare",          (DL_FUNC) &_duckdb_rapi_prepare,          2},
    {"_duckdb_rapi_ptr_to_str",       (DL_FUNC) &_duckdb_rapi_ptr_to_str,       1},
    {"_duckdb_rapi_record_batch",     (DL_FUNC) &_duckdb_rapi_record_batch,     2},
    {"_duckdb_rapi_register_arrow",   (DL_FUNC) &_duckdb_rapi_register_arrow,   4},
    {"_duckdb_rapi_register_df",      (DL_FUNC) &_duckdb_rapi_register_df,      3},
    {"_duckdb_rapi_rel_aggregate",    (DL_FUNC) &_duckdb_rapi_rel_aggregate,    3},
    {"_duckdb_rapi_rel_alias",        (DL_FUNC) &_duckdb_rapi_rel_alias,        1},
    {"_duckdb_rapi_rel_explain",      (DL_FUNC) &_duckdb_rapi_rel_explain,      1},
    {"_duckdb_rapi_rel_filter",       (DL_FUNC) &_duckdb_rapi_rel_filter,       2},
    {"_duckdb_rapi_rel_from_df",      (DL_FUNC) &_duckdb_rapi_rel_from_df,      2},
    {"_duckdb_rapi_rel_inner_join",   (DL_FUNC) &_duckdb_rapi_rel_inner_join,   3},
    {"_duckdb_rapi_rel_limit",        (DL_FUNC) &_duckdb_rapi_rel_limit,        2},
    {"_duckdb_rapi_rel_names",        (DL_FUNC) &_duckdb_rapi_rel_names,        1},
    {"_duckdb_rapi_rel_order",        (DL_FUNC) &_duckdb_rapi_rel_order,        2},
    {"_duckdb_rapi_rel_project",      (DL_FUNC) &_duckdb_rapi_rel_project,      2},
    {"_duckdb_rapi_rel_set_alias",    (DL_FUNC) &_duckdb_rapi_rel_set_alias,    2},
    {"_duckdb_rapi_rel_sql",          (DL_FUNC) &_duckdb_rapi_rel_sql,          2},
    {"_duckdb_rapi_rel_to_df",        (DL_FUNC) &_duckdb_rapi_rel_to_df,        1},
    {"_duckdb_rapi_rel_tostring",     (DL_FUNC) &_duckdb_rapi_rel_tostring,     1},
    {"_duckdb_rapi_release",          (DL_FUNC) &_duckdb_rapi_release,          1},
    {"_duckdb_rapi_shutdown",         (DL_FUNC) &_duckdb_rapi_shutdown,         1},
    {"_duckdb_rapi_startup",          (DL_FUNC) &_duckdb_rapi_startup,          3},
    {"_duckdb_rapi_unregister_arrow", (DL_FUNC) &_duckdb_rapi_unregister_arrow, 2},
    {"_duckdb_rapi_unregister_df",    (DL_FUNC) &_duckdb_rapi_unregister_df,    2},
    {NULL, NULL, 0}
};
}

void AltrepString_Initialize(DllInfo* dll);

extern "C" attribute_visible void R_init_duckdb(DllInfo* dll){
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  AltrepString_Initialize(dll);
  R_forceSymbols(dll, TRUE);
}
