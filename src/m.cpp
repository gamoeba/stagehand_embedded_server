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
#include <sys/types.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <string.h>
#include "stagehand.h"
#include <app_common.h>

#include <unzip.h>

extern "C" int server_main(const char*path, int port);

using namespace Dali;

int current_connection_fd = 0;
int wait_fd = 0;

pthread_cond_t cv;
pthread_mutex_t mp;

void event_thread_callback() {
	std::string json = Stagehand::Automation::DumpScene();
	const char* ptr = json.c_str();
	int len = json.length();
	int written = 0;
	while (written < len) {
		written += write(current_connection_fd, ptr+written, len-written);
	}

	close(current_connection_fd);

}


void* connection_handler(void* info)
{
	int nbytes;
	char buffer[256];
	current_connection_fd = *(int*)info;


	nbytes = read(current_connection_fd, buffer, 256);
	buffer[nbytes] = 0;

	TriggerEventFactory factory;
    // create a trigger event that automatically deletes itself after the callback has run in the main thread
    TriggerEventInterface *interface = factory.CreateTriggerEvent( MakeCallback(event_thread_callback), TriggerEventInterface::DELETE_AFTER_TRIGGER );

    // asynchronous call, the call back will be run sometime later on the main thread
    interface->Trigger();

	return NULL;
}

void* listeningSocket(void*)
{
	struct sockaddr_un address;
	int socket_fd, connection_fd;
	socklen_t address_length;

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0)
	{
		printf("socket() failed\n");
		return NULL;
	}

 /* start with a clean address structure */
	memset(&address, 0, sizeof(address));

	address.sun_family = AF_UNIX;
	address.sun_path[0]='\0';
	strcpy(address.sun_path+1, "stagehand_socket");

	if(bind(socket_fd,
		(struct sockaddr *) &address,
		sizeof(struct sockaddr_un)) != 0)
	{
		printf("bind() failed\n");
		return NULL;
	}

	if(listen(socket_fd, 5) != 0)
	{
		printf("listen() failed\n");
		return NULL;
	}

	while((connection_fd = accept(socket_fd,
		(struct sockaddr *) &address,
		&address_length)) > -1)
	{
		pthread_t* clientThread = new pthread_t();
		void* info = &connection_fd;
		int error = pthread_create( clientThread, NULL, connection_handler, info );
		if (error != 0) {
			perror("Error trying to create listening socket");
		}
	}

	close(socket_fd);
	return NULL;
}

void startListeningSocketThread() {
	pthread_t* serverThread = new pthread_t();
	int error = pthread_create( serverThread, NULL, listeningSocket, NULL );
	if (error!=0) {
		perror("Could not start thread for listening to socket");
	}
}


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
