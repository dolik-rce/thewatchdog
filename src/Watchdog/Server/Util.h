#ifndef _Watchdog_Server_Util_h_
#define _Watchdog_Server_Util_h_

void UpdateLogs();
void CleanResults();
void CleanAuth();
VectorMap<String,int>  Paging(Http& http);
bool CheckLocal(Http& http);
int& lastrev();
bool CheckAuth(Http& http, Sql& sql);
bool CheckAuth2(Http& http, Sql& sql, int client, const String& action);
void SendEmails(const Vector<String>& to, const Vector<String>& tokens, const String& subject, const String& text, const String& html = "");
void SendEmail(const String& to, const String& token, const String& subject, const String& text, const String& html = "");
ValueArray ParseFilter(const String& Filter);
bool MatchFilter(const ValueMap& m, int revision, int client, int result, const String& author, const String& path);
SqlVal SqlEmptyString();

namespace Upp { namespace Ini {
	extern IniString output_dir;
}}

#endif
