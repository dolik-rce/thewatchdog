#ifndef _Watchdog_Server_Sql_h_
#define _Watchdog_Server_Sql_h_

#include <DynamicSql/DynamicSql.h>

#define  MODEL <Watchdog/Server/table.sch>
#include <DynamicSql/sch_header.h>

SqlStatement InsertIgnore(int dialect, const SqlInsert& insert);
bool Upsert(Sql& sql, const SqlInsert& insert, const SqlUpdate& update);
SqlVal CountIf(const SqlVal& val, const SqlBool& b);
SqlVal Concat(const SqlVal& val1, const SqlVal& val2);

#endif
