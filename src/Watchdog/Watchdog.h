#ifndef _Watchdog_Watchdog_h_
#define _Watchdog_Watchdog_h_

#include <Core/Core.h>
using namespace Upp;

String ReplaceVars(const String& src, const VectorMap<String, String>& vars);
void ProcessIniFile(const String& fn);
void LoadConfiguration(const String& fn);

#ifdef _DEBUG
#define IFDBG(D,R) D
#else
#define IFDBG(D,R) R
#endif

#endif
