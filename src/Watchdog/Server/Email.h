#ifndef _Watchdog_Server_Email_h_
#define _Watchdog_Server_Email_h_

void SendEmail(const String& to, const String& subject, const String& text, const String& html = "");
void SendEmails(const Vector<String>& to, const Vector<String>& tokens, const String& subject, const String& text, const String& html = "");
void SendResultMails(Http& http, const String& commit, int cid, int ok, int failures, int errors);
bool GenerateDailyMail(Http& http, const String& filter, const String& token, String& text, String& html);

#endif
