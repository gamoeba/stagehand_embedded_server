/**
 * This file contains the exported symbol.
 */
#include "stagehand.h"
#include <dlog/dlog.h>

// This is an example of an exported method.
bool
startstagehand1(void)
{
	//startListeningSocketThread();
	dlog_print(DLOG_INFO, "Stagehand", "startstagehand");
	startServer();
	return true;
}
