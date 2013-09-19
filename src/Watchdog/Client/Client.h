#ifndef _Watchdog_Client_Client_h_
#define _Watchdog_Client_Client_h_

#include <Core/Core.h>
using namespace Upp;

namespace Upp { namespace Ini {
	extern IniString host;
	extern IniInt client_id;
	extern IniString password;
	extern IniInt log_level;
	extern IniString session_cookie;
	extern IniString lock_file;
}}

struct WatchdogClient {
	Vector<String> todo;
	bool lock;
	String commit;
	String output;
	String command;
	String cfg;
	Time start;
	Time end;
	
	void Usage(int exitcode = 0) const;
	void CheckParamCount(const Vector<String>& cmd, int current, int count) const;
	void SetAction(const String& value, int offset = 0);
	char GetAction() const;
	
	virtual void ParseArgument(int& i, const Vector<String>& cmd);
	virtual bool ProcessAction();
	virtual void SetConfig(const String& fn);
	virtual void Execute(const Vector<String>& cmd);
	
	virtual bool GetWork();
	virtual bool AcceptWork(const String& commit, Time start=Null);
	virtual bool SubmitWork(const String& commit, const String& output, Time start=Null, Time end=Null);
	virtual bool Run(String command);
	
	WatchdogClient();
	~WatchdogClient() {};
	
protected:
	virtual bool Auth(HttpRequest& req, const String& action="");
	Vector<String> actions;
	Vector<String> options;
	char action;
};

#endif
