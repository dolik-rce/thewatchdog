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
	unsigned GetHash() const;
	operator SqlBool() const;
private:
	struct CacheEntry : Moveable<CacheEntry> {
		Time stamp;
		Time used;
		unsigned hash;
		int count;
		CacheEntry(const CommitFilter& f);
	};
	static Vector<CacheEntry> cache;
	int FindInCache(unsigned hash,const Time& maxage) const;
	int commitcount();
};

bool CheckLocal(Http& http);
bool CheckAuth(Http& http, Sql& sql, int client, const String& action);
ValueArray ParseFilter(const String& Filter);
bool MatchFilter(const String& filter, const String& commit, const String& branch,
                 int client, bool result, const String& author, const String& path);
String SuccessRate(int status, int ok, int fail, int err);
String ComputeStatus(int status, int ok, int fail, int err);
Value ComputeColor(int status, int ok, int fail, int err, bool quoted = false);
void SetComputedAttributes(ValueMap& vm, int status = 0, const String& suffix = "");
void SetDuration(ValueMap& vm, int status);

namespace Upp { namespace Ini {
	extern IniString output_dir;
}}

#define V2N(X) Nvl(StrInt(X.ToString()))

#endif
