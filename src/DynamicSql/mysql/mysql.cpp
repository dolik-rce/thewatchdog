#include <DynamicSql/Common.h>
#include <MySql/MySql.h>

using namespace Upp;

#ifdef __cplusplus
extern "C" {
#endif
void* GetSession(){
#if __cplusplus >= 201103L
	// thread_local requires C++11
	static thread_local MySqlSession session;
	return &session;
#else
	return GetTLSession<MySqlSession>();
#endif
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
