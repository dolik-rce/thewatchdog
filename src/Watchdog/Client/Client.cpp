#include "Client.h"

namespace Upp { namespace Ini {
	INI_STRING(host, "http://localhost:8001/api", "Watchdog API URL");
	INI_INT(client_id, 0, "Watchdog client ID");
	INI_STRING(password, "", "Watchdog client password");
	INI_INT(log_level, 1, "Verbosity (0=errors only, 1=normal, 2=verbose)");
	INI_STRING(session_cookie, "__watchdog_cookie__", "Skylark session cookie ID");
	INI_STRING(lock_file, "/tmp/wd.lock", "Lock file path");
	INI_STRING(name, TrimBoth(LoadFile("/etc/hostname")), "Builder unique identification string");
}}

Time ScanTimeToUtc(const char *s) {
	int offset;
	ONCELOCK {
		offset = GetUtcTime() - GetSysTime();
	}
	Time t = ScanTime(s);
	if (IsNull(t))
		return t;
	return t + offset;
}

bool WatchdogClient::Auth(HttpRequest& req, const String& action){
	HttpRequest auth(Ini::host+("/auth"+action));
	auth.Execute();
	if(!auth.IsSuccess()){
		RLOG("Error during authentication");
		RLOG((auth.IsError() ? auth.GetErrorDesc() : AsString(auth.GetStatusCode())+':'+auth.GetReasonPhrase()));
		return false;
	}
	Vector<String> clients = Split(auth.GetHeader("wd_salts"),"|");
	Vector<String> v;
	for(int i = 0; i < clients.GetCount(); i++){
		v = Split(clients[i],":");
		if(StrInt(v[0]) == Ini::client_id)
			break;
		v.Clear();
	}
	if(v.IsEmpty()){
		RLOG("Client unknown to server");
		return false;
	}
	req.Header("Cookie", Split(auth.GetHeader("set-cookie"),";")[0])
	   .Post("__post_identity__", auth.GetHeader("wd_id"))
	   .Post("wd_auth", MD5String(auth.GetHeader("wd_nonce")+action+MD5String(v[1]+Ini::password)))
	   .Post("wd_nonce", auth.GetHeader("wd_nonce"))
	   .Post("wd_action", action)
	   .Post("client_id", v[0]);
	return true;
}

bool WatchdogClient::GetWork(){
	String target = "/api/getwork/" + IntStr(Ini::client_id);
	HttpRequest req(Ini::host + target);
	
	String resp = req.Execute();
	if(!req.IsSuccess()){
		RDUMP(resp);
		RLOG("Error during get work phase");
		RLOG((req.IsError() ? req.GetErrorDesc() : AsString(req.GetStatusCode())+':'+req.GetReasonPhrase()));
		return false;
	}
	todo = Split(resp,",");
	return true;
}

bool WatchdogClient::AcceptWork(const String& commit){
	if (IsEmpty(commit)){
		Cerr() << "ERROR: Wrong value of <commit>\n";
		return false;
	}
	
	if(lock){
		if(FileExists(String(Ini::lock_file))) {
			RLOG("Lock file '" << Ini::lock_file << "' found. Another process is probably already running.");
			return false;
		} else
			SaveFile(String(Ini::lock_file), Format("Locked @ %` by client %d\n", GetSysTime(), (int)Ini::client_id));
	}
	
	if(Ini::log_level > 0) 
		Cout() << "Accepting commit " << commit << "\n";
	
	String target = "/api/acceptwork/" + IntStr(Ini::client_id);
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	req.Post("builder", name);
	req.Post("commit", commit);
	if(!IsNull(start)){
		req.Post("start", IntStr64(start.Get()));
	}
	req.Execute();
	if(!req.IsSuccess()){
		RLOG("Error during accept work phase");
		RLOG((req.IsError() ? req.GetErrorDesc() : AsString(req.GetStatusCode())+':'+req.GetReasonPhrase()));
		return false;
	}
	return true;
}

bool WatchdogClient::SubmitWork(const String& commit, const String& output){
	if (IsEmpty(commit)){
		Cerr() << "ERROR: Wrong value of <commit>\n";
		return false;
	}
	if (output.IsVoid()){
		Cerr() << "ERROR: Output not available\n";
		return false;
	}
	
	if(Ini::log_level > 0) 
		Cout() << "Sending results\n";

	String target = "/api/submitwork/" + IntStr(Ini::client_id);
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	req.Post("builder", name);
	req.Post("commit", commit);
	req.Post("output", output);
	if(!IsNull(start))
		req.Post("start", IntStr64(start.Get()));
	if(!IsNull(end))
		req.Post("end", IntStr64(end.Get()));
	if(!IsNull(ok))
		req.Post("ok", IntStr(ok));
	if(!IsNull(errors))
		req.Post("errors", IntStr(errors));
	if(!IsNull(failures))
		req.Post("failures", IntStr(failures));
	if(!IsNull(skipped))
		req.Post("skipped", IntStr(skipped));
	req.Execute();
	if(lock && FileExists(String(Ini::lock_file))){
		DeleteFile(String(Ini::lock_file));
	}
	if(!req.IsSuccess()){
		RLOG("Error during submit work phase");
		RLOG((req.IsError() ? req.GetErrorDesc() : AsString(req.GetStatusCode())+':'+req.GetReasonPhrase()));
		return false;
	}
	return true;
}

