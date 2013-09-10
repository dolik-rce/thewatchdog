#include "Server.h"

using namespace Upp;

bool Client::Load(int id) {
	SQLR * Select(SqlAll()).From(CLIENT).Where(ID==id);
	return SQLR.Fetch(data);
}

void Client::SetAuthInfo(String& salts, ValueMap& clients) {
	salts.Cat() << data["ID"] << ':' << data["SALT"] << '|';
	clients.Add(IntStr(data["ID"])+";"+String(data["SALT"]),data["NAME"]);
}

ValueArray Client::FetchResults(const PageInfo& pg) const {
	SQLR * Select(SqlAll(RESULT), DT, CMT, UID, ToSqlVal(Regexp(PATH,data["SRC"])).As("FITS"))
	       .From(COMMITS).LeftJoin(RESULT).On(UID == CMT_UID && CLIENT_ID == data["ID"])
	       .OrderBy(Descending(DT))
	       .Limit(pg.offset, pg.limit);
	ValueArray res;
	ValueMap vm;
	Time t = GetSysTime();
	int maxage = int(data["MAX_AGE"])*24*60*60;
	while(SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		if (IsNull(vm["STATUS"])) {
			if (!vm["FITS"])
				vm.Add("STATUSSTR", "Not interested");
			else if (t-Time(vm["DT"]) > maxage)
				vm.Add("STATUSSTR", "Too old");
			else
				vm.Add("STATUSSTR", "Ready");
		} else {
			if (vm["STAT"] == WD_INPROGRESS)
				vm.Add("STATUSSTR", "In progress");
			else
				vm.Add("STATUSSTR", Format("%s %d%%", status(ComputeStatus(vm["OK"], vm["FAIL"], vm["ERR"])), vm["RATE"]));
		}
		if(vm["STATUS"] == WD_INPROGRESS)
			vm.Set("DURATION",GetSysTime()-Time(vm["START"]));
		res.Add(vm);
	}
	return res;
}

ValueMap Client::LoadAll() {
	SQLR * Select(SqlAll()).From(CLIENT).Where(ID>0);
	ValueMap res;
	ValueMap vm;
	while (SQLR.Fetch(vm))
		res.Add(vm["ID"], vm);
	return res;
}

void Client::Delete(int id) {
	SQL * ::Delete(CLIENT).Where(id==ID);
}

void Client::UpdateActivity(int id, bool work) {
	Time t = GetSysTime();
	SqlUpdate update(CLIENT);
	if (work)
		update(LAST_WORK, t);
	SQL * update(LAST_ACTIVITY, t).Where(ID==id);
}


void Client::Save() {
	if(IsNull(data["ID"])) {
		int n = data.GetKeys().Find("ID");
		if(n>=0)
			data.Remove(n);
		SQL * Insert(CLIENT)(data);
	} else {
		SQL * Update(CLIENT)(data).Where(ID == data["ID"]);
	}
}

bool Commit::Load(const String& uid) {
	SQLR * Select(SqlAll()).From(COMMITS).Where(UID == uid);
	return SQLR.Fetch(data);
}

ValueArray Commit::FetchResults() const {
	SQLR * Select(SqlAll())
	       .From(RESULT).InnerJoin(CLIENT).On(ID == CLIENT_ID)
	       .Where(CMT_UID == data["UID"]);
	ValueArray res;
	ValueMap vm;
	while(SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		int st = vm["STATUS"];
		if(st > WD_INPROGRESS) {
			st = ComputeStatus(vm["OK"], vm["FAIL"], vm["ERR"]);
			vm.Set("STATUS", st);
		}
		vm.Add("STATUSSTR", status(vm["STATUS"]));
		if(vm["STATUS"]==1)
			vm.Set("DURATION",GetSysTime()-Time(vm["START"]));
		res.Add(vm);
	}
	return res;
}

