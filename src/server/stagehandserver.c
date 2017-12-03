/*
 * libwebsockets-test-servet - libwebsockets test implementation
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

int close_testing;
int max_poll_elements;
int debug_level = 7;

#ifdef EXTERNAL_POLL
struct lws_pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
#endif
volatile int force_exit = 0;
struct lws_context *context;
struct lws_plat_file_ops fops_plat;

extern struct lws_plat_file_ops fops_mem;
extern struct lws_plat_file_ops fops_zip;


/* http server gets files from this path */
//#define LOCAL_RESOURCE_PATH INSTALL_DATADIR"/libwebsockets-test-server"
char *resource_path = "";//LOCAL_RESOURCE_PATH;

/* singlethreaded version --> no locks */

void test_server_lock(int care)
{
}
void test_server_unlock(int care)
{
}

/*
 * This demo server shows how to use libwebsockets for one or more
 * websocket protocols in the same server
 *
 * It defines the following websocket protocols:
 *
 *  dumb-increment-protocol:  once the socket is opened, an incrementing
 *				ascii string is sent down it every 50ms.
 *				If you send "reset\n" on the websocket, then
 *				the incrementing number is reset to 0.
 *
 *  lws-mirror-protocol: copies any received packet to every connection also
 *				using this protocol, including the sender
 */

enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_DUMB_INCREMENT,
	PROTOCOL_LWS_MIRROR,
	PROTOCOL_LWS_ECHOGEN,
	PROTOCOL_STAGEHAND,

	/* always last */
	DEMO_PROTOCOL_COUNT
};

/* list of supported protocols and callbacks */

static struct lws_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
		"stagehand-protocol",
		callback_stagehand,
		sizeof(struct per_session_data__dumb_increment),
		4096,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};


/* this shows how to override the lws file operations.  You don't need
 * to do any of this unless you have a reason (eg, want to serve
 * compressed files without decompressing the whole archive)
 */
static lws_filefd_type
test_server_fops_open(struct lws *wsi, const char *filename,
		      unsigned long *filelen, int flags)
{
	lws_filefd_type n;

	// open zip from file in memory
	n = fops_zip.open(&fops_mem, filename, filename+1, flags);

	lwsl_notice("%s: opening %s, ret %ld, len %lu\n", __func__, filename,
			(long)n, *filelen);

	return n;
}

void sighandler(int sig)
{
	force_exit = 1;
	lws_cancel_service(context);
}

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL /* terminator */ }
};


int server_main(const char *path, int port)
{
	struct lws_context_creation_info info;
	struct lws_vhost* vhost;
	char interface_name[128] = "";
	unsigned int ms, oldms = 0;
	const char *iface = NULL;
	char cert_path[1024] = "";
	char key_path[1024] = "";
	char ca_path[1024] = "";
	int uid = -1, gid = -1;
	int use_ssl = 0;
	int opts = 0;
	int n = 0;
#ifndef _WIN32
	int syslog_options = LOG_PID | LOG_PERROR;
#endif
#ifndef LWS_NO_DAEMONIZE
 	int daemonize = 0;
#endif

	/*
	 * take care to zero down the info struct, he contains random garbaage
	 * from the stack otherwise
	 */
	memset(&info, 0, sizeof info);
	info.port = port;
	strcpy(resource_path,path);

#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
	/*
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
	if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
		fprintf(stderr, "Failed to daemonize\n");
		return 10;
	}
#endif

	signal(SIGINT, sighandler);

#ifndef _WIN32
	/* we will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", syslog_options, LOG_DAEMON);
#endif

	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);

	lwsl_notice("libwebsockets test server - license LGPL2.1+SLE\n");
	lwsl_notice("(C) Copyright 2010-2016 Andy Green <andy@warmcat.com>\n");

	printf("Using resource path \"%s\"\n", resource_path);
#ifdef EXTERNAL_POLL
	max_poll_elements = getdtablesize();
	pollfds = malloc(max_poll_elements * sizeof (struct lws_pollfd));
	fd_lookup = malloc(max_poll_elements * sizeof (int));
	if (pollfds == NULL || fd_lookup == NULL) {
		lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
		return -1;
	}
#endif

	info.iface = iface;
	info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;

	if (use_ssl) {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		if (!cert_path[0])
			sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		if (!key_path[0])
			sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
		if (ca_path[0])
			info.ssl_ca_filepath = ca_path;
	}
	info.gid = gid;
	info.uid = uid;
	info.max_http_header_pool = 16;
	info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8;
	info.extensions = exts;
	info.timeout_secs = 5;
	info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-GCM-SHA384:"
			       "DHE-RSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-SHA384:"
			       "HIGH:!aNULL:!eNULL:!EXPORT:"
			       "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
			       "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
			       "!DHE-RSA-AES128-SHA256:"
			       "!AES128-GCM-SHA256:"
			       "!AES128-SHA256:"
			       "!DHE-RSA-AES256-SHA256:"
			       "!AES256-GCM-SHA384:"
			       "!AES256-SHA256";
	context = lws_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

	/* this shows how to override the lws file operations.  You don't need
	 * to do any of this unless you have a reason (eg, want to serve
	 * compressed files without decompressing the whole archive)
	 */
	/* stash original platform fops */
	fops_plat = *(lws_get_fops(context));
	/* override the active fops */
	lws_get_fops(context)->open = test_server_fops_open;
	lws_get_fops(context)->close = fops_zip.close;
	lws_get_fops(context)->read = fops_zip.read;
	lws_get_fops(context)->seek_cur = fops_zip.seek_cur;
	lws_get_fops(context)->write = NULL;


	vhost = lws_create_vhost(context, &info);
	if (!vhost) {
		lwsl_err("vhost creation failed\n");
		return -1;
	}


	n = 0;
	while (n >= 0 && !force_exit) {
		struct timeval tv;

		gettimeofday(&tv, NULL);

		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */

		ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
		if ((ms - oldms) > 50) {
			lws_callback_on_writable_all_protocol(context,
				&protocols[PROTOCOL_DUMB_INCREMENT]);
			oldms = ms;
		}

#ifdef EXTERNAL_POLL
		/*
		 * this represents an existing server's single poll action
		 * which also includes libwebsocket sockets
		 */

		n = poll(pollfds, count_pollfds, 50);
		if (n < 0)
			continue;

		if (n)
			for (n = 0; n < count_pollfds; n++)
				if (pollfds[n].revents)
					/*
					* returns immediately if the fd does not
					* match anything under libwebsockets
					* control
					*/
					if (lws_service_fd(context,
								  &pollfds[n]) < 0)
						goto done;
#else
		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

		n = lws_service(context, 50);
#endif
	}

#ifdef EXTERNAL_POLL
done:
#endif

	lws_context_destroy(context);

	lwsl_notice("libwebsockets-test-server exited cleanly\n");

#ifndef _WIN32
	closelog();
#endif

	return 0;
}
