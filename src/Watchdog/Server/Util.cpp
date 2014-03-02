#include "Server.h"
#include <plugin/pcre/Pcre.h>

void CleanResults(){
	SQL * Delete(RESULT)
	       .Where(STATUS==WD_INPROGRESS && START < GetSysTime()-int(Ini::max_test_time));
}

void CleanAuth(){
	SQL * Delete(AUTH).Where(VALID < GetSysTime()-600);
}

CommitFilter::CommitFilter(Http& http, bool skipPaging){
	//filter
	if(IsNull(http["f_change"])){
		branch = http[".f_branch"];
		msg = http[".f_msg"];
		author = http[".f_author"];
		path = http[".f_path"];
	} else {
		branch = http["f_branch"];
		msg = http["f_msg"];
		author = http["f_author"];
		path = http["f_path"];
		http.SessionSet("f_branch", branch);
		http.SessionSet("f_msg", msg);
		http.SessionSet("f_author", author);
		http.SessionSet("f_path", path);
	}
	//paging
	if (skipPaging)
		return;
	int count = 3;
	int pagesize = max(min(Nvl(http.Int("cnt"), Nvl(http.Int(".cnt"), 10)),100),1);
	http.SessionSet("cnt",pagesize);
	int total = commitcount();
	int hi = total/pagesize;
	int current = Nvl(http.Int("page"), hi);
	
	limit = pagesize;
	offset = pagesize*(hi-current);
	
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
}

CommitFilter::operator SqlBool() const{
	SqlBool res;
	if(!IsEmpty(branch)) {
		if(res.IsEmpty()) res = Regexp(BRANCH, ".*"+branch+".*");
		else res &= Regexp(BRANCH, ".*"+branch+".*");
	}
	if(!IsEmpty(msg)) {
		if(res.IsEmpty()) res = Regexp(MSG, ".*"+msg+".*");
		else res &= Regexp(MSG, ".*"+msg+".*");
	}
	if(!IsEmpty(author)) {
		if(res.IsEmpty()) res = Regexp(AUTHOR, ".*"+author+".*");
		else res &= Regexp(AUTHOR, ".*"+author+".*");
	}
	if(!IsEmpty(path)) {
		if(res.IsEmpty()) res = Regexp(PATH, ".*"+path+".*");
		else res &= Regexp(PATH, ".*"+path+".*");
	}
	if(res.IsEmpty())
		res.SetTrue();
	return res;
}

unsigned CommitFilter::GetHash() const {
	return CombineHash(branch, author, path, msg);
}

CommitFilter::CacheEntry::CacheEntry(const CommitFilter& f) {
	hash = f.GetHash();
	used = stamp = GetSysTime();
	Sql sql;
	sql * Select(Count(SqlAll()).As("CNT"))
	      .From(COMMITS)
	      .Where(f);
	ValueMap vm;
	if(sql.Fetch(vm))
		count = vm["CNT"];
	else
		count = 0;
	LOG("CREATE "<<hash<<"@"<<stamp);
}

int CommitFilter::FindInCache(unsigned hash, const Time& maxage) const {
	// first, we search for item in cache
	for(int i = 0; i < cache.GetCount(); ++i){
		if(cache[i].hash == hash){
			// found the item...
			if(cache[i].stamp < maxage){
				// ... but it is expired, so we replace it by refreshed version
				cache[i] = CacheEntry(*this);
			} else {
				// .. and it is not expired, we only update the usage timestamp
				cache[i].used = GetSysTime();
			}
			return i;
		}
	}
	// if we get here, we know the searched entry is not in the cache and we have to add it
	if(Ini::filter_cache_size == cache.GetCount()){
		// cache is full, we first try to drop expired items
		Vector<int> rem;
		for(int i = 0; i < cache.GetCount(); ++i){
			if(cache[i].stamp < maxage)
				rem.Add(i);
		}
		if(rem.GetCount() > 0){
			// found some expired items, drop them and add fresh entry
			cache.Remove(rem);
			cache.Add(CacheEntry(*this));
			return cache.GetCount()-1;
		} else {
			// no items are expired, we have to replace the least recently used item
			int oldest = 0;
			for(int i = 1; i < cache.GetCount(); i++){
				if(cache[oldest].used > cache[i].used)
					oldest = i;
			}
			cache[oldest] = CacheEntry(*this);
			return oldest;
		}
	} else {
		// there is enough space in the cache, just add new item
		cache.Add(CacheEntry(*this));
		return cache.GetCount()-1;
	}
}

