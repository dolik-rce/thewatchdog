#include "Watchdog.h"

void LoadConfiguration(const String& fn) {
	String base;
	if(!fn.IsEmpty()) {
		if (FileExists(fn)) {
			SetIniFile(fn);
			return;
		}
		base = fn;
	} else
		base = GetExeTitle();
	
	String path = GetHomeDirectory() + ":" + GetExeDirFile("") + ":/etc/thewatchdog";
	String ini = GetFileOnPath(base + ".cfg", path);
	if (ini.IsEmpty())
		ini = GetFileOnPath(base + ".ini", path);
	if (ini.IsEmpty())
		ini = GetFileOnPath(base, path);
	if (ini.IsEmpty() || ini == GetExeFilePath()) {
		StdLogSetup(LOG_CERR);
		RLOG("Configuration file " << base << "{.cfg,.ini,} not found on path " << path);
		Exit();
	} else
		SetIniFile(ini);
}
