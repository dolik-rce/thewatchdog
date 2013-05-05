#include "Client.h"

namespace Upp { namespace Ini {
	INI_STRING(host, "http://localhost:8001/api", "Watchdog API URL");
	INI_INT(client_id, 1, "Watchdog client ID");
	INI_STRING(password, "", "Watchdog client password");
	INI_INT(max_age, -1, "Only commits this many days old can be build (negative value means any age)");
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

bool WatchdogClient::GetWork(int max_age){
	String target = "/api/getwork/" + IntStr(Ini::client_id);
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	if(max_age >= 0)
		req.Post("max_age", IntStr(max_age));
	else if (Ini::max_age >= 0)
		req.Post("max_age", IntStr(Ini::max_age));
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

bool WatchdogClient::AcceptWork(int revision, Time start){
	if(lock){
		if(FileExists(String(Ini::lock_file))) {
			RLOG("Lock file '" << Ini::lock_file << "' found. Another process is probably already running.");
			return false;
		} else
			SaveFile(String(Ini::lock_file), Format("Locked @ %` by client %d\n", GetSysTime(), (int)Ini::client_id));
	}
	
	if(Ini::log_level > 0) 
		Cout() << "Accepting revision " << revision << "\n";
	
	String target = "/api/acceptwork/" + IntStr(Ini::client_id);
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	req.Post("revision", IntStr(revision));
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

bool WatchdogClient::SubmitWork(const int revision, const int result, const int time, const String& output, Time start, Time end){
	if(Ini::log_level > 0) 
		Cout() << "Sending result '" << status(result) << "'\n";

	String target = "/api/submitwork/" + IntStr(Ini::client_id);
	HttpRequest req(Ini::host + target);
	if(!Auth(req, target))
		return false;
	
	req.Post("result", IntStr(result));
	req.Post("time", IntStr(time));
	req.Post("revision", IntStr(revision));
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

WatchdogClient::WatchdogClient(bool lock) : lock(lock) {}
