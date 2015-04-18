#include "Server.h"

#define  MODEL <Watchdog/Server/table.sch>
#include <DynamicSql/sch_source.h>
#include <DynamicSql/sch_schema.h>

namespace Upp{ namespace Ini {
	INI_STRING(skylark_root, "", "Application root");
	INI_STRING(db_backend, "sqlite", "Database backend (mysql, sqlite)");
	INI_STRING(db_scripts_dir, "/tmp/wd_db", "Database update scripts directory");
	INI_STRING(dsql_plugin_path, "/tmp", "Directory containing DynamicSql plugins");
	INI_STRING(sqlite_db, "/tmp/watchdog.db", "Database file path for sqlite");
	INI_STRING(sql_user, "root", "MySql user");
	INI_STRING(sql_password, "", "MySql password");
	INI_STRING(sql_database, "watchdog", "MySql database name");
	INI_STRING(sql_host, "localhost", "MySql hostname");
	INI_INT(sql_port, MYSQL_PORT, "MySql port number");
	INI_STRING(sql_socket, "/var/run/mysqld/mysqld.sock", "MySql socket path");
	INI_BOOL(sql_log, true, "Activates logging of MySql queries");
	INI_STRING(log_file, GetExeDirFile("log")+"/"+GetExeTitle()+".log", "Path for the log file");
	INI_BOOL(log_stderr, IFDBG(true, false), "Log to standard error stream");
	INI_INT(log_level, 2, "Logging verbosity (0,1,2)");
	INI_STRING(output_dir, GetExeFilePath()+"/output","Directory where the output logs are stored");
	INI_STRING(server_url, "http://localhost:8001", "Url of the server where the application runs.");
	INI_STRING(svn, ".", "URL/path of the svn repository");
	INI_INT(max_test_time, 86400, "Number of seconds before an 'In progress' record is deleted from results");
	INI_BOOL(debug, false, "Activates debug functions");
	INI_STRING(smtp_host, "127.0.0.1", "SMTP server address");
	INI_INT(smtp_port, 25, "SMTP server port");
	INI_BOOL(smtp_use_ssl, false, "Whether to use SSL to connect to SMTP");
	INI_STRING(smtp_user, "", "SMTP user (if empty, authentication is not attempted)");
	INI_STRING(smtp_password, "", "SMTP password");
	INI_STRING(smtp_from, "Watchdog", "Name to put in the From field of sent emails");
	INI_STRING(smtp_sender, "Watchdog@example.com", "E-mail to put in the From field of sent e-mails");
	INI_INT(filter_cache_size, 16, "Maximum number of items in filter/pagination cache");
	INI_INT(filter_cache_expiration, 30, "Expiration time of filter/pagination cache items (in seconds)");
	INI_INT(rss_max_age, 30, "How old results should be shown in RSS feeds");
	INI_INT(rss_max_count, 500, "Maximal amount of results that can be shown in RSS feeds");
}}

void Watchdog::WorkThread()
{
	SetDateFormat("%1:4d/%2:02d/%3:02d");
	
	OpenDB();
	RunThread();
	CloseDB();
}

void Watchdog::SigUsr1(){
	RLOG("Received SIGUSR1, reopening log file");
	ReopenLog();
	RLOG("Log file reopened on SIGUSR1");
}

void Watchdog::OpenDB(){
	switch(sql.GetDialect()) {
	case MY_SQL:
		if(!sql.MySqlConnect((String)Ini::sql_user,
		                     (String)Ini::sql_password,
		                     (String)Ini::sql_database,
		                     (String)Ini::sql_host,
		                     Ini::sql_port,
		                     (String)Ini::sql_socket)) {
			RLOG("Can't connect to database '" << (String)Ini::sql_database << "'");
			RLOG(sql.GetSession().GetLastError());
			Exit();
		}
		sql.MySqlAutoReconnect();
		SQL = sql.GetMySqlSession();
		break;
	case SQLITE3:
		if(!sql.SqliteOpen((String)Ini::sqlite_db)){
			RLOG("Can't create or open database file '" << (String)Ini::sqlite_db << "'");
			Exit();
		}
		SQL = sql.GetSqlite3Session();
		AddSqliteCompatibilityFunctions(sql);
		break;
	default:
		RLOG("Sorry, SQL dialect '" << (String)Ini::db_backend << "' is not yet supported");
		Exit();
	}
	SQL.GetSession().LogErrors(true);
	SQL.GetSession().SetTrace();
}

