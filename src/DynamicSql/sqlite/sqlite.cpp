#include <DynamicSql/Common.h>
#include <plugin/sqlite3/Sqlite3.h>

using namespace Upp;

#ifdef __cplusplus
extern "C" {
#endif
void* GetSession(){
#if __cplusplus >= 201103L
	// thread_local requires C++11
	static thread_local Sqlite3Session session;
	return &session;
#else
	return GetTLSession<Sqlite3Session>();
#endif
}

bool Open(const char* filename){
	Sqlite3Session* session = (Sqlite3Session*)GetSession();
	return session->Open(filename);
}

void Close(const char* filename){
	Sqlite3Session* session = (Sqlite3Session*)GetSession();
	session->Close();
}

#ifdef __cplusplus
}
#endif
