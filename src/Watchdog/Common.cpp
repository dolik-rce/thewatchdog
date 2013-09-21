#include "Watchdog.h"

void ProcessIniFile(const String& fn){
	RLOG("Loading configuration from file '" << fn << "'");
	String ini = LoadFile(fn);
	ini = ReplaceVars(ini, Environment());
	String tmp = GetTempFileName(GetFileName(fn));
	SaveFile(tmp, ini);
	SetIniFile(tmp);
	GetIniKey("");
	DeleteFile(tmp);
}

void LoadConfiguration(const String& fn) {
	String base;
	if(!fn.IsEmpty()) {
		if (FileExists(fn)) {
			ProcessIniFile(fn);
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
		ProcessIniFile(ini);
}

String ReplaceVars(const String& src, const VectorMap<String, String>& vars) {
	String r;
	const char *b = src;
	const char *s = b;
	for(;;) {
		s = strchr(s, '$');
		if(s) {
			r.Cat(b, s);
			if(s[1] == '$') {
				r.Cat('$');
				b = s = s + 2;
			}
			else {
				b = s;
				CParser p(++s);
				if(p.IsId()) {
					String id = p.ReadId();
					int q = vars.Find(id);
					if(q >= 0) {
						r.Cat(vars[q]);
						b = s = p.GetSpacePtr();
					}
				}
			}
		}
		else {
			r.Cat(b, src.End());
			return r;
		}
	}
}
