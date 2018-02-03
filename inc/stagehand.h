#ifndef _STAGEHAND_H_
#define _STAGEHAND_H_

/**
 * This header file is included to define _EXPORT_.
 */
#include <stdbool.h>
#include <tizen.h>


#ifdef __cplusplus
extern "C" {
#endif

void startListeningSocketThread();
void startServer(const char* stagehand_path, int port);
// This method is exported from stagehand.so
EXPORT_API bool startstagehand1(const char* stagehand_path, int port);

#ifdef __cplusplus
}
#endif
#endif // _STAGEHAND_H_

