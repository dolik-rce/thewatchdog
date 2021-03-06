#ifndef _Watchdog_Admin_Admin_h_
#define _Watchdog_Admin_Admin_h_

#include <Watchdog/Client/Client.h>

struct WatchdogAdmin : public WatchdogClient {
	String branch;
	String branches;

	virtual bool GetState();
	virtual bool Update();
	virtual bool Clean();
	virtual bool DailyReport();
	virtual bool Delete(const String& branch);

	void ParseArgument(int& i, const Vector<String>& cmd);
	virtual bool ProcessAction();

	WatchdogAdmin();
	~WatchdogAdmin() {};
};

#endif
