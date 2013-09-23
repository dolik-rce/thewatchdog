#include <plugin/sqlite3/Sqlite3.h>

using namespace Upp;

One<Sqlite3Session>& gSession(){
	static One<Sqlite3Session> session;
	return session;
}

#ifdef __cplusplus
extern "C" {
#endif
void* GetSession(){
	One<Sqlite3Session>& s = gSession();
	if(s.IsEmpty())
		s.Create();
	return ~s;
}

void* ResetSession(){
	return &(gSession().Create());
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
