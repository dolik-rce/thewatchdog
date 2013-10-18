#ifndef _Watchdog_Watchdog_h_
#define _Watchdog_Watchdog_h_

#include <Core/Core.h>
using namespace Upp;

void LoadConfiguration(const String& fn);

#ifdef _DEBUG
#define IFDBG(D,R) D
#else
#define IFDBG(D,R) R
#endif

#endif
