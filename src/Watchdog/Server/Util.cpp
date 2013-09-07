#include "Server.h"
#include <plugin/pcre/Pcre.h>

void CleanResults(){
	SQL * Delete(RESULT)
	       .Where(STATUS==WD_INPROGRESS && START < GetSysTime()-int(Ini::max_test_time));
}

void CleanAuth(){
	SQL * Delete(AUTH).Where(VALID < GetSysTime()-600);
}

PageInfo Paging(Http& http){
	int count = 3;
	int pagesize = max(min(Nvl(http.Int("cnt"), Nvl(http.Int(".cnt"), 10)),100),1);
	http.SessionSet("cnt",pagesize);
	int total = commitcount();
	int hi = total/pagesize;
	int current = Nvl(http.Int("page"), hi);
	
	PageInfo result;
	result.limit = pagesize;
	result.offset = pagesize*(hi-current);
	
	ValueArray va;
	ValueMap vm;
	
	if(current != hi){
		vm.Set("TEXT", "<< Newest");
		va.Add(vm);
	}
	for(int i = min(current+count, hi); i > current; --i){
		vm.Set("PAGE", i);
		vm.Set("TEXT", IntStr(i));
		va.Add(vm);
	}
	if(current < hi){
		vm.Set("PAGE", current + 1);
		vm.Set("TEXT", "< Newer");
		va.Add(vm);
	}
	{
		vm.Set("PAGE", "current");
		vm.Set("TEXT", IntStr(current));
		va.Add(vm);
	}
	if(current > 0){
		vm.Set("PAGE", current - 1);
		vm.Set("TEXT", "Older >");
		va.Add(vm);
	}
	for(int i = current - 1; i > max(current-(count+1), 0); --i){
		vm.Set("PAGE", i);
		vm.Set("TEXT", IntStr(i));
		va.Add(vm);
	}
	if(current != 0){
		vm.Set("PAGE", 0);
		vm.Set("TEXT", "Oldest >>");
		va.Add(vm);
	}
	http("PAGING", va);
	return result;
}

bool CheckLocal(Http& http){
	if(http.GetHeader("host").StartsWith("localhost")
	 ||http.GetHeader("host").StartsWith("127.0.0.1"))
		return true;
	http.Response(403, "Forbiden");
	return false;
}

int& commitcount(bool force){
	static Time last(Null);
	static int count(0);
	if(force || GetSysTime() - last > 15){
		Sql sql;
		sql * Select(Count(SqlSet(1))).From(COMMITS);
		if(sql.Fetch()){
			count = sql[0];
			last = GetSysTime();
		} else {
			count = 0;
			last = Null;
		}
	}
	return count;
};

bool CheckAuth(Http& http, Sql& sql){
	String nonce = http.GetHeader("wd-nonce");
	sql * Delete(AUTH).Where(NONCE==nonce);
	if(sql.GetRowsProcessed() == 0){
		http << "Auth FAIL";
		http.Response(403,"Auth FAIL (nonce)");
		return false;
	}
	sql * Select(PASSWORD)
	      .From(CLIENT).Where(ID == http.Int("client_id"));
	String pwd;
	sql.Fetch(pwd);
	if (http.GetHeader("wd-auth")!=MD5String(nonce+pwd)) {
		http << "Auth FAIL";
		http.Response(403,"Auth FAIL (auth)");
		return false;
	}
	return true;
}

bool CheckAuth2(Http& http, Sql& sql, int client, const String& action){
	if(http.Int("client_id")!=client && http.Int("client_id")!=0){
		http << "Permission denied";
		http.Response(403,"Permission denied");
		return false;
	}
	if(http["wd_action"] != action) {
		http << "Auth FAIL (action)";
		http.Response(403,"Permission denied");
		return false;
	}
	String nonce = http["wd_nonce"];
	String auth = http["wd_auth"];
	sql * Delete(AUTH).Where(NONCE==nonce);
	if(sql.GetRowsProcessed() == 0){
		http << "Auth FAIL (nonce)";
		http.Response(403,"Permission denied");
		return false;
	}
	sql * Select(PASSWORD)
	      .From(CLIENT).Where(ID == http.Int("client_id"));
	String pwd;
	sql.Fetch(pwd);
	if (auth!=MD5String(nonce+String(http["wd_action"])+pwd)) {
		http << "Auth FAIL (auth)";
		http.Response(403,"Permission denied");
		return false;
	}
	return true;
}

