#ifndef _Watchdog_Server_Sql_h_
#define _Watchdog_Server_Sql_h_

#include <DynamicSql/DynamicSql.h>

#define  MODEL <Watchdog/Server/table.sch>
#include <DynamicSql/sch_header.h>

SqlStatement InsertIgnore(const SqlInsert& insert);
bool Upsert(Sql& sql, const SqlInsert& insert, const SqlUpdate& update);
SqlBool Regexp(const SqlVal& a, const SqlVal& b);
SqlVal SqlEmptyString();
SqlVal DateSub(const SqlVal& date, const SqlVal& interval);
SqlVal SqlInterval(const SqlVal& count, const String& unit);
SqlVal TimeDiff(const SqlVal& a, const SqlVal& b);
String SqlEscape(const Value& in);

inline SqlVal ToSqlVal(const SqlBool& b) {
	return SqlVal(~b, SqlS::HIGH);
}

inline SqlVal If(const SqlBool& b, const SqlVal& trueval, const SqlVal& falseval) {
	return SqlFunc("if", SqlSet(SqlVal(~b, SqlS::HIGH), trueval, falseval));
}

inline SqlVal SqlNull() {
	return SqlVal("NULL", SqlS::HIGH);
}

inline SqlVal CountIf(const SqlVal& val, const SqlBool& b){
	//SqlBool cond = (b && SqlBool(b(SqlS::COMP) + " is not NULL", SqlS::COMP));
	return SqlFunc("sum", If(b, val, 0));
}

inline SqlVal CountDistinctIf(const SqlVal& val, const SqlBool& b){
	return Count(Distinct(If(b, val, SqlNull())));
}

inline SqlVal Concat(const SqlSet& set){
	return SqlFunc("concat", set);
}

inline SqlId SqlAll(const SqlId& tbl) {
	return SqlId("*").Of(tbl);
}

void AddSqliteCompatibilityFunctions(DynamicSqlSession& dsql);

#endif
