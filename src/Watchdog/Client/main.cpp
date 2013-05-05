#ifdef flagMAIN

#include "Client.h"

void Usage(int exitcode){
	(exitcode==0?Cout():Cerr()) << 
	"Usage:\n"
	"\t" << GetExeTitle() << " [options] action\n\n"
	"Actions:\n"
	"\t-h --help\n"
	"\t\tPrints usage information (this text)\n"
	"\t-g --get\n"
	"\t\tReturns a list of not yet built revisions\n"
	"\t-a --accept <revision>\n"
	"\t\tNotifies server about building <revision>.\n"
	"\t-s --submit <revision> <result> <duration> <output_file>\n"
	"\t\tSends results to the server. Optional start and end times can\n"
	"\t\tbe supplied as int64.\n"
	"\t-c --command <command>\n"
	"\t\tGets and accepts work automatically, then performs <command>\n"
	"\t\twith '@revision' substituted by actual revision number\n"
	"\t\tand then submits the results to server.\n\n"
	"Options:\n"
	"\t-C --config <file>\n"
	"\t\tPath to configuration file\n"
	"\t-L --nolock\n"
	"\t\tDo not use locking (default is to lock before --accept\n"
	"\t\tand unlock after --submit)\n"
	"\t-S --start <time>\n"
	"\t\tStart date given in \"YYYY/MM/DD hh:mm:ss\" format, only \n"
	"\t\thonoured with --accept and --submit\n"
	"\t-E --end <time>\n"
	"\t\tEnd date given given in \"YYYY/MM/DD hh:mm:ss\" format, only\n"
	"\t\thonoured with --submit\n"
	"\t-A --age <days>\n"
	"\t\tMaximum age of commits, only honoured with --get and --command\n\n"
	"If not supplied, server will use the time of the accept request as start time\n"
	"and the time of submit request as end time.\n";
	Exit(exitcode);
}

void Command(WatchdogClient& client, String& command, int maxage){
	// ask for work
	if(!client.GetWork(maxage))
		Exit(2);
	
	// select what to do
	if(client.todo.GetCount()==0){
		if(Ini::log_level>0) 
			Cout() << "Nothing to do.\n";
		Exit(0);
	}
	int revision = StrInt(client.todo.Top());
	if(Ini::log_level > 0) 
		Cout() << "Revision " << revision << " selected for build\n";
	if(!client.AcceptWork(revision))
		Exit(3);
	
	// replace @revision
	command.Replace("@revision", IntStr(revision));
	
	// do work
	if(Ini::log_level > 0) 
		Cout() << "Executing command '" << command << "'\n";
	String output;
	Time start = GetSysTime();
	int result = Sys(command, output);
	int duration = GetSysTime()-start;
	
	if(Ini::log_level > 0) 
		Cout() << "Execution finished after " << duration << " with exit code '" << result << "'\n";
	
	if(result != 0)
		result = WD_FAILED;
	else
		result = WD_DONE;
	
	// send the results
	if(!client.SubmitWork(revision, result, duration, output))
		Exit(4);
}

inline void CheckParamCount(const Vector<String>& cmd, int current, int count){
	if(cmd.GetCount() <= current+count) {
		Cerr() << "ERROR: Wrong number of parameters for " << cmd[current] << "\n";
		Usage(1);
	}
}

inline void SetAction(char& action, const String& value){
	if(action == 0) {
		if (value.StartsWith("--")) {
			action=value[2];
		} else {
			action=value[1];
		}
	} else {
		Cerr() << "ERROR: Only one action can be specified\n\n";
		Usage(3);
	}
}

