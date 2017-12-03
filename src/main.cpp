/* * Copyright (c) 2016 Gamoeba Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// EXTERNAL INCLUDES
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
//#include <linux/un.h>
#include "stagehand.h"
#include <app_common.h>

#include <unzip.h>

extern "C" int server_main(const char*path, int port);


void startServer()
{
//	printf ("Starting Stagehand server\n");
	//dlog_print(DLOG_INFO, "Stagehand", "start server");
	const char* path = "/";
	//dlog_print(DLOG_INFO, "Stagehand", "shared path: %s", path);

	pid_t pid = fork();
//	char respath[256];

	if (pid==0) {
		//dlog_print(DLOG_INFO, "Stagehand", "start server forked");

		//int res = daemon(0,0);
		//if (res !=0 ) {
		//	perror("Faild to detach process from parent");
		//}
		//sprintf(respath,"--resource_path=%s", STAGEHAND_SERVER_DIR);
		//execlp("stagehandserver", "stagehandserver", respath, "--port=8080", NULL);


		int res = server_main(path, 7690);
		dlog_print(DLOG_INFO, "Stagehand", "server finished");

		exit(res);
		//printf("stagehandserver start failed\n");
		// if it fails, just exit, nothing more to do
	}
}

extern "C" {

    int main(int argc, char** argv)
    {
        startServer();        

        return 0;
    }
}
//// TODO Can the code below be written to not generate these warnings
//#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
//#pragma GCC diagnostic ignored "-Wuninitialized"
//
//void Application::MainLoop()
//{
//  startServer();
//  startListeningSocketThread();
//  void(Application::*mainloop_ptr)();
//
//  *(void**)(&mainloop_ptr) = dlsym(RTLD_NEXT, "_ZN4Dali11Application8MainLoopEv");
//  Application& instance = *this;
//  (instance.*mainloop_ptr)();
//}
//
//void Application::MainLoop(Configuration::ContextLoss configuration)
//{
//  startServer();
//  startListeningSocketThread();
//  void(Application::*mainloop_ptr)(Configuration::ContextLoss);
//
//  *(void**)(&mainloop_ptr) = dlsym(RTLD_NEXT, "_ZN4Dali11Application8MainLoopENS_13Configuration11ContextLossE");
//  Application& instance = *this;
//  (instance.*mainloop_ptr)(configuration);
//}