bool WatchdogClient::Run(String command){
	// ask for work
	if(!GetWork())
		return false;
	
	// select what to do
	if(todo.GetCount()==0){
		if(Ini::log_level>0) 
			Cout() << "Nothing to do.\n";
		return false;
	}
	String commit = todo.Top();
	if(Ini::log_level > 0) 
		Cout() << "Commit " << commit << " selected for build\n";
	if(!AcceptWork(commit))
		return false;
	
	// replace @commit
	command.Replace("@commit", commit);
	
	// do work
	if(Ini::log_level > 0) 
		Cout() << "Executing command '" << command << "'\n";
	String output;
	start = GetUtcTime();
	int result = Sys(command, output);
	end = GetUtcTime();
	
	if(Ini::log_level > 0) {
		Cout() << "Execution finished after " << (end-start) << " seconds with exit code '" << result << "'\n";
		Cout() << "Parsing command output\n";
	}
	int pos = output.Find("\n@");
	while (pos >= 0) {
		String id;
		const char* c = output.Begin() + pos + 2;
		for (; *c != '=' && *c != '\n' && *c != 0; ++c)
			id.Cat(*c);
		if(id == "ok") {
			ok = max(ScanInt(++c), 0);
		} else if(id == "errors") {
			errors = max(ScanInt(++c), 0);
		} else if(id == "failures") {
			failures = max(ScanInt(++c), 0);
		} else if(id == "skipped") {
			skipped = max(ScanInt(++c), 0);
		} else {
			pos = output.Find("\n@", pos + 2);
			continue;
		}
		int endpos = output.Find('\n', pos + 2);
		output.Remove(pos, endpos - pos);
		pos = output.Find("\n@", pos);
	}
	// send the results
	return SubmitWork(commit, output);
}

void WatchdogClient::CheckParamCount(const Vector<String>& cmd, int current, int count) const {
	if(cmd.GetCount() <= current+count) {
		Cerr() << "ERROR: Wrong number of parameters for " << cmd[current] << "\n";
		Usage(1);
	}
}

void WatchdogClient::SetAction(const String& value, int offset){
	if(action == 0) {
		if (value.StartsWith("--")) {
			action=value[2+offset];
		} else {
			action=value[1];
		}
	} else {
		Cerr() << "ERROR: Only one action can be specified\n\n";
		Usage(2);
	}
}

char WatchdogClient::GetAction() const {
	return action;
}

void WatchdogClient::ParseArgument(int& i, const Vector<String>& cmd){
	if(cmd[i] == "--help" || cmd[i] == "-h") {
		Usage(0);
	} else if(cmd[i] == "--get" || cmd[i] == "-g") {
		SetAction(cmd[i]);
	} else if(cmd[i] == "--accept" || cmd[i] == "-a") {
		SetAction(cmd[i]);
		CheckParamCount(cmd, i, 1);
		commit = cmd[++i];
	} else if(cmd[i] == "--submit" || cmd[i] == "-s") {
		SetAction(cmd[i]);
		CheckParamCount(cmd, i, 2);
		commit = cmd[++i];
		output = cmd[++i];
	} else if(cmd[i] == "--run" || cmd[i] == "-r") {
		SetAction(cmd[i]);
		CheckParamCount(cmd, i, 1);
		command = cmd[++i];
	} else if(cmd[i] == "--config" || cmd[i] == "-C") {
		CheckParamCount(cmd, i, 1);
		cfg = cmd[++i];
	} else if(cmd[i] == "--nolock" || cmd[i] == "-L") {
		lock = false;
	} else if(cmd[i] == "--utc" || cmd[i] == "-U") {
		utc = true;
	} else if(cmd[i] == "--start" || cmd[i] == "-S") {
		CheckParamCount(cmd, i, 1);
		if (utc)
			start = ScanTime(cmd[++i]);
		else
			start = ScanTimeToUtc(cmd[++i]);
	} else if(cmd[i] == "--end" || cmd[i] == "-E") {
		CheckParamCount(cmd, i, 1);
		if (utc)
			end = ScanTime(cmd[++i]);
		else
			end = ScanTimeToUtc(cmd[++i]);
	} else if(cmd[i] == "--ok" || cmd[i] == "-O") {
		CheckParamCount(cmd, i, 1);
		ok = StrInt(cmd[++i]);
	} else if(cmd[i] == "--errors" || cmd[i] == "-R") {
		CheckParamCount(cmd, i, 1);
		errors = StrInt(cmd[++i]);
	} else if(cmd[i] == "--failures" || cmd[i] == "-F") {
		CheckParamCount(cmd, i, 1);
		failures = StrInt(cmd[++i]);
	} else if(cmd[i] == "--skipped" || cmd[i] == "-K") {
		CheckParamCount(cmd, i, 1);
		skipped = StrInt(cmd[++i]);
	} else {
		Cerr() << "ERROR: Unknown argument '" << cmd[i] << "'\n\n";
		Usage(3);
	}
}