void Watchdog::SaveSchema(){
	SqlSchema sch(sql.GetDialect());
	All_Tables(sch, sql.GetDialect());
	String schdir=AppendFileName(Ini::db_scripts_dir, sql.DialectToString());
	RealizeDirectory(schdir);
	sch.SaveNormal(schdir);
}

void Watchdog::UpdateDB(){
	RLOG("Checking for database updates");
	SqlSchema sch(sql.GetDialect());
	All_Tables(sch, sql.GetDialect());
	String schdir=AppendFileName(Ini::db_scripts_dir, sql.DialectToString());
	if(sch.ScriptChanged(SqlSchema::UPGRADE, schdir)){
		RLOG("\tUpgrading schema");
		SqlPerformScript(sch.Upgrade());
	}
	if(sch.ScriptChanged(SqlSchema::ATTRIBUTES, schdir)){
		RLOG("\tUpgrading indices");
		SqlPerformScript(sch.Attributes());
	}
	if(sch.ScriptChanged(SqlSchema::CONFIG, schdir)) {
		RLOG("\tUpgrading config");
		SqlPerformScript(sch.ConfigDrop());
		SqlPerformScript(sch.Config());
	}
}

void Watchdog::CloseDB(){
	int dialect = DynamicSqlSession::StringToDialect((String)Ini::db_backend);
	switch(dialect) {
	case MY_SQL:
		sql.MySqlClose();
		break;
	case SQLITE3:
		sql.SqliteClose();
		break;
	default:
		NEVER();
	}
}

void Watchdog::SetAdmin(){
	String salt = FormatIntBase(Random()+1679616, 36).Right(4);
	
	Client admin;
	bool exists = admin.Load(0);
	if(!exists)
		SQLR.Cancel(); // Crashes when closing DB if we don't call Cancel... WTF?
	admin("SALT", String(salt));
	if (!exists)
		admin("NAME", "Admin");
	Cout() << "Enter name of the administrator account [defaults to '" << admin["NAME"] << "']: ";
	Cout().Flush();
	String ans = ReadStdIn();
	if(!ans.IsEmpty())
		admin("NAME", ans);
	
	if (!exists)
		admin("DESCR", "Administrator account");
	Cout() << "Enter description of the administrator account [defaults to '" << admin["DESCR"] << "']: ";
	Cout().Flush();
	ans = ReadStdIn();
	if(!ans.IsEmpty())
		admin("DESCR", ans);
	
	Cout() << "Enter new administrator password: ";
	Cout().Flush();
	admin("PASSWORD", MD5String(String(salt) + ReadStdIn()));
	
	admin.Save(0);
}

Watchdog::Watchdog() {
	root = Ini::skylark_root;
#ifdef _DEBUG
	use_caching = false;
#endif
	// dialect plugin must be initialized while still in single thread mode
	sql.SetLibraryPath(Ini::dsql_plugin_path);
	sql.SetDialect(DynamicSqlSession::StringToDialect((String)Ini::db_backend));
}

#ifdef flagMAIN

CONSOLE_APP_MAIN {
	SetDateFormat("%1:4d/%2:02d/%3:02d");
	StdLogSetup(LOG_CERR|LOG_TIMESTAMP);
	RLOG(" === INITIALIZING WATCHDOG === ");
	
	const Vector<String>& cmd=CommandLine();
	LoadConfiguration(cmd.GetCount()?cmd[0]:"");
	
	StdLogSetup(LOG_FILE
	           |LOG_TIMESTAMP
	           |LOG_APPEND
	           |(Ini::log_stderr?LOG_CERR:0),
	           (String)Ini::log_file);
	Smtp::Trace(Ini::log_level == 2);
	
	RLOG(GetIniInfoFormatted());
	RDUMPM(Environment());
	
	Watchdog wd;
	wd.OpenDB();
	wd.UpdateDB();
	wd.SaveSchema();
	
	if(cmd.GetCount() > 1 && cmd[1] == "--set-admin") {
		wd.SetAdmin();
		wd.CloseDB();
		return;
	}
	
	wd.CloseDB();
	
	RLOG(" === STARTING WATCHDOG === ");
	
	wd.Run();
}

#endif
