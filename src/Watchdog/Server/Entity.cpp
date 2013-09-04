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
	SQLR * Select(SqlAll(RESULT), DT, REVISION, ToSqlVal(Regexp(PATH,data["SRC"])).As("FITS"))
	       .From(WORK).LeftJoin(RESULT).On(REVISION == REV && CLIENT_ID == data["ID"])
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

bool Commit::Load(int rev) {
	SQLR * Select(SqlAll()).From(WORK).Where(REVISION == rev);
	return SQLR.Fetch(data);
}

ValueArray Commit::FetchResults() const {
	SQLR * Select(SqlAll())
	       .From(RESULT).InnerJoin(CLIENT).On(ID == CLIENT_ID)
	       .Where(REV == data["REVISION"]);
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
	      .From(WORK)
	      .LeftJoin(RESULT).On(REVISION==REV)
	      .GroupBy(REVISION)
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
	Upsert(SQL, Insert(WORK)(data),
	            Update(WORK)(data).Where(REVISION == data["REVISION"]));
}

bool Result::Load(int rev, int id) {
	SQLR * Select(SqlAll(), (OK+FAIL+ERR+SKIP).As("TOTAL"))
	      .From(RESULT)
	      .InnerJoin(WORK).On(REVISION == REV)
	      .InnerJoin(CLIENT).On(ID == CLIENT_ID)
	      .Where(CLIENT_ID == id && REV == rev);
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
	SQLR * Select(SqlAll(RESULT),REVISION,CLIENT_ID)
	       .From(
	           Select(REVISION.As("FILTER"))
	           .From(WORK)
	           .OrderBy(Descending(DT))
	           .Limit(pg.offset, pg.limit)
	       )
	       .InnerJoin(WORK).On(SqlId("FILTER")==REVISION)
	       .LeftJoin(RESULT).On(REV == REVISION)
	       .LeftJoin(CLIENT).On(ID == CLIENT_ID)
	       .OrderBy(CLIENT_ID);
	VectorMap<Tuple2<int,int>,ValueMap> rows;
	SortedIndex<int> clients;
	SortedIndex<int> commits;
	ValueArray v_clients;
	ValueArray v_commits;
	ValueMap vm;
	while (SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		int rev = vm["REVISION"];
		int cid = vm["CLIENT_ID"];
		rows.Add(MakeTuple(rev,cid), vm);
		clients.FindAdd(cid);
		commits.FindAdd(rev);
	}
	ValueMap results;
	for(int i = commits.GetCount()-1; i>=0 ; --i){
		v_commits.Add(commits[i]);
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
	return LoadFile(Format("%s/%d/%d.log",(String)Ini::output_dir,data["REV"],data["CLIENT_ID"]));
}

void Result::Delete(int rev, int id) {
	if(IsNull(rev))
		SQL * ::Delete(RESULT).Where(CLIENT_ID==id);
	else
		SQL * ::Delete(RESULT).Where(CLIENT_ID==id && REV==rev);
}

void Result::Save() {
	//TODO when changing API
}