void SendEmails(const Vector<String>& to, const Vector<String>& tokens, const String& subject, const String& text, const String& html){
	RLOG("Sending emails");
	Smtp mail;
	
	mail.Host(Ini::smtp_host)
	    .Port(Ini::smtp_port)
	    .SSL(Ini::smtp_use_ssl);
	if(!IsEmpty(Ini::smtp_user))
	   mail.Auth(Ini::smtp_user, Ini::smtp_password);
	String body, htmlbody;
	for(int i = 0; i < to.GetCount(); ++i){
		body = text;
		body.Replace("@token@",tokens[i]);
		mail.From(Ini::smtp_sender, Ini::smtp_from)
		    .To(to[i])
		    .Subject(subject)
		    .Body(body);
		if(!IsEmpty(html)){
			htmlbody = html;
			htmlbody.Replace("@token@",tokens[i]);
			mail.Body(htmlbody, "text/html");
		}
		if(mail.Send())
		    RLOG("Email successfully sent to " << to[i]);
		else
		    RLOG("Email sending failed: " << mail.GetError());
	}
}
void SendEmail(const String& to, const String& token, const String& subject, const String& text, const String& html){
	Vector<String> v1;
	v1.Add(to);
	Vector<String> v2;
	v2.Add(token);
	SendEmails(v1, v2, subject, text, html);
}

ValueArray ParseFilter(const String& filter){
	Vector<String> v = Split(filter,"&");
	ValueArray res;
	if(v.IsEmpty()) {
		res.Add("All the results");
		return res;
	}
	for(int i = 0; i < v.GetCount(); i++){
		if(v[i].StartsWith("author=")){
			v[i].Replace("author=","Author is ");
			res.Add(v[i]);
		} else if(v[i].StartsWith("path=")){
			v[i].Replace("path=","Commit affects path ");
			res.Add(v[i]);
		} else if(v[i].StartsWith("client=")){
			v[i].Replace("client=","");
			Sql sql;
			sql * Select(NAME).From(CLIENT).Where(ID==StrInt(v[i]));
			String name;
			sql.Fetch(name);
			res.Add("Client is " + name);
		} else if(v[i].StartsWith("status=")){
			res.Add(v[i].EndsWith("=ok")?"Only successfull":"Only failed");
		}
	}
	return res;
}

bool MatchFilter(const ValueMap& m, const String& commit, const String& branch, 
		int client, int result, const String& author, const String& path){
	Vector<String> v = Split(AsString(m["FILTER"]),"&");
	if(v.IsEmpty())
		return true;
	for(int i = 0; i < v.GetCount(); i++){
		if(v[i].StartsWith("author=")){
			v[i].Replace("author=","");
			if(author != v[i])
				return false;
		} else if(v[i].StartsWith("path=")){
			v[i].Replace("path=","");
			RegExp re(v[i]);
			if (re.Match(path))
				return false;
		} else if(v[i].StartsWith("branch=")){
			v[i].Replace("branch=","");
			RegExp re(v[i]);
			if (re.Match(branch))
				return false;
		} else if(v[i].StartsWith("client=")){
			v[i].Replace("client=","");
			if(StrInt(v[i]) != client)
				return false;
		} else if(v[i].StartsWith("status=")){
			if (!((v[i].EndsWith("=ok") && result == WD_DONE)
			   || (v[i].EndsWith("=failed") && result != WD_DONE)))
				return false;
		}
	}
	return true;
}

double SuccessRate(int ok, int fail, int err){
	if (ok+fail+err == 0)
		return 0;
	return roundr(100.0*ok/(ok+fail+err), 2);
}

int ComputeStatus(int ok, int fail, int err){
	if (err>0) 
		return WD_ERROR;
	if (fail>0)
		return WD_FAILED;
	return WD_DONE;
}

Value ComputeColor(int ok, int fail, int err, bool quoted){
	if (ok+fail+err == 0)
		return Raw("");
	double norm =  1.0 / (ok+fail+err);
	int r = 0x7 * fail * norm + 0x8;
	int g = 0x7 * ok * norm + 0x8;
	int b = 0x8;
	return Raw(Format(quoted?"\"#%X%X%X\"":"s%X%X%X", r, g, b));
}

void SetComputedAttributes(ValueMap& vm) {
	vm.Add("RATE", SuccessRate(V2N(vm["OK"]), V2N(vm["FAIL"]), V2N(vm["ERR"])));
	vm.Add("COLOR", ComputeColor(V2N(vm["OK"]), V2N(vm["FAIL"]), V2N(vm["ERR"])));
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
	int force = arg.GetCount() ? int(arg[0]) : -1;
	if (!(force == 1 || (force != 0 && Ini::debug)))
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
