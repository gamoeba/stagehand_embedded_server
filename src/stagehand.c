/**
 * This file contains the exported symbol.
 */
#include "stagehand.h"
#include <dlog/dlog.h>

bool
startstagehand1(const char* stagehand_path, int port)
{
	//startListeningSocketThread();
	dlog_print(DLOG_INFO, "Stagehand", "startstagehand");
	startServer(stagehand_path, port);
	return true;
}
