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

ValueArray Client::FetchResults(const CommitFilter& f) const {
	SQLR * Select(SqlAll(RESULT),
	              DT, CMT, BRANCH,
	              UID, ToSqlVal(Regexp(PATH,data["SRC"])).As("FITS"))
	       .From(COMMITS)
	       .LeftJoin(RESULT).On(UID == CMT_UID && CLIENT_ID == data["ID"])
	       .Where(f)
	       .OrderBy(Descending(DT))
	       .Limit(f.offset, f.limit);
	ValueArray res;
	ValueMap vm;
	Time t = GetSysTime();
	int maxage = int(data["MAX_AGE"])*24*60*60;
	while(SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		SetDuration(vm, vm["STATUS"]);
		if (IsNull(vm["STATUS"])) {
			if (!vm["FITS"])
				vm.Set("STATUSSTR", "Not interested");
			else if (t-Time(vm["DT"]) > maxage)
				vm.Set("STATUSSTR", "Too old");
			else
				vm.Set("STATUSSTR", "Ready");
		}
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

void Client::Save(int force_id) {
	if(IsNull(data["ID"])) {
		if(force_id >= 0) {
			data.Set("ID", force_id);
		} else {
			int n = data.GetKeys().Find("ID");
			if(n>=0)
				data.Remove(n);
		}
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
		SetDuration(vm, vm["STATUS"]);
		res.Add(vm);
	}
	return res;
}

ValueArray Commit::LoadPage(const CommitFilter& f) {
	SQLR * Select(SqlAll(),
	             CountIf(1, STATUS==WD_INPROGRESS).As("RUNNING"),
	             SqlFunc("sum",OK).As("OK"),
	             SqlFunc("sum",FAIL).As("FAIL"),
	             SqlFunc("sum",ERR).As("ERR"),
	             SqlFunc("sum",SKIP).As("SKIP"))
	      .From(COMMITS)
	      .LeftJoin(RESULT).On(UID==CMT_UID)
	      .Where(f)
	      .GroupBy(UID)
	      .OrderBy(Descending(DT))
	      .Limit(f.offset, f.limit);
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
	SetDuration(data, data["STATUS"]);
	return true;
}

ValueMap Result::LoadPage(const CommitFilter& f) {
	SQLR * Select(SqlAll(RESULT),UID,CMT,BRANCH,CLIENT_ID)
	       .From(
	           Select(UID.As("FILTER"))
	           .From(COMMITS)
	           .Where(f)
	           .OrderBy(Descending(DT))
	           .Limit(f.offset, f.limit)
	           .AsTable("FILTER_TABLE")
	       )
	       .InnerJoin(COMMITS).On(SqlId("FILTER")==UID)
	       .LeftJoin(RESULT).On(CMT_UID == UID)
	       .LeftJoin(CLIENT).On(ID == CLIENT_ID)
	       .OrderBy(Descending(DT));
	VectorMap<Tuple2<String, int>,ValueMap> rows;
	SortedIndex<int> clients;
	VectorMap<String, ValueMap> commits;
	ValueArray v_clients;
	ValueMap v_commits;
	ValueMap vm;
	while (SQLR.Fetch(vm)){
		SetComputedAttributes(vm);
		String uid = vm["UID"];
		int cid = vm["CLIENT_ID"];
		rows.Add(MakeTuple(uid, cid), vm);
		clients.FindAdd(cid);
		ValueMap& commit = commits.GetAdd(uid);
		commit.Set("CMT", vm["CMT"]);
		commit.Set("BRANCH", vm["BRANCH"]);
	}
	ValueMap results;
	for(int i = 0; i<commits.GetCount() ; ++i){
		v_commits.Add(commits.GetKey(i), commits[i]);
		vm.Clear();
		for(int j = 0; j < clients.GetCount(); ++j){
			if(!IsNull(clients[j]))
				vm.Add(clients[j], rows.GetAdd(MakeTuple(commits.GetKey(i), clients[j])));
		}
		results.Add(commits.GetKey(i), vm);
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
	return LoadFile(Format("%s/%s/%d.log",(String)Ini::output_dir,data["CMT_UID"],data["CLIENT_ID"]));
}

void Result::Delete(const String& uid, int id) {
	if(IsEmpty(uid))
		SQL * ::Delete(RESULT).Where(CLIENT_ID==id);
	else
		SQL * ::Delete(RESULT).Where(CLIENT_ID==id && CMT_UID==uid);
}

void Result::Save() {
	Upsert(SQL, Insert(RESULT)(data),
	            Update(RESULT)(data).Where(CMT_UID == data["CMT_UID"] && CLIENT_ID == data["CLIENT_ID"]));
}

ValueMap Branch::LoadAll() {
	SQL * Select(BRANCH,
	             SqlFunc("COUNT",Distinct(UID)).As("CMT_CNT"),
	             SqlMin(DT).As("FIRST"),
	             SqlMax(DT).As("LAST"),
	             SqlSum(OK).As("OK"),
	             SqlSum(FAIL).As("FAIL"),
	             SqlSum(ERR).As("ERR"),
	             SqlSum(SKIP).As("SKIP"))
	      .From(COMMITS)
	      .LeftJoin(RESULT).On(UID == CMT_UID)
	      .GroupBy(BRANCH)
	      .OrderBy(BRANCH);
	ValueMap vm;
	ValueMap branches;
	while(SQL.Fetch(vm)) {
		SetComputedAttributes(vm, WD_DONE);
		branches.Set(vm["BRANCH"],vm);
	}
	return branches;
}

void Branch::Delete(const String& branch) {
	SQL * ::Delete(RESULT)
	        .Where(In(CMT_UID, Select(UID)
	                           .From(COMMITS)
	                           .Where(BRANCH == branch)));
	SQL * ::Delete(COMMITS).Where(BRANCH == branch);
}
