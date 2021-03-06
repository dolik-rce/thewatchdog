#include "Sql.h"
#include "Server.h"

SqlStatement InsertIgnore(const SqlInsert& insert) {
	int dialect = (*(Watchdog*)(&SkylarkApp::TheApp())).sql.GetDialect();
	String s = SqlStatement(insert).Get(dialect);
	ASSERT(s.StartsWith("insert "));
	switch(dialect) {
	case SQLITE3:
		s = "insert or ignore " + s.Mid(7);
		break;
	case MY_SQL:
		s = "insert ignore " + s.Mid(7);
		break;
	default:
		NEVER_("InsertIgnore not implemented for this dialect");
	}
	return SqlStatement(s);
}

bool Upsert(Sql& sql, const SqlInsert& insert, const SqlUpdate& update) {
	sql * InsertIgnore(insert);
	if (sql.GetRowsProcessed() == 0) {
		sql * update;
		return false;
	}
	return true;
}

SqlBool Regexp(const SqlVal& a, const SqlVal& b) {
	static SqlVal any("'.*'", SqlS::HIGH);
	return SqlBool(Nvl(a, SqlEmptyString()), " regexp ", If(b == SqlEmptyString(), any, Nvl(b, any)), SqlS::COMP);
}

SqlVal SqlEmptyString(){
	static SqlVal s("''", SqlS::HIGH);
	return s;
}

SqlVal DateSub(const SqlVal& date, const SqlVal& interval) {
	int dialect = (*(Watchdog*)(&SkylarkApp::TheApp())).sql.GetDialect();
	switch(dialect) {
	case SQLITE3:
		return SqlFunc("datetime", SqlSet(date, SqlVal("'-'||" + ~interval, SqlS::HIGH)));
	case MY_SQL:
		return SqlFunc("date_sub", SqlSet(date, interval));
	default:
		NEVER_("DateSub not implemented for this dialect");
		return SqlVal();
	}
}

SqlVal SqlInterval(const SqlVal& count, const String& unit) {
	int dialect = (*(Watchdog*)(&SkylarkApp::TheApp())).sql.GetDialect();
	switch(dialect) {
	case SQLITE3:
		return SqlVal(~count + "||" + ("' " + unit + "'"), SqlS::HIGH);
	case MY_SQL:
		return SqlVal("interval " + ~count + " " + unit, SqlS::HIGH);
	default:
		NEVER_("SqlInterval not implemented for this dialect");
		return SqlVal();
	}
}

SqlVal TimeDiff(const SqlVal& a, const SqlVal& b) {
	int dialect = (*(Watchdog*)(&SkylarkApp::TheApp())).sql.GetDialect();
	switch(dialect) {
	case SQLITE3:
		return SqlFunc("strftime", SqlSet("%s", b)) - SqlFunc("strftime", SqlSet("%s", a));
	case MY_SQL:
		return SqlFunc("timestampdiff", SqlSet(SqlVal("second", SqlS::HIGH), a, b));
	default:
		NEVER_("TimeDiff not implemented for this dialect");
		return SqlVal();
	}
}

String SqlEscape(const Value& in){
	if(in.IsNull() && !in.Is<String>() && !in.Is<WString>()) {
		return "NULL";
	}
	if(in.Is<int>() || in.Is<int64>() || in.Is<double>())
		return in.ToString();
	if(in.Is<Time>() || in.Is<Date>())
		return "\'" + in.ToString() + "\'";
	
	String str = in.ToString();
	const char* s = str.Begin();
	const char* lim = str.End();
	String t;
	t.Cat('\'');
	while(s < lim) {
		switch(*s) {
		case '\a': t.Cat("\\a"); break;
		case '\b': t.Cat("\\b"); break;
		case '\f': t.Cat("\\f"); break;
		case '\t': t.Cat("\\t"); break;
		case '\v': t.Cat("\\v"); break;
		case '\r': t.Cat("\\r"); break;
		case '\'': t.Cat("''"); break;
		case '\\': t.Cat("\\\\"); break;
		case '\n': t.Cat("\\n"); break;
		default:
			if(byte(*s) < 32 || (byte)*s >= 0x7f) {
				char h[4];
				int q = (byte)*s;
				h[0] = '\\';
				h[1] = (3 & (q >> 6)) + '0';
				h[2] = (7 & (q >> 3)) + '0';
				h[3] = (7 & q) + '0';
				t.Cat(h, 4);
			}
			else {
				t.Cat(*s);
			}
			break;
		}
		s++;
	}
	t.Cat('\'');
	return t;
}

SqlBool SqlFilter(const String& filter) {
	Vector<String> v = Split(filter,"&");
	SqlBool res(true);
	if(v.IsEmpty()) {
		return res;
	}
	for(int i = 0; i < v.GetCount(); i++){
		if(v[i].StartsWith("author=")){
			res &= (AUTHOR == v[i].Mid(7));
		} else if(v[i].StartsWith("path=")){
			res &= Regexp(PATH, v[i].Mid(5));
		} else if(v[i].StartsWith("client=")){
			res &= (CLIENT_ID == v[i].Mid(7));
		} else if(v[i].StartsWith("status=")){
			res &= (v[i].EndsWith("=ok") ? (ERR + FAIL == 0) : (ERR + FAIL != 0));
		}
	}
	return res;
}
