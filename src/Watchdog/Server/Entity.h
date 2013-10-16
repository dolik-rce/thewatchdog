#ifndef _Watchdog_Server_Client_h_
#define _Watchdog_Server_Client_h_

#include "Sql.h"

template<class T>
struct Entity {
	ValueMap data;
	
	Entity<T>& operator()(const char* k, const Value& v) { data.Set(k, v); return *this; }
	const Value& operator[](const char* k) { return data[k]; }
	String ToString() const {
		String s;
		for(int i = 0; i < data.GetCount(); i++)
			s << "\n'" << data.GetKey(i) << "': " << data.GetValue(i);
		return s;
	}
};

struct Client : public Entity<Client>, public Moveable<Client> {
	bool Load(int id);
	void SetAuthInfo(String& salts, ValueMap& clients);
	ValueArray FetchResults(const CommitFilter& f) const;
	static ValueMap LoadAll();
	static void Delete(int id);
	static void UpdateActivity(int id, bool work = false);
	void Save(int force_id = -1);
};

struct Commit : public Entity<Commit>, public Moveable<Commit> {
	bool Load(const String& uid);
	ValueArray FetchResults() const;
	static ValueArray LoadPage(const CommitFilter& f);
	void Save();
};

struct Result : public Entity<Result>, public Moveable<Result> {
	bool Load(const String& uid, int id);
	static ValueMap LoadPage(const CommitFilter& f);
	String FetchOutput() const;
	static void Delete(const String& uid, int id);
	void Save();
};

struct Branch : public Entity<Branch>, public Moveable<Branch> {
	static ValueMap LoadAll();
	static void Delete(const String& branch);
};

#endif
