#ident "$Header$"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h> 
#include <string.h>
#include <getopt.h>

#include <globus_common.h>

#include "logd_proto.h"
#include "glite/lb/consumer.h"
#include "glite/wms/tls/ssl_helpers/ssl_inits.h"

static const char rcsid[] = "@(#)$Id$";
static int verbose = 0;
static int debug = 0;
static int port = EDG_WLL_LOG_PORT_DEFAULT;
static char *prefix = EDG_WLL_LOG_PREFIX_DEFAULT;
static char *cert_file = NULL;
static char *key_file = NULL;
static char *CAcert_dir = NULL;
static char *gridmap_file = NULL;
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
	{ "verbose", no_argument, 0, 'v' },
	{ "debug", no_argument, 0, 'd' },
	{ "port", required_argument, 0, 'p' },
	{ "file-prefix", required_argument, 0, 'f' },
	{ "cert", required_argument, 0, 'c' },
	{ "key", required_argument, 0, 'k' },
	{ "CAdir", required_argument, 0, 'C' },
	{ "gridmap", required_argument, 0, 'g' },
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
		"-v, --verbose              print extensive debug output\n"
		"-p, --port <num>           port to listen\n"
		"-f, --file-prefix <prefix> path and prefix for event files\n"
		"-c, --cert <file>          location of server certificate\n"
		"-k, --key  <file>          location of server private key\n"
		"-C, --CAdir <dir>          directory containing CA certificates\n"
		"-s, --socket <dir>         socket to send messages (NOT IMPLEMENTED YET)\n"
		"--noAuth                   do not check caller's identity\n"
		"--noIPC                    do not send messages to inter-logger\n"
		"--noParse                  do not parse messages for correctness\n",
		program_name,program_name);
}

