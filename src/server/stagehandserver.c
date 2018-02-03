/*
 * libwebsockets-test-server - libwebsockets test implementation
 *
 * Copyright (C) 2010-2017 Andy Green <andy@warmcat.com>
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
 * may be proprietary.	So unlike the library itself, they are licensed
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
volatile int force_exit = 0, dynamic_vhost_enable = 0;
struct lws_vhost *dynamic_vhost;
struct lws_context *context;
struct lws_plat_file_ops fops_plat;

extern struct lws_plat_file_ops fops_mem;

/* http server gets files from this path */
#define LOCAL_RESOURCE_PATH INSTALL_DATADIR"/libwebsockets-test-server"
char *resource_path = "/";//LOCAL_RESOURCE_PATH;
#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
char crl_path[1024] = "";
#endif

int
callback_stagehand(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len);
/*
 * This demonstrates how to use the clean protocol service separation of
 * plugins, but with static inclusion instead of runtime dynamic loading
 * (which requires libuv).
 *
 * dumb-increment doesn't use the plugin, both to demonstrate how to
 * do the protocols directly, and because it wants libuv for a timer.
 *
 * Please consider using test-server-v2.0.c instead of this: it has the
 * same functionality but
 *
 * 1) uses lws built-in http handling so you don't need to deal with it in
 * your callback
 *
 * 2) Links with libuv and uses the plugins at runtime
 *
 * 3) Uses advanced lws features like mounts to bind parts of the filesystem
 * to the served URL space
 *
 * Another option is lwsws, this operates like test-server-v2,0.c but is
 * configured using JSON, do you do not need to provide any code for the
 * serving action at all, just implement your protocols in plugins.
 */

#define LWS_PLUGIN_STATIC
//#include "../plugins/protocol_lws_mirror.c"
//#include "../plugins/protocol_lws_status.c"
//#include "../plugins/protocol_lws_meta.c"

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
 *
 *  lws-status:			informs connected browsers of who else is
 *				connected.
 */

enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,
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


/* this shows how to override the lws file operations.	You don't need
 * to do any of this unless you have a reason (eg, want to serve
 * compressed files without decompressing the whole archive)
 */
static lws_fop_fd_t
test_server_fops_open(const struct lws_plat_file_ops *fops,
		     const char *vfs_path, const char *vpath,
		     lws_fop_flags_t *flags)
{
	lws_fop_fd_t fop_fd;

	// open zip from file in memory 
	fop_fd = fops_zip.open(&fops_mem, vfs_path, vfs_path+1, flags);

	if (fop_fd)
		lwsl_info("%s: opening %s, ret %p, len %lu\n", __func__,
				vfs_path, fop_fd,
				(long)lws_vfs_get_length(fop_fd));
	else
		lwsl_info("%s: open %s failed\n", __func__, vfs_path);

	return fop_fd;
}
void sighandler(int sig)
{
#if !defined(WIN32) && !defined(_WIN32)
	/* because windows is too dumb to have SIGUSR1... */
	if (sig == SIGUSR1) {
		/*
		 * For testing, you can fire a SIGUSR1 at the test server
		 * to toggle the existence of an identical server on
		 * port + 1
		 */
		dynamic_vhost_enable ^= 1;
		lws_cancel_service(context);
		lwsl_notice("SIGUSR1: dynamic_vhost_enable: %d\n",
				dynamic_vhost_enable);
		return;
	}
#endif
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


int _server_main()
{
	struct lws_context_creation_info info;
	struct lws_vhost *vhost;
	const char *iface = NULL;
	char cert_path[1024] = "";
	char key_path[1024] = "";
	char ca_path[1024] = "";
	int uid = -1, gid = -1;
	int use_ssl = 0;
	int pp_secs = 0;
	int opts = 0;
	int n = 0;

	int syslog_options = LOG_PID | LOG_PERROR;
	/*
	 * take care to zero down the info struct, he contains random garbaage
	 * from the stack otherwise
	 */
	memset(&info, 0, sizeof info);
	info.port = 27000;


	signal(SIGINT, sighandler);
	signal(SIGUSR1, sighandler);

	/* we will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", syslog_options, LOG_DAEMON);

	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);

	lwsl_notice("libwebsockets test server - license LGPL2.1+SLE\n");
	lwsl_notice("(C) Copyright 2010-2017 Andy Green <andy@warmcat.com>\n");

	printf("Using resource path \"%s\"\n", resource_path);

	info.iface = iface;
	info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.ws_ping_pong_interval = pp_secs;

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
	info.max_http_header_pool = 256;
	info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8 | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
	info.extensions = exts;
	info.timeout_secs = 5;
	info.ip_limit_ah = 24; /* for testing */
	info.ip_limit_wsi = 105; /* for testing */


	context = lws_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

	vhost = lws_create_vhost(context, &info);
	if (!vhost) {
		lwsl_err("vhost creation failed\n");
		return -1;
	}

	/*
	 * For testing dynamic vhost create / destroy later, we use port + 1
	 * Normally if you were creating more vhosts, you would set info.name
	 * for each to be the hostname external clients use to reach it
	 */

	info.port++;

	/* this shows how to override the lws file operations.	You don't need
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

	n = 0;

	while (n >= 0 && !force_exit) {
		struct timeval tv;

		gettimeofday(&tv, NULL);

		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

		n = lws_service(context, 50);

	}


	lws_context_destroy(context);

	lwsl_notice("libwebsockets-test-server exited cleanly\n");

#ifndef _WIN32
	closelog();
#endif

	return 0;
}


int server_main() {
	return _server_main();
}
