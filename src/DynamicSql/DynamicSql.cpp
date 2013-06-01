#include "DynamicSql.h"

#define DLIMODULE   SQLITE_DLL
#define DLIHEADER   <DynamicSql/sqlite/sqlite.dli>
#include <Core/dli_source.h>

#define DLIMODULE   MYSQL_DLL
#define DLIHEADER   <DynamicSql/mysql/mysql.dli>
#include <Core/dli_source.h>

void DynamicSqlSession::Begin() {
	GetSession().Begin();
}

void DynamicSqlSession::Commit() {
	GetSession().Commit();
}

void DynamicSqlSession::Rollback() {
	GetSession().Rollback();
}

int DynamicSqlSession::GetTransactionLevel() const{
	return GetSession().GetTransactionLevel();
}

bool DynamicSqlSession::IsOpen() const
{
	return GetSession().IsOpen();
}

Vector<String> DynamicSqlSession::EnumUsers()
{
	return GetSession().EnumUsers();
}

Vector<String> DynamicSqlSession::EnumDatabases()
{
	return GetSession().EnumDatabases();
}

Vector<String> DynamicSqlSession::EnumTables(String database)
{
	return GetSession().EnumTables(database);
}

Vector<String> DynamicSqlSession::EnumViews(String database)
{
	return GetSession().EnumViews(database);
}

Vector<String> DynamicSqlSession::EnumSequences(String database)
{
	return GetSession().EnumSequences(database);
}

Vector<SqlColumnInfo> DynamicSqlSession::EnumColumns(String database, String table)
{
	return GetSession().EnumColumns(database, table);
}

Vector<String> DynamicSqlSession::EnumPrimaryKey(String database, String table)
{
	return GetSession().EnumPrimaryKey(database, table);
}

NAMESPACE_UPP
String MySqlTextType(int n) {
	return n < 65536 ? Format("varchar(%d)", n) : String("text");
}
END_UPP_NAMESPACE
