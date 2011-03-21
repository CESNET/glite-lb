#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h> 
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>

#if defined(FREEBSD) || defined(__FreeBSD__)
#define TCP_CORK TCP_NOPUSH
#endif

#include "glite/lbu/log.h"
#include "glite/lb/context-int.h"
#include "glite/lb/timeouts.h"
#include "logd_proto.h"
#include "glite/security/glite_gss.h"
#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#endif

#define DEFAULT_PIDFILE "/var/glite/glite-lb-logd.pid"

typedef void (*logd_handler_t)(int);

static const char rcsid[] = "@(#)$Id$";
static int debug = 0;
static int port = EDG_WLL_LOG_PORT_DEFAULT;
static char *prefix = EDG_WLL_LOG_PREFIX_DEFAULT;
static char *pidfile = DEFAULT_PIDFILE;
static char *cert_file = NULL;
static char *key_file = NULL;
static char *CAcert_dir = NULL;
static int noAuth = 0;
static int noIPC = 0;
static int noParse = 0;

#define DEFAULT_SOCKET "/tmp/interlogger.sock"
char *socket_path = DEFAULT_SOCKET;

extern int confirm_sock;
extern char confirm_sock_name[256];

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },
	{ "debug", no_argument, 0, 'd' },
	{ "port", required_argument, 0, 'p' },
	{ "file-prefix", required_argument, 0, 'f' },
	{ "cert", required_argument, 0, 'c' },
	{ "key", required_argument, 0, 'k' },
	{ "CAdir", required_argument, 0, 'C' },
	{ "pidfile",required_argument, 0, 'i' },
	{ "socket",required_argument, 0, 's' },
	{ "noAuth", no_argument, 0, 'x' },
	{ "noIPC", no_argument, 0, 'y' },
	{ "noParse", no_argument, 0, 'z' },
	{ NULL, 0, NULL, 0}
};

/*
 *----------------------------------------------------------------------
 *
 * usage - print usage
 *
 *----------------------------------------------------------------------
 */

static void
usage(char *program_name) {
	fprintf(stdout,"%s\n"
		"- collect events from logging API calls,\n"
		"- save them to files and\n"
		"- send them to inter-logger\n\n"
		"Usage: %s [option]\n"
		"-h, --help                 display this help and exit\n"
		"-V, --version              output version information and exit\n"
		"-d, --debug                do not run as daemon\n"
		"-p, --port <num>           port to listen\n"
		"-f, --file-prefix <prefix> path and prefix for event files\n"
		"-c, --cert <file>          location of server certificate\n"
		"-k, --key  <file>          location of server private key\n"
		"-C, --CAdir <dir>          directory containing CA certificates\n"
		"-s, --socket <dir>         interlogger's socket to send messages to\n"
		"-i, --pidfile <file>       pid file\n"
		"--noAuth                   do not check caller's identity\n"
		"--noIPC                    do not send messages to inter-logger\n"
		"--noParse                  do not parse messages for correctness\n",
		program_name,program_name);
}

static logd_handler_t mysignal(int num,logd_handler_t handler)
{
	struct sigaction	sa,osa;

	memset(&sa,0,sizeof(sa));
	sa.sa_handler = handler;
	return sigaction(num,&sa,&osa) ? SIG_ERR : osa.sa_handler;
}

/*
 *----------------------------------------------------------------------
 *
 * handle_signal -
 *      HUP  - reread log4crc
 *	USR1 - print priorities of all standard categories
 *	USR2 - print priorities of all LB categories
 *
 *----------------------------------------------------------------------
 */

static int received_signal = 0;

static void handle_signal(int num)
{
	received_signal = num;
}

void do_handle_signal() {

	if (received_signal == 0) return;
	
	if (received_signal != SIGCHLD) glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Received signal %d\n", received_signal);
	switch (received_signal) {
	case SIGHUP:
		/* TODO: reload all external configurations, see
		https://rt3.cesnet.cz/rt/Ticket/Display.html?id=24879 */
		glite_common_log_reread();
		break;
	case SIGUSR1:
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,
			"Logging priority is now %s for %s, %s for %s and %s for %s\n", 
			glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_SECURITY)),
			LOG_CATEGORY_SECURITY,
			glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_ACCESS)),
			LOG_CATEGORY_ACCESS,
			glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_CONTROL)),
			LOG_CATEGORY_CONTROL);
		break;
	case SIGUSR2:
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,
			"Logging priority is now %s for %s and %s for %s\n", 
			glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_LB)),
			LOG_CATEGORY_LB,
			glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_LB_LOGD)),
			LOG_CATEGORY_LB_LOGD);
		break;
	case SIGPIPE:
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Broken pipe, lost communication channel.\n");
		break;
	case SIGCHLD:
		while (wait3(NULL,WNOHANG,NULL) > 0);
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		if (confirm_sock) {
			glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Closing confirmation socket.\n");
			close(confirm_sock);
			unlink(confirm_sock_name);
		}
		unlink(pidfile);
		exit(1);
		break;
	default: break;
	}

	received_signal = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * doit - do all the dirty work
 *
 *----------------------------------------------------------------------
 */
