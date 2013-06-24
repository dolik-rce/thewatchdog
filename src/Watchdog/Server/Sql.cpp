#include "Sql.h"

SqlStatement InsertIgnore(int dialect, const SqlInsert& insert) {
	String s = SqlStatement(insert).Get(dialect);
	ASSERT(s.StartsWith("insert "));
	switch(dialect) {
	case SQLITE3:
		s = "insert or ignore " + s.Mid(7);
		break;
	case MY_SQL:
		s = "insert ignore " + s.Mid(7);
		break;
	default:
		NEVER_("InsertIgnore not implemented for this dialect");
	}
	return SqlStatement(s);
}

bool Upsert(Sql& sql, const SqlInsert& insert, const SqlUpdate& update) {
	sql * InsertIgnore(sql.GetDialect(), insert);
	if (sql.GetRowsProcessed() == 0) {
		sql * update;
		return false;
	}
	return true;
}

SqlVal CountIf(const SqlVal& val, const SqlBool& b){
	SqlBool cond = (!b || SqlBool(b(SqlS::COMP) + " is NULL", SqlS::COMP));
	return SqlFunc("sum", Case(cond, SqlVal(0))(val));
}

SqlVal Concat(const SqlSet& set){
//	//sqlite compatible concat function
//	if(SQL.GetDialect() == SQLITE3){
//		String s = ~set;
//		s.Replace(",","||");
//		return SqlVal(s, SqlS::FN);
//	}
	return SqlFunc("concat", set);
}

//add some compatibility functions to sqlite3
#include <plugin/sqlite3/lib/sqlite3.h>

extern "C" {
void sqlite_md5(sqlite3_context *context, int argc, sqlite3_value **argv) {
	if (argc == 1) {
		String s((const char*)sqlite3_value_text(argv[0]));
		String md5 = MD5StringS(s);
		sqlite3_result_text(context, md5.Begin(), md5.GetCount(), SQLITE_TRANSIENT);
		return;
	}
	sqlite3_result_null(context);
}
void sqlite_concat(sqlite3_context *context, int argc, sqlite3_value **argv) {
	if(argc==0) {
		sqlite3_result_null(context);
		return;
	}
	String s;
	for(int i = 0; i < argc; i++)
		s.Cat((const char*)sqlite3_value_text(argv[0]));
	sqlite3_result_text(context, s.Begin(), s.GetCount(), SQLITE_TRANSIENT);
}
}

void AddSqliteCompatibilityFunctions(sqlite3* db){
	sqlite3_initialize();
	sqlite3_create_function(db, "md5", 1, SQLITE_ANY, 0, sqlite_md5, 0, 0);
	sqlite3_create_function(db, "concat", -1, SQLITE_ANY, 0, sqlite_concat, 0, 0);
}
