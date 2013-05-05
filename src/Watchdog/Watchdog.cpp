#include "Watchdog.h"

const char* status(int n) {
	static const char* status[] = {
		"",
		"In progress",
		"Failed",
		"Error",
		"OK"
	};
	return status[n];
}
