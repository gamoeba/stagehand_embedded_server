/*
 * libwebsockets-test-server - libwebsockets test implementation
 *
 * Copyright (C) 2010-2016 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated
 * the work to the public domain by waiving all of his or her rights
 * to the work worldwide under copyright law, including all related
 * and neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
 *
 * The test apps are intended to be adapted for use in your code, which
 * may be proprietary.  So unlike the library itself, they are licensed
 * Public Domain.
 */
#include "stagehandserver.h"

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <linux/un.h>
#include <dlog.h>

#define BUF_SIZE 4096
#define RESP_SIZE 1024*1024


char* issueCmd(const char* cmd)
{
 struct sockaddr_un address;
 int  socket_fd, nbytes;
 char buffer[BUF_SIZE];

 char* responseBuf = malloc(RESP_SIZE);
 socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
 if(socket_fd < 0)
 {
  dlog_print(DLOG_INFO, "Stagehand", "socket() failed\n");
  return "";
 }

 /* start with a clean address structure */
 memset(&address, 0, sizeof(address));
 
 address.sun_family = AF_UNIX;
 address.sun_path[0] = '\0';
 strcpy(address.sun_path+1, "stagehand_socket");

 if(connect(socket_fd, 
            (struct sockaddr *) &address, 
            sizeof(address)) != 0)
 {
	 dlog_print(DLOG_INFO, "Stagehand", "connect failed\n");

  printf("connect() failed\n");
  return "";
 }

 ssize_t written = write(socket_fd, cmd, strlen(cmd));

 if (written==-1) {
	  dlog_print(DLOG_INFO, "Stagehand", " write to socket() failed");

 	perror("write to socket failed");
 } 
 int eol = 0;
 int index = 0;
 while (!eol) {
 	nbytes = read(socket_fd, buffer, BUF_SIZE);
 	strncpy(responseBuf+index, buffer, nbytes);
 	index += nbytes;
 	if (nbytes>0 && buffer[nbytes-1]=='\n') {
 		eol = 1;
 	}
 	if (nbytes ==0)
 		eol = 1;
 }
 responseBuf[index] ='\0';

 close(socket_fd);

 return responseBuf;
}


int
callback_stagehand(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	//unsigned char buf[LWS_PRE + 512];
	struct per_session_data__dumb_increment *pss =
			(struct per_session_data__dumb_increment *)user;
	//unsigned char *p = &buf[LWS_PRE];
	//int n, m;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		pss->number = 0;
		break;

	// case LWS_CALLBACK_SERVER_WRITEABLE:
	// 	n = sprintf((char *)p, "%d", pss->number++);
	// 	m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
	// 	if (m < n) {
	// 		lwsl_err("ERROR %d writing to di socket\n", n);
	// 		return -1;
	// 	}
	// 	if (close_testing && pss->number == 50) {
	// 		lwsl_info("close tesing limit, closing\n");
	// 		return -1;
 // 		}
	// 	break;

	case LWS_CALLBACK_RECEIVE:
		dlog_print(DLOG_INFO, "Stagehand", "received stagehand message: %s", (const char*)in);
		printf("received message %s\n",(const char *) in);
		if (strcmp((const char *)in, "dump_scene\n") == 0){
			pss->number = 0;
			const char* resp = issueCmd(in);
			lws_write(wsi, (unsigned char*) resp,strlen(resp), LWS_WRITE_TEXT);
		}
		if (strcmp((const char *)in, "closeme\n") == 0) {
			lwsl_notice("dumb_inc: closing as requested\n");
			lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,
					 (unsigned char *)"seeya", 5);
			return -1;
		}
		break;
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */
	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	/*
	 * this just demonstrates how to handle
	 * LWS_CALLBACK_WS_PEER_INITIATED_CLOSE and extract the peer's close
	 * code and auxiliary data.  You can just not handle it if you don't
	 * have a use for this.
	 */
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
		{
		int n;
		lwsl_notice("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: len %d\n",
			    len);
		for (n = 0; n < (int)len; n++)
			lwsl_notice(" %d: 0x%02X\n", n,
				    ((unsigned char *)in)[n]);
		break;
		}
	default:
		break;
	}

	return 0;
}