static int
doit(int socket, edg_wll_GssCred cred_handle, char *file_name_prefix, int noipc, int noparse)
{
    char 	*subject;
    int 	ret,fd,count;
    struct timeval timeout;
    edg_wll_GssConnection	con;
    edg_wll_GssStatus	gss_stat;
    edg_wll_GssPrincipal client = NULL;
    fd_set fdset;
    struct sockaddr_storage	peer;
    socklen_t	alen = sizeof peer;
    char 	peerhost[64], peerserv[16];

    ret = count = 0;
    FD_ZERO(&fdset);

    /* accept */
    timeout.tv_sec = ACCEPT_TIMEOUT;
    timeout.tv_usec = 0;
    getpeername(socket,(struct sockaddr *) &peer,&alen);
    glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Accepting connection (remaining timeout %d.%06d sec)\n",
		(int)timeout.tv_sec, (int) timeout.tv_usec);
    
    ret = getnameinfo ((struct sockaddr *) &peer, alen, 
		peerhost, sizeof(peerhost), peerserv, sizeof(peerserv), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret) {
	glite_common_log(LOG_CATEGORY_ACCESS, LOG_PRIORITY_WARN, "getnameinfo: %s", gai_strerror (ret));
	strcpy(peerhost, "unknown"); strcpy(peerserv, "unknown"); 
    }

/* XXX: ugly workaround, we may detect false expired certificated
 * probably due to bug in Globus GSS/SSL. */
