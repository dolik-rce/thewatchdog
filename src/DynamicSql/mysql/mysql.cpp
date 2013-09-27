#include <MySql/MySql.h>

using namespace Upp;

#ifdef __cplusplus
extern "C" {
#endif
void* GetSession(){
	static MySqlSession session;
	return &session;
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
