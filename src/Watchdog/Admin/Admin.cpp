#include "Admin.h"

bool WatchdogAdmin::Update(){
	String target = "/api/update";
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	String response = req.Execute();
	if(!req.IsSuccess()){
		RDUMP(response);
		RLOG("Error during get work phase");
		RLOG((req.IsError() ? req.GetErrorDesc() : AsString(req.GetStatusCode())+':'+req.GetReasonPhrase()));
		return false;
	}
	return true;
}

bool WatchdogAdmin::Clean(){
	if(Ini::log_level > 0) 
		Cout() << "Instructing server to clean up ...\n";
	
	String target = "/api/clean";
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	String response = req.Execute();
	if(!req.IsSuccess()){
		RDUMP(response);
		RLOG("Error sending request");
		RLOG((req.IsError() ? req.GetErrorDesc() : AsString(req.GetStatusCode())+':'+req.GetReasonPhrase()));
		return false;
	}
	return true;
}

void WatchdogAdmin::ParseArgument(int& i, const Vector<String>& cmd){
	if(cmd[i] == "--clean" || cmd[i] == "-c")
		SetAction(cmd[i]);
	else if(cmd[i] == "--update" || cmd[i] == "-u")
		SetAction(cmd[i]);
	else
		WatchdogClient::ParseArgument(i, cmd);
}

bool WatchdogAdmin::ProcessAction() {
	switch (GetAction()) {
	case 'u':
		return Update();
	case 'c':
		return Clean();
	default:
		return WatchdogClient::ProcessAction();
	}
}

WatchdogAdmin::WatchdogAdmin(){
	actions.Insert(1) = "\t-u --update\n"
		"\t\tInstructs server to check for new commits in tracked svn\n";
	actions.Insert(2) = "\t-c --clean\n"
		"\t\tInstructs server to clean up authorization data and stale jobs\n";
}
