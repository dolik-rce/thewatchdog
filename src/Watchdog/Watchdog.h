#ifndef _Watchdog_Client_h_
#define _Watchdog_Client_h_

#include <Core/Core.h>
using namespace Upp;


enum {
	WD_READY,
	WD_INPROGRESS,
	WD_FAILED,
	WD_ERROR,
	WD_DONE
};

const char* status(int n);

#endif
