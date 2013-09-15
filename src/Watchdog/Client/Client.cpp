#include "Client.h"

namespace Upp { namespace Ini {
	INI_STRING(host, "http://localhost:8001/api", "Watchdog API URL");
	INI_INT(client_id, 0, "Watchdog client ID");
	INI_STRING(password, "", "Watchdog client password");
	INI_INT(log_level, 1, "Verbosity (0=errors only, 1=normal, 2=verbose)");
	INI_STRING(session_cookie, "__watchdog_cookie__", "Skylark session cookie ID");
	INI_STRING(lock_file, "/tmp/wd.lock", "Lock file path");
}}

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
	if(!Auth(req, target))
		return false;
	
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

bool WatchdogClient::AcceptWork(const String& commit, Time start){
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

bool WatchdogClient::SubmitWork(const String& commit, const int duration, const String& output, Time start, Time end){
	if (IsEmpty(commit)){
		Cerr() << "ERROR: Wrong value of <commit>\n";
		return false;
	}
	if (duration < 0){
		Cerr() << "ERROR: Wrong value of <duration>\n";
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
	
	req.Post("duration", IntStr(duration));
	req.Post("commit", commit);
	req.Post("output", output);
	if(!IsNull(start)){
		req.Post("start", IntStr64(start.Get()));
	}
	if(!IsNull(end)){
		req.Post("end", IntStr64(end.Get()));
	}
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
	Time start = GetSysTime();
	int result = Sys(command, output);
	int duration = GetSysTime()-start;
	
	if(Ini::log_level > 0) 
		Cout() << "Execution finished after " << duration << " with exit code '" << result << "'\n";
	
	// send the results
	return SubmitWork(commit, duration, output);
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
		CheckParamCount(cmd, i, 4);
		commit = cmd[++i];
		duration = StrInt(cmd[++i]);
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
	} else if(cmd[i] == "--start" || cmd[i] == "-S") {
		CheckParamCount(cmd, i, 1);
		start = ScanTime(cmd[++i]);
	} else if(cmd[i] == "--end" || cmd[i] == "-E") {
		CheckParamCount(cmd, i, 1);
		end = ScanTime(cmd[++i]);
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
		return AcceptWork(commit, start);
	case 's':
		return SubmitWork(commit,
		                  duration,
		                  LoadFile(output));
	case 'r':
		return Run(command);
	default:
		NEVER();
	}
}

void WatchdogClient::SetConfig(const String& fn) {
	if(!FileExists(fn)){
		Cerr() << "ERROR: Configuration file '" << fn << "' not found\n";
		Exit(64);
	}
	SetIniFile(fn);
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
	
	if (cfg.IsEmpty())
		cfg = GetExeDirFile(GetExeTitle()+".ini");
	SetConfig(cfg);
	
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

WatchdogClient::WatchdogClient() : lock(true), action(0) {
	actions.Add() = "\t-h --help\n"
		"\t\tPrints usage information (this text)\n";
	actions.Add() = "\t-g --get\n"
		"\t\tReturns a list of not yet built commits\n";
	actions.Add() = "\t-a --accept <commit>\n"
		"\t\tNotifies server about building <commit>.\n";
	actions.Add() = "\t-s --submit <commit> <duration> <output_file>\n"
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
	options.Add() = "\t-S --start <time>\n"
		"\t\tStart date given in \"YYYY/MM/DD hh:mm:ss\" format, only \n"
		"\t\thonoured with --accept and --submit\n";
	options.Add() = "\t-E --end <time>\n"
		"\t\tEnd date given given in \"YYYY/MM/DD hh:mm:ss\" format, only\n"
		"\t\thonoured with --submit\n";
	options.Add() = "If not supplied, server will use the time of the accept request as start time\n"
		"and the time of submit request as end time.\n";
}
