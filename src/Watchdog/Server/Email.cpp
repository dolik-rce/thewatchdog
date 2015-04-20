#include "Server.h"

using namespace Upp;

void SendEmail(const String& to, const String& subject, const String& text, const String& html) {
	RLOG("Sending email '"<<subject<<"' to "<<to);
	Smtp mail;
	mail.Host(Ini::smtp_host)
	    .Port(Ini::smtp_port)
	    .SSL(Ini::smtp_use_ssl)
	    .From(Ini::smtp_sender, Ini::smtp_from)
	    .To(to)
	    .Subject(subject)
	    .Body(text);

	if(!IsEmpty(Ini::smtp_user))
		mail.Auth(Ini::smtp_user, Ini::smtp_password);

	if(!IsEmpty(html)){
		mail.Body(html, "text/html");
	}
	if(mail.Send())
	    RLOG("Email successfully sent");
	else
	    RLOG("Email sending failed: " << mail.GetError());
}

void SendEmails(const Vector<String>& to, const Vector<String>& tokens, const String& subject, const String& text, const String& html){
	RLOG("Sending emails");
	String body, htmlbody;
	for(int i = 0; i < to.GetCount(); ++i){
		body = text;
		body.Replace("@token@",tokens[i]);
		if(!IsEmpty(html)){
			htmlbody = html;
			htmlbody.Replace("@token@",tokens[i]);
		}
		SendEmail(to[i], subject, body, htmlbody);
	}
}

void SendResultMails(Http& http, const String& commit, int cid, int ok, int failures, int errors) {
	SQLR * Select(UID, BRANCH, CMT, DT, AUTHOR, MSG, PATH,
	              ID, NAME, DESCR, SRC,
	              START, FINISHED, STATUS,
	              TimeDiff(START, FINISHED).As("DURATION"))
	      .From(RESULT)
	      .InnerJoin(COMMITS).On(CMT_UID == UID)
	      .InnerJoin(CLIENT).On(ID == CLIENT_ID)
	      .Where(CMT_UID==commit && CLIENT_ID == cid);
	ValueMap data;
	SQLR.Fetch(data);
	data.Add("STATUSSTR", ComputeStatus(WD_DONE, ok, failures, errors));
	http("DATA", data)("SELF", String(Ini::server_url));
	String html = http.RenderString("templates/resultmail");
	http("PLAIN",1);
	String text = http.RenderString("templates/resultmail");

	ValueMap m;
	Vector<String> to, tokens;
	SQLR * Select(EMAIL,FILTER,SqlFunc("md5",Concat(SqlSet(EMAIL,FILTER,TOKEN,FREQ))).As("TOK"))
	       .From(MAIL)
	       .Where(FREQ == "each");
	while(SQLR.Fetch(m)){
		if(MatchFilter(m["FILTER"], commit, data["BRANCH"], cid, errors+failures==0, data["AUTHOR"], data["PATH"])){
			to.Add(m["EMAIL"]);
			tokens.Add(m["TOK"]);
		}
	}
	SendEmails(to, tokens, Format("%s %s@%s", data["NAME"], data["CMT"], data["BRANCH"]), text, html);
}

bool GenerateDailyMail(Http& http, const String& filter, const String& token, String& text, String& html) {
	Time now = GetUtcTime();
	Date to(now.year, now.month, now.day);
	Date from = to - 1;

	SQLR * Select(CLIENT_ID, BRANCH, NAME,
	              SqlSum(OK).As("OK"), SqlSum(ERR).As("ERR"), SqlSum(FAIL).As("FAIL"), SqlSum(SKIP).As("SKIP"),
	              SqlSum(1).As("CNT"), Avg(TimeDiff(START, FINISHED)).As("AVG_DURATION"),
	              SqlMax(If(ERR+FAIL==0, DT, Null)).As("LAST_OK"), SqlMax(If(ERR+FAIL==0, Null, DT)).As("LAST_FAIL"))
	      .From(RESULT)
	      .InnerJoin(COMMITS).On(CMT_UID == UID)
	      .InnerJoin(CLIENT).On(ID == CLIENT_ID)
	      .Where(DT >= from && DT < to && STATUS == WD_DONE && SqlFilter(filter))
	      .GroupBy(CLIENT_ID, BRANCH);

	ValueMap data;
	ValueMap clients;
	ValueMap vm;
	ValueArray va;
	int prev = Null;
	while(SQLR.Fetch(vm)) {
		SetComputedAttributes(vm, WD_DONE);
		vm.Set("COLOR", ComputeColor(WD_DONE, vm["OK"], vm["FAIL"], vm["ERR"]).To<RawHtmlText>().text.Mid(1));
		clients.Set(vm["CLIENT_ID"], vm["NAME"]);
		if (prev != vm["CLIENT_ID"] && !IsNull(prev)) {
			data.Set(prev, va);
			va.Clear();
		}
		va.Add(vm);
		prev = vm["CLIENT_ID"];
	}
	if (!IsNull(prev))
		data.Set(prev, va);

	if(data.IsEmpty())
		return false;

	http("CLIENTS", clients)
	    ("DATA", data)
	    ("FILTER", ParseFilter(filter))
	    ("TOKEN", token)
	    ("FROM", from)
	    ("TO", to)
	    ("SELF", String(Ini::server_url));
	html = http.RenderString("templates/dailymail");
	http("PLAIN",1);
	text = http.RenderString("templates/dailymail");
	return true;
}