CONSOLE_APP_MAIN{
	StdLogSetup(LOG_FILE | LOG_COUT);
	SetDateFormat("%1:4d/%2:02d/%3:02d");
	SetDateScan("ymd");
	
	const Vector<String>& cmd = CommandLine();
	if(cmd.GetCount() < 1) {
		Usage(1);
	}
	
	Time start;
	Time end;
	int maxage = -1;
	int revision = -1;
	int duration = -1;
	int result = -1;
	bool lock = true;
	char action = 0;
	String cfg;
	String output;
	String command;
	
	for(int i = 0; i < cmd.GetCount(); i++){
		if(cmd[i] == "--help" || cmd[i] == "-h") {
			Usage(0);
		} else if(cmd[i] == "--get" || cmd[i] == "-g") {
			SetAction(action, cmd[i]);
		} else if(cmd[i] == "--accept" || cmd[i] == "-a") {
			SetAction(action, cmd[i]);
			CheckParamCount(cmd, i, 1);
			revision = StrInt(cmd[++i]);
		} else if(cmd[i] == "--submit" || cmd[i] == "-s") {
			SetAction(action, cmd[i]);
			CheckParamCount(cmd, i, 4);
			revision = StrInt(cmd[++i]);
			result = StrInt(cmd[++i]);
			duration = StrInt(cmd[++i]);
			output = cmd[++i];
		} else if(cmd[i] == "--command" || cmd[i] == "-c") {
			SetAction(action, cmd[i]);
			CheckParamCount(cmd, i, 1);
			command = cmd[++i];
		} else if(cmd[i] == "--config" || cmd[i] == "-C") {
			CheckParamCount(cmd, i, 1);
			cfg = cmd[++i];
		} else if(cmd[i] == "--nolock" || cmd[i] == "-L") {
			lock = false;
		} else if(cmd[i] == "--start" || cmd[i] == "-S") {
			CheckParamCount(cmd, i, 1);
			start = ScanTime(cmd[++i]);
		} else if(cmd[i] == "--end" || cmd[i] == "-E") {
			CheckParamCount(cmd, i, 1);
			end = ScanTime(cmd[++i]);
		} else if(cmd[i] == "--age" || cmd[i] == "-A") {
			CheckParamCount(cmd, i, 1);
			maxage = StrInt(cmd[++i]);
		} else {
			Cerr() << "ERROR: Unknown argument '" << cmd[i] << "'\n\n";
			Usage(2);
		}
	}
	
	if (cfg.IsEmpty())
		cfg = GetExeDirFile(GetExeTitle()+".ini");
	if(!FileExists(cfg)){
		Cerr() << "ERROR: Configuration file '" << cfg << "' not found\n";
		Exit(1);
	}
	SetIniFile(cfg);
	LOG_(Ini::log_level==2, GetIniInfoFormatted());
	
	WatchdogClient client(lock);

	if (action == 0) {
		Cerr() << "ERROR: No action given\n\n";
		Usage(5);
	} else if (action == 'g') {
		if(!client.GetWork(maxage))
			Exit(3);
		if(client.todo.IsEmpty()){
			if(Ini::log_level > 0) 
				Cout() << "Nothing to do.\n";
			Exit(0);
		}
		for(int i = 0; i < client.todo.GetCount(); i++){
			Cout() << client.todo[i] << "\n";
		}
	} else if(action == 'a') {
		if (revision <= 0){
			Cerr() << "ERROR: Wrong value of <revision>\n";
			Exit(4);
		}
		if(!client.AcceptWork(revision, start))
			Exit(3);
	} else if(action == 's') {
		if (revision <= 0){
			Cerr() << "ERROR: Wrong value of <revision>\n";
			Exit(1);
		}
		if (result < WD_READY || result > WD_DONE){
			Cerr() << "ERROR: Wrong value of <result>\n";
			Exit(1);
		}
		if (duration < 0){
			Cerr() << "ERROR: Wrong value of <duration>\n";
			Exit(1);
		}
		String out = LoadFile(output);
		if (out.IsVoid()){
			Cerr() << "ERROR: Can't load file '"<<output<<"'\n";
			Exit(4);
		}
		client.SubmitWork(revision, result, duration, out);
	} else if(action == 'c') {
		Command(client, command, maxage);
	}
}

#endif
