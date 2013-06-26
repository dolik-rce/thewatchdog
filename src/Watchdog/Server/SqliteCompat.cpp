#include "Sql.h"

namespace {
	T_SQLITE_DLL* sSqlite = NULL;
}

extern "C" {
void sqlite_md5(sqlite3_context *context, int argc, sqlite3_value **argv) {
	String md5 = MD5StringS((const char*)sSqlite->sqlite3_value_text(argv[0]));
	sSqlite->sqlite3_result_text(context, md5.Begin(), md5.GetCount(), SQLITE_TRANSIENT);
	return;
}
void sqlite_concat(sqlite3_context *context, int argc, sqlite3_value **argv) {
	if(argc==0) {
		sSqlite->sqlite3_result_null(context);
		return;
	}
	String s;
	for(int i = 0; i < argc; i++)
		s.Cat((const char*)sSqlite->sqlite3_value_text(argv[0]));
	sSqlite->sqlite3_result_text(context, s.Begin(), s.GetCount(), SQLITE_TRANSIENT);
}
void sqlite_if(sqlite3_context *context, int argc, sqlite3_value **argv) {
	bool cond = sSqlite->sqlite3_value_int(argv[0]);
	if(cond)
		return sSqlite->sqlite3_result_value(context, argv[1]); 
	else
		return sSqlite->sqlite3_result_value(context, argv[2]); 
}
}

void AddSqliteCompatibilityFunctions(DynamicSqlSession& dsql){
//	sqlite3_initialize();
	::sqlite3* db = (::sqlite3*)(Sqlite3Session::sqlite3*)dsql.GetSqlite3Session();
	sSqlite = &dsql.GetSqliteDli();
	sSqlite->sqlite3_create_function(db, "md5", 1, SQLITE_ANY, 0, (void*)sqlite_md5, 0, 0);
	sSqlite->sqlite3_create_function(db, "concat", -1, SQLITE_ANY, 0, (void*)sqlite_concat, 0, 0);
	sSqlite->sqlite3_create_function(db, "if", 3, SQLITE_ANY, 0, (void*)sqlite_if, 0, 0);
}