#define _EXPIRED_CERTIFICATE_MESSAGE "certificate has expired"

    if ((ret = edg_wll_gss_accept(cred_handle,socket,&timeout,&con, &gss_stat)) < 0) {
	glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_DEBUG,"timeout after gss_accept is %d.%06d sec\n",
		(int)timeout.tv_sec, (int) timeout.tv_usec);
	if ( ret == EDG_WLL_GSS_ERROR_TIMEOUT ) {
		glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: Client authentication failed - timeout reached, closing.\n",peerhost);
	} else if (ret == EDG_WLL_GSS_ERROR_GSS) {
		char *gss_err;

		edg_wll_gss_get_error(&gss_stat, "Client authentication failed", &gss_err);
		if (strstr(gss_err,_EXPIRED_CERTIFICATE_MESSAGE)) {
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: false expired certificate: %s\n",peerhost,gss_err);
			free(gss_err);
			return -1;
		}
		glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: GSS error: %s, closing.\n",peerhost,gss_err);
		free(gss_err);
	} else {
		glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: Client authentication failed, closing.\n",peerhost);
	}
	return 1;
    }

    /* authenticate */
    glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_DEBUG,"Processing authentication:\n");
    ret = edg_wll_gss_get_client_conn(&con, &client, &gss_stat);
    if (ret) {
        char *gss_err;
        edg_wll_gss_get_error(&gss_stat, "Cannot read client identification", &gss_err);
        glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN, "%s: %s\n", peerhost,gss_err);
        free(gss_err);
    }

    if (ret || client->flags & EDG_WLL_GSS_FLAG_ANON) {
	glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"  User not authenticated, setting as \"%s\". \n",EDG_WLL_LOG_USER_DEFAULT);
	subject=strdup(EDG_WLL_LOG_USER_DEFAULT);
    } else {
	glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_INFO,"  User successfully authenticated as: %s\n",client->name);
	subject=strdup(client->name);
    }
    if (client)
	edg_wll_gss_free_princ(client);

    /* get and process the data */
    timeout.tv_sec = CONNECTION_TIMEOUT;
    timeout.tv_usec = 0;
    
    while (timeout.tv_sec > 0) {
	count++;
	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Waiting for data delivery no. %d (remaining timeout %d.%06d sec)\n",
		count, (int)timeout.tv_sec, (int) timeout.tv_usec);
	FD_SET(con.sock,&fdset);
	fd = select(con.sock+1,&fdset,NULL,NULL,&timeout);
	switch (fd) {
	case 0: /* timeout */
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Connection timeout expired\n");
		timeout.tv_sec = 0; 
		break;
	case -1: /* error */
		switch(errno) {
		case EINTR:
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"XXX: Waking up (remaining timeout %d.%06d sec)\n",
				(int)timeout.tv_sec, (int) timeout.tv_usec);
			continue;
		default:
			glite_common_log_SYS_ERROR("select");
			timeout.tv_sec = 0;
			break;
		}
		break;
	default:
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Waking up (remaining timeout %d.%06d sec)\n",
			(int)timeout.tv_sec, (int) timeout.tv_usec);
		break;
	}
	if (FD_ISSET(con.sock,&fdset)) {
		ret = edg_wll_log_proto_server(&con,&timeout,subject,file_name_prefix,noipc,noparse);
		// TODO: put into edg_wll_log_proto_server?
		if (ret != 0) {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"timeout after edg_wll_log_proto_server is %d.%06d sec\n",
				(int)timeout.tv_sec, (int) timeout.tv_usec);
			if (ret != EDG_WLL_GSS_ERROR_EOF) 
				glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): Error\n");
			else if (count == 1)
				glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): Error. EOF occured.\n");
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
			break;
		} else {
			timeout.tv_sec = CONNECTION_TIMEOUT;
			timeout.tv_usec = 0;
		}
	}

    }

	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG, "Closing descriptor %d.",con.sock);
	edg_wll_gss_close(&con, NULL);
	if (subject) free(subject);
	return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * Main -
 *
 *----------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
   int ret;
   int childpid;
   int opt;
   FILE *pidf;
   sigset_t mask;

   int listener_fd;
   int client_fd;
   struct sockaddr_storage client_addr;
   socklen_t client_addr_len;

   time_t	cert_mtime = 0;
   edg_wll_GssStatus	gss_stat;
   edg_wll_GssCred	cred = NULL;


   setlinebuf(stdout);
   setlinebuf(stderr);

   /* welcome */
   fprintf(stdout,"\
This is LocalLogger, part of Workload Management System in EU DataGrid & EGEE.\n");

   /* get arguments */
   while ((opt = getopt_long(argc,argv,
	"h"  /* help */
	"V"  /* version */
	"d"  /* debug */
	"p:" /* port */
	"f:" /* file prefix */
	"c:" /* certificate */
	"k:" /* key */
	"C:" /* CA dir */
	"s:" /* socket */
	"i:" /* pidfile */
	"x"  /* noAuth */
	"y"  /* noIPC */
	"z",  /* noParse */
	long_options, (int *) 0)) != EOF) {

	switch (opt) {
		case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); exit(0);
		case 'd': debug = 1; break;
		case 'p': port = atoi(optarg); break;
		case 'f': prefix = optarg; break;
		case 'c': cert_file = optarg; break;
		case 'k': key_file = optarg; break;
		case 'C': CAcert_dir = optarg; break;
		case 's': socket_path = optarg; break;
		case 'i': pidfile = optarg; break;
		case 'x': noAuth = 1; break;
		case 'y': noIPC = 1; break;
		case 'z': noParse = 1; break;
		case 'h':
		default:
			usage(argv[0]); exit(0);
	}
   }
   if (glite_common_log_init()) {
	fprintf(stderr,"glite_common_log_init() failed, exiting.");
	exit(1);
   }
   glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Initializing...\n");

   /* check noParse */
   if (noParse) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Parse messages for correctness... [no]\n");
   } else {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Parse messages for correctness... [yes]\n");
   }

   /* check noIPC */
   if (noIPC) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Send messages also to inter-logger... [no]\n");
   } else {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Send messages also to inter-logger... [yes]\n");
   }

   /* check prefix correctness */
   if (strlen(prefix) > FILENAME_MAX - 34) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"Too long prefix (%s) for file names, would not be able to write to log files. Exiting.\n",prefix);
	exit(1);
   }
   /* TODO: check for write permisions */
   glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Messages will be stored with the filename prefix \"%s\".\n",prefix);

   if (CAcert_dir)
	setenv("X509_CERT_DIR", CAcert_dir, 1);

#ifdef LB_PERF
   glite_wll_perftest_init(NULL, NULL, NULL, NULL, 0);
