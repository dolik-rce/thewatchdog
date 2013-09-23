#include <MySql/MySql.h>

using namespace Upp;

One<MySqlSession>& gSession(){
	static One<MySqlSession> session;
	return session;
}

#ifdef __cplusplus
extern "C" {
#endif
void* GetSession(){
	One<MySqlSession>& s = gSession();
	if(s.IsEmpty())
		s.Create();
	return ~s;
}

void* ResetSession(){
	return &(gSession().Create());
}

bool Connect(const char *user, const char *password, const char *database,
	         const char *host, int port, const char *socket){
	MySqlSession* session = (MySqlSession*)GetSession();
	return session->Connect(user, password, database, host, port, socket);
}

void AutoReconnect(){
	MySqlSession* session = (MySqlSession*)GetSession();
	session->AutoReconnect();
}

void Close() {
	MySqlSession* session = (MySqlSession*)GetSession();
	session->Close();
}

#ifdef __cplusplus
}
#endif
