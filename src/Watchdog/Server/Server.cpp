#include "Server.h"

#define  MODEL <Watchdog/Server/table.sch>
#define  SCHEMADIALECT <MySql/MySqlSchema.h>
#include <Sql/sch_schema.h>
#include <Sql/sch_source.h>

namespace Upp{ namespace Ini {
	INI_STRING(sql_user, "root", "MySql user");
	INI_STRING(sql_password, "", "MySql password");
	INI_STRING(sql_database, "watchdog", "MySql database name");
	INI_STRING(sql_host, "localhost", "MySql hostname");
	INI_INT(sql_port, MYSQL_PORT, "MySql port number");
	INI_STRING(sql_socket, "/var/run/mysqld/mysqld.sock", "MySql socket path");
	INI_BOOL(sql_log, true, "Activates logging of MySql queries");
	INI_STRING(log_file, GetExeDirFile("log")+"/"+GetExeTitle()+".log", "Path for the log file");
	INI_INT(log_level, 2, "Logging verbosity (0,1,2)");
	INI_STRING(output_dir,GetExeFilePath()+"/output","Directory where the output logs are stored");
	INI_STRING(server_url,"http://localhost:8001", "Url of the server where the application runs.");
	INI_STRING(svn, ".", "URL/path of the svn repository");
	INI_INT(max_build_time, 86400, "Number of seconds before an 'In progress' record is deleted from results");
	INI_BOOL(debug, false, "Activates debug functions");
	INI_STRING(smtp_host, "127.0.0.1", "SMTP server address");
	INI_INT(smtp_port, 25, "SMTP server port");
	INI_BOOL(smtp_use_ssl, false, "Whether to use SSL to connect to SMTP");
	INI_STRING(smtp_user, "", "SMTP user (if empty, authentication is not attempted)");
	INI_STRING(smtp_password, "", "SMTP password");
	INI_STRING(smtp_from, "Watchdog", "Name to put in the From field of sent emails");
	INI_STRING(smtp_sender, "Watchdog@example.com", "E-mail to put in the From field of sent e-mails");
}}

void OpenSQL(MySqlSession& session)
{
	if(!session.Connect((String)Ini::sql_user,
                        (String)Ini::sql_password,
                        (String)Ini::sql_database,
                        (String)Ini::sql_host,
                        Ini::sql_port,
                        (String)Ini::sql_socket)) {
		RLOG("Can't connect to database");
		RLOG(session.GetLastError());
		Exit(1);
	}
	SQL = session;
}

void Watchdog::WorkThread()
{
	SetDateFormat("%1:4d/%2:02d/%3:02d");
	MySqlSession mysql;
	if(Ini::sql_log){
		mysql.LogErrors();
		mysql.SetTrace();
	}
	OpenSQL(mysql);
	RunThread();
}

void Watchdog::SigUsr1(){
	RLOG("Received SIGUSR1, reopening log file");
	ReopenLog();
	RLOG("Log file reopened on SIGUSR1");
}

void InitDB()
{
	MySqlSession sqlsession;
	OpenSQL(sqlsession);
	SqlSchema sch(MY_SQL);
	All_Tables(sch);
	SqlPerformScript(sch.Upgrade());
	SqlPerformScript(sch.Attributes());
//	Sql sql;
//	sql * Insert(CLIENT)(NAME, "Basic apps")
//	                    (PASSWORD, "aaa")
//	                    (DESCR, "Testing client")
//	                    (SRC, "/trunk/uppsrc");
//	sql * Insert(CLIENT)(NAME, "Launchpad")
//	                    (PASSWORD, "bbb")
//	                    (DESCR, "Another testing client")
//	                    (SRC, "/trunk/");
}

Value Duration(const Vector<Value>& arg, const Renderer *)
{
	int t = arg[0];
	if (t<=0)
		return "";
	else if (t<60)
		return Format("%d`s", t);
	else if (t<3600)
		return Format("%d`m %d`s", t/60, t%60);
	else
		return Format("%d`h %d`m %d`s", t/3600, (t%3600)/60, (t%3600)%60);
}

Value Email(const Vector<Value>& arg, const Renderer *)
{
	String name;
	switch(arg.GetCount()) {
	case 1:
		name = arg[0];
		break;
	case 2:
		name = arg[1];
		break;
	case 0:
	default:
		throw Exc("email: wrong number of arguments");
	}
	String addr;
	addr.Cat() << "<a href=\"mailto:" << arg[0] << "\">" << name << "</a>";
	RawHtmlText r;
	r.text.Cat("<script type=\"text/javascript\">document.write('");
	for(int i = addr.GetCount()-1; i >= 0; --i)
		r.text.Cat(addr[i]);
	r.text.Cat("'.split('').reverse().join(''));</script>");
	return RawPickToValue(r);
}

Value Dbg(const Vector<Value>& arg, const Renderer *r)
{
	if(!Ini::debug)
		return Value();
	String html;
	if(r) {
		const VectorMap<String, Value>& set = r->Variables();
		html << "<div class=\"dbg\"><table border='1'><tr><th>ID</th><th>VALUE</th></tr>";
		for(int i = 0; i < set.GetCount(); i++)
			html << "<tr><td>"
			     << EscapeHtml(set.GetKey(i))
			     << "</td><td><pre class=\"prewrap\">"
			     << EscapeHtml(AsString(set[i]))
			     << "</pre></td></tr>"
			;
		html << "</table></div>";
	}
	return Raw(html);
}

INITBLOCK {
	Compiler::Register("Duration", Duration);
	Compiler::Register("email", Email);
	Compiler::Register("dbg", Dbg);
};

Watchdog::Watchdog() {
	root = "";
#ifdef _DEBUG
	use_caching = false;
#endif
	MySqlSession sqlsession;
	OpenSQL(sqlsession);
}

#ifdef flagMAIN

namespace Upp { namespace Ini {
	extern IniString path;
}}

CONSOLE_APP_MAIN{
	const Vector<String>& cmd = CommandLine();
	
	String ini;
	if(cmd.GetCount())
		ini = cmd[0];
	else
		ini = GetDataFile("Server.ini");
	
	RLOG("Loading configuration from '" << ini << "'");
	SetIniFile(ini);
	
	SetDateFormat("%1:4d/%2:02d/%3:02d");
	
	StdLogSetup(LOG_FILE
	           |LOG_TIMESTAMP
	           |LOG_APPEND
#ifdef _DEBUG
	           |LOG_CERR
#endif
	           , (String)Ini::log_file);
	Smtp::Trace(Ini::log_level == 2);
	
	//InitDB(); //only for development phase
#ifdef _DEBUG
	Ini::path = String(Ini::path) + ";" + GetFileDirectory(__FILE__) + "static";
#endif
	RLOG(" === STARTING WATCHDOG === ");
	RLOG(GetIniInfoFormatted());
	
	Watchdog().Run();
}

#endif