Vector<CommitFilter::CacheEntry> CommitFilter::cache;

int CommitFilter::commitcount(){
	int p = FindInCache(GetHash(), GetSysTime()-Ini::filter_cache_expiration);
	return cache[p].count;
}

bool CheckLocal(Http& http){
	if(http.GetHeader("host").StartsWith("localhost")
	 ||http.GetHeader("host").StartsWith("127.0.0.1"))
		return true;
	http.Response(403, "Forbiden");
	return false;
}

bool CheckAuth(Http& http, Sql& sql, int client, const String& action){
	#define AUTHLOG(X) RLOG("AUTH "<<X<<" (client:"<<cid<<", action:"<<http["wd_action"]<<"ip:"<<http.GetPeerAddr()<<")")
	int cid=http.Int("client_id");
	if(cid!=client && cid!=0){
		http << "Permission denied";
		http.Response(403,"Permission denied");
		AUTHLOG("FAIL [wrong client id]");
		return false;
	}
	if(http["wd_action"] != action) {
		http << "Auth FAIL (action)";
		http.Response(403,"Permission denied");
		AUTHLOG("FAIL [wrong action]");
		return false;
	}
	String nonce = http["wd_nonce"];
	String auth = http["wd_auth"];
	sql * Delete(AUTH).Where(NONCE==nonce);
	if(sql.GetRowsProcessed() == 0){
		http << "Auth FAIL (nonce)";
		http.Response(403,"Permission denied");
		AUTHLOG("FAIL [wrong nonce]");
		return false;
	}
	sql * Select(PASSWORD)
	      .From(CLIENT).Where(ID == http.Int("client_id"));
	String pwd;
	sql.Fetch(pwd);
	if (auth!=MD5String(nonce+String(http["wd_action"])+pwd)) {
		http << "Auth FAIL (auth)";
		http.Response(403,"Permission denied");
		AUTHLOG("FAIL [wrong password]");
		return false;
	}
	AUTHLOG("OK");
	return true;
	#undef AUTHLOG
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
		int client, bool result, const String& author, const String& path){
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
			if (!((v[i].EndsWith("=ok") && result)
			   || (v[i].EndsWith("=failed") && !result)))
				return false;
		}
	}
	return true;
}

String SuccessRate(int status, int ok, int fail, int err){
	if (status != WD_DONE || ok+fail+err == 0)
		return "";
	return DblStr(roundr(100.0*ok/(ok+fail+err), 2))+"%";
}

String ComputeStatus(int status, int ok, int fail, int err){
	if (status == WD_INPROGRESS)
		return "In progress";
	if (ok+err+fail == 0)
		return "No results";
	if (status != WD_DONE)
		return "";
	if (err>0)
		return "Error";
	if (fail>0)
		return "Failed";
	return "OK";
}

Value ComputeColor(int status, int ok, int fail, int err, bool quoted){
	if(status == WD_INPROGRESS)
		return Raw(quoted?"\"#88F\"":"s88F");
	if (ok+fail+err == 0)
		return Raw("");
	double norm =  1.0 / (ok+fail+err);
	int r = 0x7 * fail * norm + 0x8;
	int g = 0x7 * ok * norm + 0x8;
	int b = 0x8;
	return Raw(Format(quoted?"\"#%X%X%X\"":"s%X%X%X", r, g, b));
}

void SetComputedAttributes(ValueMap& vm, int status, const String& suffix) {
	int st = status ? status : V2N(vm["STATUS"+suffix]);
	int ok = V2N(vm["OK"+suffix]);
	int fail = V2N(vm["FAIL"+suffix]);
	int err = V2N(vm["ERR"+suffix]);
	vm.Add("RATE"+suffix, SuccessRate(st, ok, fail, err));
	vm.Add("COLOR"+suffix, ComputeColor(st, ok, fail, err));
	vm.Add("STATUSSTR"+suffix, ComputeStatus(st, ok, fail, err));
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
