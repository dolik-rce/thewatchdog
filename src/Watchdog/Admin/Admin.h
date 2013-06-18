#ifndef _Watchdog_Admin_Admin_h_
#define _Watchdog_Admin_Admin_h_

#include <Watchdog/Client/Client.h>

struct WatchdogAdmin : public WatchdogClient {
	virtual bool Update();
	virtual bool Clean();

	void ParseArgument(int& i, const Vector<String>& cmd);
	virtual bool ProcessAction();

	WatchdogAdmin();
	~WatchdogAdmin() {};
};

#endif
