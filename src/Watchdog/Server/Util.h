#ifndef _Watchdog_Server_Util_h_
#define _Watchdog_Server_Util_h_

void CleanResults();
void CleanAuth();

struct CommitFilter {
	String branch;
	String author;
	String path;
	String msg;
	int limit;
	int offset;
	CommitFilter(Http& http, bool skipPaging = false);
	operator SqlBool() const;
};

bool CheckLocal(Http& http);
int& commitcount(bool force = false);
bool CheckAuth(Http& http, Sql& sql);
bool CheckAuth2(Http& http, Sql& sql, int client, const String& action);
void SendEmails(const Vector<String>& to, const Vector<String>& tokens, const String& subject, const String& text, const String& html = "");
void SendEmail(const String& to, const String& token, const String& subject, const String& text, const String& html = "");
ValueArray ParseFilter(const String& Filter);
bool MatchFilter(const ValueMap& m, const String& commit, const String& branch,
                 int client, bool result, const String& author, const String& path);
double SuccessRate(int ok, int fail, int err);
String ComputeStatus(int ok, int fail, int err);
Value ComputeColor(int ok, int fail, int err, bool quoted = false);
void SetComputedAttributes(ValueMap& vm);

namespace Upp { namespace Ini {
	extern IniString output_dir;
}}

#define V2N(X) Nvl(StrInt(X.ToString()))

#endif
