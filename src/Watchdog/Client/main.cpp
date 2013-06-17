#ifdef flagMAIN

#include "Client.h"

CONSOLE_APP_MAIN{
	WatchdogClient().Execute(CommandLine());
}

#endif
