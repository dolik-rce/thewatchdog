#ifndef _Watchdog_Version_h_
#define _Watchdog_Version_h_

const char* WatchdogVersion(){
	//automagicaly refreshed by src/version.sh via git filter
	static const char* version = "";
	return version;
}

#endif