static sighandler_t mysignal(int num,sighandler_t handler)
{
	struct sigaction	sa,osa;

	memset(&sa,0,sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	return sigaction(num,&sa,&osa) ? SIG_ERR : osa.sa_handler;
}

/*
 *----------------------------------------------------------------------
 *
 * handle_signal -
 *	USR1 - increase the verbosity of the program
 *	USR2 - decrease the verbosity of the program
 *
 *----------------------------------------------------------------------
 */
void handle_signal(int num) {
	if (num != SIGCHLD) edg_wll_ll_log(LOG_NOTICE,"Received signal %d\n", num);
	switch (num) {
		case SIGUSR1:
			if (edg_wll_ll_log_level < LOG_DEBUG) edg_wll_ll_log_level++;
			edg_wll_ll_log(LOG_NOTICE,"Logging level is now %d\n", edg_wll_ll_log_level);
			break;
		case SIGUSR2:
			if (edg_wll_ll_log_level > LOG_EMERG) edg_wll_ll_log_level--;
			edg_wll_ll_log(LOG_NOTICE,"Logging level is now %d\n", edg_wll_ll_log_level);
			break;
		case SIGPIPE:
			edg_wll_ll_log(LOG_NOTICE,"Broken pipe, lost communication channel.\n");
			break;
		case SIGCHLD:
			while (wait3(NULL,WNOHANG,NULL) > 0);
			break;
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			if (confirm_sock) {
				edg_wll_ll_log(LOG_NOTICE,"Closing confirmation socket.\n");
				close(confirm_sock);
				unlink(confirm_sock_name);
			}
			exit(1);
			break;
		default: break;
	}
}

/*
 *----------------------------------------------------------------------
 *
 * doit - do all the dirty work
 *
 *----------------------------------------------------------------------
 */
static int
doit(int socket, void *cred_handle, char *file_name_prefix, int noipc, int noparse)
{
    SSL 	*ssl = NULL;
    X509 	*peer = NULL;

    char 	*subject;
    char 	buf[1024];
    int 	ret;
    struct timeval timeout = {10,0};             

    /* authentication */
    edg_wll_ll_log(LOG_INFO,"Processing authentication:\n");
// FIXME - put here some meaningfull value of timeout + do somthing if timeouted
    ssl = edg_wll_ssl_accept(cred_handle,socket,&timeout);
    if (ssl==NULL) {
	    edg_wll_ll_log(LOG_ERR,"edg_wll_ssl_accept() failed (%s)\n",
			    ERR_error_string(ERR_get_error(), NULL));
	    return(-1);
    }
    peer = SSL_get_peer_certificate(ssl);
    if (peer != NULL) {
       X509_NAME *s;

       edg_wll_ll_log(LOG_INFO,"  User successfully authenticated with the following certificate:\n");
       X509_NAME_oneline(X509_get_issuer_name(peer), buf, sizeof(buf));
       edg_wll_ll_log(LOG_INFO, "  Issuer: %s\n", buf);
       X509_NAME_oneline(X509_get_subject_name(peer), buf, sizeof(buf));
       edg_wll_ll_log(LOG_INFO, "  Subject: %s\n", buf);
       s = X509_NAME_dup(X509_get_subject_name(peer));
       proxy_get_base_name(s);
       subject=X509_NAME_oneline(s,NULL,0);
       X509_NAME_free(s);
    }
    else {
       edg_wll_ll_log(LOG_INFO,"  User not authenticated, setting as \"%s\". \n",EDG_WLL_LOG_USER_DEFAULT);
       subject=strdup(EDG_WLL_LOG_USER_DEFAULT);
    }

    ret = edg_wll_log_proto_server(ssl,subject,file_name_prefix,noipc,noparse);

    edg_wll_ssl_close(ssl);
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
   void *cred_handle = NULL;

   int ret;
   int childpid;
   int opt;

   int listener_fd;
   int client_fd;
   struct sockaddr_in client_addr;
   int client_addr_len;

   char *my_subject_name = NULL;

   time_t	cert_mtime = 0, key_mtime = 0;


   setlinebuf(stdout);
   setlinebuf(stderr);

   /* welcome */
   fprintf(stdout,"\
This is LocalLogger, part of Workload Management System in EU DataGrid.\
Copyright (c) 2002 CERN, INFN and CESNET on behalf of the EU DataGrid.\n");

   /* get arguments */
   while ((opt = getopt_long(argc,argv,
	"h"  /* help */
	"V"  /* version */
	"v"  /* verbose */
	"d"  /* debug */
	"p:" /* port */
	"f:" /* file prefix */
	"c:" /* certificate */
	"k:" /* key */
	"C:" /* CA dir */
	"g:" /* gridmap */
	"s:" /* socket */
	"x"  /* noAuth */
	"y"  /* noIPC */
	"z",  /* noParse */
	long_options, (int *) 0)) != EOF) {

	switch (opt) {
		case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); exit(0);
		case 'v': verbose = 1; break;
		case 'd': debug = 1; break;
		case 'p': port = atoi(optarg); break;
		case 'f': prefix = optarg; break;
		case 'c': cert_file = optarg; break;
		case 'k': key_file = optarg; break;
		case 'C': CAcert_dir = optarg; break;
		case 'g': gridmap_file = optarg; break;
		case 's': socket_path = optarg; break;
		case 'x': noAuth = 1; break;
		case 'y': noIPC = 1; break;
		case 'z': noParse = 1; break;
		case 'h':
		default:
			usage(argv[0]); exit(0);
	}
   }
   edg_wll_ll_log_init(verbose ? LOG_DEBUG : LOG_INFO);
   edg_wll_ll_log(LOG_INFO,"Initializing...\n");

   /* check noParse */
   edg_wll_ll_log(LOG_INFO,"Parse messages for correctness...");
   if (noParse) {
       edg_wll_ll_log(LOG_INFO,"no.\n");
   } else {
       edg_wll_ll_log(LOG_INFO,"yes.\n");
   }

   /* check noIPC */
   edg_wll_ll_log(LOG_INFO,"Send messages also to inter-logger...");
   if (noIPC) {
       edg_wll_ll_log(LOG_INFO,"no.\n");
   } else {
       edg_wll_ll_log(LOG_INFO,"yes.\n");
   }

   /* check prefix correctness */
/* XXX: check probably also write permisions */
   edg_wll_ll_log(LOG_INFO,"Store messages with the filename prefix \"%s\"...",prefix);
   if (strlen(prefix) > FILENAME_MAX - 34) {
	edg_wll_ll_log(LOG_INFO,"no.\n");
	edg_wll_ll_log(LOG_CRIT,"Too long prefix for file names, would not be able to write to log files. Exiting.\n");
	exit(1);
   } else {
	edg_wll_ll_log(LOG_INFO,"yes.\n");
   }

   /* parse X509 arguments to environment */
   edg_wll_set_environment(cert_file, key_file, NULL, NULL, CAcert_dir, gridmap_file);
   if (noAuth) setenv("X509_USER_PROXY","/dev/null",1);

   /* daemonize */
   edg_wll_ll_log(LOG_INFO,"Running as daemon...");
   if (debug) {
       edg_wll_ll_log(LOG_NOTICE,"no.\n");
   }
   else if (daemon(0,0) < 0) {
       edg_wll_ll_log(LOG_CRIT,"Failed to run as daemon. Exiting.\n");
       perror("daemon");
       exit(1);
   }
   else {
       edg_wll_ll_log(LOG_INFO,"yes.\n");
   }

   /* initialize Globus common module */
   edg_wll_ll_log(LOG_INFO,"Initializing Globus common module...");
   if (globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS) {
       edg_wll_ll_log(LOG_NOTICE,"no.\n");
       edg_wll_ll_log(LOG_CRIT, "Failed to initialize Globus common module. Exiting.\n");
       exit(1);
   } else {
       edg_wll_ll_log(LOG_INFO,"yes.\n");
   }

   /* initialize signal handling */
   if (mysignal(SIGUSR1, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGUSR2, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGPIPE, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGHUP,  SIG_DFL) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGINT,  handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGQUIT, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGTERM, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }
   if (mysignal(SIGCHLD, handle_signal) == SIG_ERR) { perror("signal"); exit(1); }

   /* SSL init */
   edg_wll_ll_log(LOG_INFO,"Initializing SSL:\n");
   if (edg_wlc_SSLInitialization() != 0) {
	   edg_wll_ll_log(LOG_CRIT,"Failed to initialize SSL. Exiting.\n");
	   exit(1);
   }

   cred_handle=edg_wll_ssl_init(SSL_VERIFY_PEER, 1,cert_file,key_file,0,0);
   if (cred_handle==NULL) {
       edg_wll_ll_log(LOG_CRIT,"Failed to initialize SSL certificates. Exiting.\n");
       exit(1);
   }

   edg_wll_ssl_get_my_subject(cred_handle, &my_subject_name);
   if (my_subject_name!=NULL) {
       edg_wll_ll_log(LOG_INFO,"  server running with certificate: %s\n",my_subject_name);
       free(my_subject_name);
   } else if (noAuth) {
       edg_wll_ll_log(LOG_INFO,"  running without certificate\n");
   } else {
       edg_wll_ll_log(LOG_CRIT,"No server credential found. Exiting.\n");
       exit(1);
   }

   /* do listen */
   edg_wll_ll_log(LOG_INFO,"Listening on port %d\n",port);
   listener_fd = do_listen(port);
   if (listener_fd == -1) {
       edg_wll_ll_log(LOG_CRIT,"Failed to listen on port %d\n",port);
       edg_wll_ssl_free(cred_handle);
       exit(-1);
   }

   client_addr_len = sizeof(client_addr);
   bzero((char *) &client_addr, client_addr_len);

   /*
    * Main loop
    */
   while (1) {
       edg_wll_ll_log(LOG_INFO,"Accepting incomming connections...\n");
       client_fd = accept(listener_fd, (struct sockaddr *) &client_addr,
                          &client_addr_len);
       if (client_fd < 0) {
          close(listener_fd);
          edg_wll_ll_log(LOG_CRIT,"Failed to accept incomming connections\n");
          perror("accept");
          edg_wll_ssl_free(cred_handle);
          exit(-1);
       }

       if (edg_wll_ssl_watch_creds(key_file,cert_file,&key_mtime,&cert_mtime) > 0) {
	       void * new_cred_handle=edg_wll_ssl_init(SSL_VERIFY_PEER, 1,cert_file,key_file,0,0);
	       if (new_cred_handle) {
		       edg_wll_ssl_free(cred_handle);
		       cred_handle = new_cred_handle;
	       }
       }
    /* FORK - change next line if fork() is not needed (for debugging for
     * example
     */
#if 1
       if ((childpid = fork()) < 0) {
             perror("fork()");
             close(client_fd);
       }
       if (childpid == 0) {
             ret=doit(client_fd,cred_handle,prefix,noIPC,noParse);
             close(client_fd);
             goto end;
       }
       if (childpid > 0) {
             close(client_fd);
       }
#else
       ret=doit(client_fd,cred_handle,prefix,msg_sock);
       close(client_fd);
#endif
    } /* while */

end:
   close(listener_fd);
   edg_wll_ssl_free(cred_handle);
   exit(ret);
}
