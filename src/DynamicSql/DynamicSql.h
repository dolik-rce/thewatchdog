#ifndef _sqlbackend_DynamicSql_h_
#define _sqlbackend_DynamicSql_h_

#include <MySql/MySql.h>
#include <plugin/sqlite3/Sqlite3.h>

using namespace Upp;

#define DLIMODULE   SQLITE_DLL
#define DLIHEADER   <DynamicSql/sqlite/sqlite.dli>
#include <Core/dli_header.h>

#define DLIMODULE   MYSQL_DLL
#define DLIHEADER   <DynamicSql/mysql/mysql.dli>
#include <Core/dli_header.h>

inline unsigned int lg2(unsigned int v){
	// from http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
	static const unsigned int b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
	register unsigned int r = (v & b[0]) != 0;
	r |= ((v & b[1]) != 0) << 1;
	r |= ((v & b[2]) != 0) << 2;
	r |= ((v & b[3]) != 0) << 3;
	r |= ((v & b[4]) != 0) << 4;
	return r;
}


class DynamicSqlSession : public SqlSession {
private:
	String libpath;
	T_SQLITE_DLL& sqlite;
	T_MYSQL_DLL& mysql;
	
	typedef DynamicSqlSession CLASSNAME;
	
public:
	virtual void                  Begin();
	virtual void                  Commit();
	virtual void                  Rollback();
	virtual int                   GetTransactionLevel() const;
	
	virtual bool                  IsOpen() const;
	
	virtual Vector<String>        EnumUsers();
	virtual Vector<String>        EnumDatabases();
	virtual Vector<String>        EnumTables(String database);
	virtual Vector<String>        EnumViews(String database);
	virtual Vector<String>        EnumSequences(String database);
	virtual Vector<SqlColumnInfo> EnumColumns(String database, String table);
	virtual Vector<String>        EnumPrimaryKey(String database, String table);
	
	SqlSession& GetSession() {
		void *session;
		switch(dialect) {
			case MY_SQL: session = mysql.GetSession(); break;
			case SQLITE3: session = sqlite.GetSession(); break;
		}
		ASSERT(session!=NULL);
		return *(SqlSession*)session;
	}
	
	SqlSession& GetSession() const{
		void *session;
		switch(dialect) {
			case MY_SQL: session = mysql.GetSession(); break;
			case SQLITE3: session = sqlite.GetSession(); break;
		}
		ASSERT(session!=NULL);
		return *(SqlSession*)session;
	}
	
	MySqlSession& GetMySqlSession(){
		ASSERT(dialect == MY_SQL);
		return *((MySqlSession*)&GetSession());
	};
	
	Sqlite3Session& GetSqlite3Session(){
		ASSERT(dialect == SQLITE3);
		return *((Sqlite3Session*)&GetSession());
	};
	
	bool MySqlConnect(const char *user = NULL, const char *password = NULL, const char *database = NULL,
	                  const char *host = NULL, int port = MYSQL_PORT, const char *socket = NULL) {
		ASSERT(dialect == MY_SQL);
		return mysql.Connect(user, password, database, host, port, socket);
	}
	
	void MySqlAutoReconnect() {
		ASSERT(dialect == MY_SQL);
		mysql.AutoReconnect();
	}
	
	void MySqlClose() {
		ASSERT(dialect == MY_SQL);
		mysql.Close();
	}
	
	bool SqliteOpen(const char *filename) {
		ASSERT(dialect == SQLITE3);
		return sqlite.Open(filename);
	}
	
	void SqliteClose() {
		ASSERT(dialect == SQLITE3);
		sqlite.Close();
	}
	
	static const char** Dialects(){
		static const char* names[] = { "oracle", "sqlite", "mysql" , "mssql", "pgsql"};
		return names;
	}
	
	static const int* SupportedDialects(){
		static const int dialects[] = { SQLITE3, MY_SQL, 0 };
		return dialects;
	}
	
	String DialectToString() const {
		ASSERT(lg2(dialect)>=0 && lg2(dialect)<5);
		return Dialects()[lg2(dialect)];
	}
	
	static String DialectToString(int dialect) {
		ASSERT(lg2(dialect)>=0 && lg2(dialect)<5);
		return Dialects()[lg2(dialect)];
	}
	
	static int StringToDialect(const char* str){
		for(int i = 0; i < 5; i++){
			if(Dialects()[i] == ToLower(str))
				return 1<<i;
		}
		return 0;
	}
	
	DynamicSqlSession(int _dialect = -1, const String path = ".") :
	    libpath(path), sqlite(SQLITE_DLL_()), mysql(MYSQL_DLL_())
	{
		if(_dialect>=0)
			SetDialect(_dialect);
	}
	~DynamicSqlSession() {}
	
	void SetLibraryPath(const String path) { libpath = path; }
	
	void SetDialect(int _dialect){
		switch(_dialect) {
			case MY_SQL: mysql.Load(libpath+"/mysql.so"); break;
			case SQLITE3: sqlite.Load(libpath+"/sqlite.so"); break;
			default: NEVER_("Unknown SQL dialect");
		}
		Dialect(_dialect);
	}
};

#endif
