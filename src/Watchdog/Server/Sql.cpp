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

SqlVal Concat(const SqlVal& val1, const SqlVal& val2){
	//sqlite compatible concat function
	if(SQL.GetDialect() == SQLITE3)
		return SqlVal(val1, "||", val2, SqlS::HIGH);
	return SqlFunc("concat", SqlSet(val1, val2));
}
