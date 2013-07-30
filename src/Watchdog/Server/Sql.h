#ifndef _Watchdog_Server_Sql_h_
#define _Watchdog_Server_Sql_h_

#include <DynamicSql/DynamicSql.h>

#define  MODEL <Watchdog/Server/table.sch>
#include <DynamicSql/sch_header.h>

SqlStatement InsertIgnore(int dialect, const SqlInsert& insert);
bool Upsert(Sql& sql, const SqlInsert& insert, const SqlUpdate& update);
SqlBool Regexp(const SqlVal& a, const SqlVal& b);

inline SqlVal ToSqlVal(const SqlBool& b) {
	return SqlVal(~b, SqlS::HIGH);
}

inline SqlVal If(const SqlBool& b, const SqlVal& trueval, const SqlVal& falseval) {
	return SqlFunc("if", SqlSet(SqlVal(~b, SqlS::HIGH), trueval, falseval));
}

inline SqlVal CountIf(const SqlVal& val, const SqlBool& b){
	//SqlBool cond = (b && SqlBool(b(SqlS::COMP) + " is not NULL", SqlS::COMP));
	return SqlFunc("sum", If(b, val, 0));
}

inline SqlVal Concat(const SqlSet& set){
	return SqlFunc("concat", set);
}

void AddSqliteCompatibilityFunctions(DynamicSqlSession& dsql);

#endif
