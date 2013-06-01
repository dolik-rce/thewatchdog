#include <plugin/sqlite3/Sqlite3.h>

using namespace Upp;

#ifdef __cplusplus
extern "C" {
#endif
void* GetSession(){
	static Sqlite3Session session;
	return &session;
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