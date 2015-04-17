#ifndef _Watchdog_Watchdog_h
#define _Watchdog_Watchdog_h

#include <Skylark/Skylark.h>
#include <Core/SMTP/SMTP.h>
#include <Watchdog/Watchdog.h>
using namespace Upp;

#include "Sql.h"
#include "Util.h"
#include "Entity.h"
#include "Email.h"

namespace Upp{ 
	void ReopenLog();
	namespace Ini {
		extern IniString sql_user;
		extern IniString sql_password;
		extern IniString sql_database;
		extern IniString sql_host;
		extern IniInt    sql_port;
		extern IniString sql_socket;
		extern IniBool   sql_log;
		extern IniBool   debug;
		extern IniString log_file;
		extern IniInt    log_level;
		extern IniString output_dir;
		extern IniString server_url;
		extern IniString svn;
		extern IniInt    max_test_time;
		extern IniString smtp_host;
		extern IniInt    smtp_port;
		extern IniBool   smtp_use_ssl;
		extern IniString smtp_user;
		extern IniString smtp_password;
		extern IniString smtp_from;
		extern IniString smtp_sender;
		extern IniInt    filter_cache_size;
		extern IniInt    filter_cache_expiration;
		extern IniInt    rss_max_age;
		extern IniInt    rss_max_count;
	}
}

enum {
	WD_INPROGRESS=1,
	WD_DONE=4
};

struct Watchdog : SkylarkApp {
	DynamicSqlSession sql;
	
	virtual void WorkThread();
	virtual void SigUsr1();
	
	void SaveSchema();
	void OpenDB();
	void UpdateDB();
	void CloseDB();
	
	void SetAdmin();
	
	Watchdog();
};

 
#endif