ValueArray Commit::LoadPage(const PageInfo& pg) {
	SQLR * Select(SqlAll(),
	             CountIf(1, STATUS==WD_INPROGRESS).As("RUNNING"),
	             SqlFunc("sum",OK).As("OK"),
	             SqlFunc("sum",FAIL).As("FAIL"),
	             SqlFunc("sum",ERR).As("ERR"),
	             SqlFunc("sum",SKIP).As("SKIP"))
	      .From(COMMITS)
	      .LeftJoin(RESULT).On(UID==CMT_UID)
	      .GroupBy(UID)
	      .OrderBy(Descending(DT))
	      .Limit(pg.offset, pg.limit);
	ValueArray res;
	ValueMap vm;
	while (SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		res.Add(vm);
	}
	return res;
}

void Commit::Save() {
	Upsert(SQL, Insert(COMMITS)(data),
	            Update(COMMITS)(data).Where(UID == data["UID"]));
}

bool Result::Load(const String& uid, int id) {
	SQLR * Select(SqlAll(), (OK+FAIL+ERR+SKIP).As("TOTAL"))
	      .From(RESULT)
	      .InnerJoin(COMMITS).On(UID == CMT_UID)
	      .InnerJoin(CLIENT).On(ID == CLIENT_ID)
	      .Where(CLIENT_ID == id && CMT_UID == uid);
	if(!SQLR.Fetch(data))
		return false;
	SetComputedAttributes(data);
	int st = data["STATUS"];
	if(st > WD_INPROGRESS) {
		st = ComputeStatus(data["OK"], data["FAIL"], data["ERR"]);
		data.Set("STATUS", st);
	}
	if(st == WD_INPROGRESS)
		data.Set("DURATION", GetSysTime()-Time(data["START"]));
	data.Set("STATUSN", st);
	data.Set("STATUS", status(st));
	return true;
}

ValueMap Result::LoadPage(const PageInfo& pg) {
	SQLR * Select(SqlAll(RESULT),UID,CMT,CLIENT_ID)
	       .From(
	           Select(UID.As("FILTER"))
	           .From(COMMITS)
	           .OrderBy(Descending(DT))
	           .Limit(pg.offset, pg.limit)
	           .AsTable("FILTER_TABLE")
	       )
	       .InnerJoin(COMMITS).On(SqlId("FILTER")==UID)
	       .LeftJoin(RESULT).On(CMT_UID == UID)
	       .LeftJoin(CLIENT).On(ID == CLIENT_ID)
	       .OrderBy(Descending(DT));
	VectorMap<Tuple2<String, int>,ValueMap> rows;
	SortedIndex<int> clients;
	VectorMap<String, String> commits;
	ValueArray v_clients;
	ValueMap v_commits;
	ValueMap vm;
	while (SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		String uid = vm["UID"];
		int cid = vm["CLIENT_ID"];
		rows.Add(MakeTuple(uid, cid), vm);
		clients.FindAdd(cid);
		commits.GetAdd(uid) = vm["CMT"];
	}
	ValueMap results;
	for(int i = 0; i<commits.GetCount() ; ++i){
		v_commits.Add(commits.GetKey(i), commits[i]);
		vm.Clear();
		for(int j = 0; j < clients.GetCount(); ++j){
			if(!IsNull(clients[j]))
				vm.Add(clients[j], rows.GetAdd(MakeTuple(commits[i], clients[j])));
		}
		results.Add(commits[i], vm);
	}
	for(int i = (!clients.IsEmpty() && IsNull(clients[0]))?1:0; i < clients.GetCount(); i++)
		v_clients.Add(clients[i]);
	ValueMap res;
	res.Set("RESULTS", results);
	res.Set("COMMITS", v_commits);
	res.Set("CLIENTS", v_clients);
	res.Set("ALLCLIENTS", Client::LoadAll());
	return res;
}

String Result::FetchOutput() const {
	return LoadFile(Format("%s/%d/%d.log",(String)Ini::output_dir,data["CMT_UID"],data["CLIENT_ID"]));
}

void Result::Delete(const String& uid, int id) {
	if(IsEmpty(uid))
		SQL * ::Delete(RESULT).Where(CLIENT_ID==id);
	else
		SQL * ::Delete(RESULT).Where(CLIENT_ID==id && CMT_UID==uid);
}

void Result::Save() {
	//TODO when changing API
}

