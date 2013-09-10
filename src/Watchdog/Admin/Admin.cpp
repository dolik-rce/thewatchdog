#include "Admin.h"

bool WatchdogAdmin::GetState(){
	String target = "/api/getstate";
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	req.Post("branches", branches);
	String response = req.Execute();
	if(!req.IsSuccess()){
		RDUMP(response);
		RLOG("Error sending request");
		RLOG((req.IsError() ? req.GetErrorDesc() : AsString(req.GetStatusCode())+':'+req.GetReasonPhrase()));
		return false;
	}
	
	Cout() << response;
	return true;
}

bool WatchdogAdmin::Update(){
	String target = "/api/update";
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	String line = ReadStdIn();
	String data;
	while(!line.IsEmpty()) {
		data << line << '\n';
		line = ReadStdIn();
	}
	req.Post("data", data);
	String response = req.Execute();
	if(!req.IsSuccess()){
		RDUMP(response);
		RLOG("Error sending request");
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
	else if(cmd[i] == "--state" || cmd[i] == "-t")
		SetAction(cmd[i], 1);
	else if(cmd[i] == "--branches" || cmd[i] == "-B") {
		CheckParamCount(cmd, i, 1);
		branches = cmd[++i];
	} else
		WatchdogClient::ParseArgument(i, cmd);
}

bool WatchdogAdmin::ProcessAction() {
	switch (GetAction()) {
	case 'u':
		return Update();
	case 'c':
		return Clean();
	case 't':
		return GetState();
	default:
		return WatchdogClient::ProcessAction();
	}
}

WatchdogAdmin::WatchdogAdmin() : branches(".*") {
	actions.Insert(1) = "\t-u --update\n"
		"\t\tSend a list of new commits from stdin to the server. The format\n"
		"\t\tof the input is \"REV\\tDATE\\tAUTHOR\\tLOGMSG\\tPATH\\tBRANCH\\n\".\n";
	actions.Insert(2) = "\t-c --clean\n"
		"\t\tInstructs server to clean up authorization data and stale jobs\n";
	actions.Insert(3) = "\t-t --state\n"
		"\t\tAsks server about last updates in branches\n";
	options.Insert(1) = "\t-B --branches\n"
		"\t\tRegexp selecting branches, defaults to \".*\", only\n"
		"\t\thonoured with --state\n";
}
