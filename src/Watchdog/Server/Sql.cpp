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

SqlBool Regexp(const SqlVal& a, const SqlVal& b) {
	return SqlBool(a, " regexp ", b, SqlS::COMP);
}

SqlVal SqlEmptyString(){
	static SqlVal s("''",SqlS::HIGH);
	return s;
}