bool WatchdogClient::ProcessAction(){
	switch (GetAction()) {
	case 0:
		Cerr() << "ERROR: No action given\n\n";
		return false;
	case 'g':
		if(!GetWork())
			return false;
		if(todo.IsEmpty()){
			if(Ini::log_level > 0) 
				Cout() << "Nothing to do.\n";
			return true;
		}
		for(int i = 0; i < todo.GetCount(); i++){
			Cout() << todo[i] << "\n";
		}
		return true;
	case 'a':
		return AcceptWork(commit);
	case 's':
		return SubmitWork(commit, LoadFile(output));
	case 'r':
		return Run(command);
	default:
		NEVER();
	}
}

void WatchdogClient::SetConfig(const String& fn) {
	LoadConfiguration(fn);
	LOG_(Ini::log_level==2, GetIniInfoFormatted());
}

void WatchdogClient::Execute(const Vector<String>& cmd) {
	StdLogSetup(LOG_FILE | LOG_COUT);
	SetDateFormat("%1:4d/%2:02d/%3:02d");
	SetDateScan("ymd");
	
	if(cmd.GetCount() < 1) {
		Usage(4);
	}
	
	for(int i = 0; i < cmd.GetCount(); i++)
		ParseArgument(i, cmd);
	
	SetConfig(cfg);
	name = Ini::name;
	
	if(!ProcessAction())
		Exit(65);
}

void WatchdogClient::Usage(int exitcode) const {
	Stream& s = exitcode==0?Cout():Cerr();
	s << 
	"Usage:\n"
	"\t" << GetExeTitle() << " action [options]\n\n"
	"Actions:\n";
	for(int i = 0; i < actions.GetCount(); i++){
		s << actions[i];
	}
	s << "Options:\n";
	for(int i = 0; i < options.GetCount(); i++){
		s << options[i];
	}
	Exit(exitcode);
}

WatchdogClient::WatchdogClient() : lock(true), utc(false), action(0),
		ok(Null), errors(Null), failures(Null), skipped(Null) {
	actions.Add() = "\t-h --help\n"
		"\t\tPrints usage information (this text)\n";
	actions.Add() = "\t-g --get\n"
		"\t\tReturns a list of not yet built commits\n";
	actions.Add() = "\t-a --accept <commit>\n"
		"\t\tNotifies server about building <commit>.\n";
	actions.Add() = "\t-s --submit <commit> <output_file>\n"
		"\t\tSends results to the server. Optional start and end times can\n"
		"\t\tbe supplied as int64.\n";
	actions.Add() = "\t-r --run <command>\n"
		"\t\tGets and accepts work automatically, then performs <command>\n"
		"\t\twith '@commit' substituted by actual commit identifier\n"
		"\t\tand then submits the results to server.\n";
	options.Add() = "\t-C --config <file>\n"
		"\t\tPath to configuration file\n";
	options.Add() = "\t-L --nolock\n"
		"\t\tDo not use locking (default is to lock before --accept\n"
		"\t\tand unlock after --submit)\n";
	options.Add() = "\t-U --utc\n"
		"\t\tTimes passed to --start and/or --end are in UTC, not local time\n";
	options.Add() = "\t-O --ok <count>\n"
	                "\t-R --errors <count>\n"
	                "\t-F --failures <count>\n"
	                "\t-K --skipped <count>\n"
		"\t\tNumber of tests for each status. Ok defaults to 1, others to 0.\n"
		"\t\tOnly honoured with --submit\n";
	options.Add() = "\t-S --start <time>\n"
		"\t\tStart date given in \"YYYY/MM/DD hh:mm:ss\" format, only \n"
		"\t\thonoured with --accept or --submit\n";
	options.Add() = "\t-E --end <time>\n"
		"\t\tEnd date given given in \"YYYY/MM/DD hh:mm:ss\" format, only\n"
		"\t\thonoured with --submit\n";
	options.Add() = "If not supplied, server will use the time of the accept request as start time\n"
		"and the time of submit request as end time.\n";
}