#endif
 
   edg_wll_gss_watch_creds(cert_file,&cert_mtime);
   /* XXX DK: support noAuth */
   ret = edg_wll_gss_acquire_cred_gsi(cert_file, key_file, &cred, &gss_stat);
   if (ret) {
	/* XXX DK: call edg_wll_gss_get_error() */
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"Failed to get GSI credentials. Exiting.\n");
	exit(1);
   }

   if (cred->name!=NULL) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Server running with certificate: %s\n",cred->name);
   } else if (noAuth) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Server running without certificate\n");
   }

   /* initialize signal handling */
   if (mysignal(SIGUSR1, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGUSR2, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGPIPE, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGHUP,  handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGINT,  handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGQUIT, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGTERM, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGCHLD, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }

   sigemptyset(&mask);
   sigaddset(&mask, SIGUSR1);
   sigaddset(&mask, SIGUSR2);
   sigaddset(&mask, SIGPIPE);
   sigaddset(&mask, SIGHUP);
   sigaddset(&mask, SIGINT);
   sigaddset(&mask, SIGQUIT);
   sigaddset(&mask, SIGTERM);
   sigaddset(&mask, SIGCHLD);
   sigprocmask(SIG_UNBLOCK, &mask, NULL);

   /* do listen */
   glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Listening on port %d\n",port);
   listener_fd = do_listen(port);
   if (listener_fd == -1) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"Failed to listen on port %d\n",port);
	edg_wll_gss_release_cred(&cred, NULL);
	exit(-1);
   } else {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_DEBUG,"Listener's socket descriptor is '%d'\n",listener_fd);
   }

   client_addr_len = sizeof(client_addr);
   bzero((char *) &client_addr, client_addr_len);

/* just try it before deamonizing to be able to complain aloud */
  if (!(pidf = fopen(pidfile,"w"))) {
        perror(pidfile);
        exit(-1);
  }
  fclose(pidf);


   /* daemonize */
   if (debug) {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Running as daemon... [no]\n");
   } else {
	glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_INFO,"Running as daemon... [yes]\n");
	if (daemon(0,0) < 0) {
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"Failed to run as daemon. Exiting.\n");
		glite_common_log_SYS_ERROR("daemon");
		exit(1);
	}
   }

  pidf = fopen(pidfile,"w"); assert(pidf); /* XXX */
  fprintf(pidf,"%d\n",getpid());
  fclose(pidf);

  umask(S_IRWXG | S_IRWXO);

   /*
    * Main loop
    */
   while (1) {
        int opt,my_errno;

	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Accepting incomming connections...\n");
	client_fd = accept(listener_fd, (struct sockaddr *) &client_addr,
			&client_addr_len);
	my_errno = errno;
	do_handle_signal();
	if (client_fd < 0) {
		if (my_errno == EINTR) continue;
		close(listener_fd);
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_FATAL,"Failed to accept incomming connections\n");
		glite_common_log_SYS_ERROR("accept");
		edg_wll_gss_release_cred(&cred, NULL);
		exit(-1);
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Incomming connection on socket '%d'\n",client_fd);
	}

	opt = 0;
	if (setsockopt(client_fd,IPPROTO_TCP,TCP_CORK,(const void *) &opt,sizeof opt)) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Can't reset TCP_CORK\n");
	}
	opt = 1;
	if (setsockopt(client_fd,IPPROTO_TCP,TCP_NODELAY,(const void *) &opt,sizeof opt)) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Can't set TCP_NODELAY\n");
	}

	switch (edg_wll_gss_watch_creds(cert_file,&cert_mtime)) {
	edg_wll_GssCred newcred;
	case 0: break;
	case 1:
		ret = edg_wll_gss_acquire_cred_gsi(cert_file,key_file,&newcred,&gss_stat);
		if (ret) {
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"Reloading credentials failed, continue with older\n");
		} else {
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_DEBUG,"Reloading credentials succeeded\n");
			edg_wll_gss_release_cred(&cred, NULL);
			cred = newcred;
		}
		break;
	case -1:
		glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"edg_wll_gss_watch_creds failed\n");
		break;
	}

	/* FORK - change next line if fork() is not needed (for debugging for example) */
#if 1
	if ((childpid = fork()) < 0) {
		glite_common_log_SYS_ERROR("fork");
		if (client_fd) close(client_fd);
	}
	if (childpid == 0) {
		ret = doit(client_fd,cred,prefix,noIPC,noParse);
		if (client_fd) close(client_fd);
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_DEBUG,"Exiting.\n");
		exit(0);
	}
	if (childpid > 0) {
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_DEBUG,"Forked a new child with PID %d\n",childpid);
		if (client_fd) close(client_fd);
	}
#else
	ret = doit(client_fd,cred,prefix,noIPC,noParse);
	if (client_fd) close(client_fd);

#endif
    } /* while */

	if (listener_fd) close(listener_fd);
	edg_wll_gss_release_cred(&cred, NULL);
	exit(ret);
}
