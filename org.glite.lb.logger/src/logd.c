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

#include "glite/lb/context-int.h"
#include "logd_proto.h"
#include "glite/lb/consumer.h"
#include "glite/security/glite_gss.h"
#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#endif

static const char rcsid[] = "@(#)$Id$";
static int verbose = 0;
static int debug = 0;
static int port = EDG_WLL_LOG_PORT_DEFAULT;
static char *prefix = EDG_WLL_LOG_PREFIX_DEFAULT;
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
	{ "verbose", no_argument, 0, 'v' },
	{ "debug", no_argument, 0, 'd' },
	{ "port", required_argument, 0, 'p' },
	{ "file-prefix", required_argument, 0, 'f' },
	{ "cert", required_argument, 0, 'c' },
	{ "key", required_argument, 0, 'k' },
	{ "CAdir", required_argument, 0, 'C' },
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
doit(int socket, gss_cred_id_t cred_handle, char *file_name_prefix, int noipc, int noparse)
{
    char 	*subject;
    int 	ret;
    struct timeval timeout = {10,0};
    edg_wll_GssConnection	con;
    edg_wll_GssStatus	gss_stat;
    gss_buffer_desc	gss_token = GSS_C_EMPTY_BUFFER;
    gss_name_t	client_name = GSS_C_NO_NAME;
    OM_uint32	min_stat;
    gss_OID	name_type = GSS_C_NO_OID;

    /* authentication */
    edg_wll_ll_log(LOG_INFO,"Processing authentication:\n");
// FIXME - put here some meaningfull value of timeout + do somthing if timeouted
    ret = edg_wll_gss_accept(cred_handle,socket,&timeout,&con, &gss_stat);
    if (ret) {
	    edg_wll_ll_log(LOG_ERR,"edg_wll_gss_accept() failed\n");
	    return(-1);
    }

    gss_stat.major_status = gss_inquire_context(&gss_stat.minor_status, con.context,
	                                        &client_name, NULL, NULL, NULL, NULL,
						NULL, NULL);
    if (GSS_ERROR(gss_stat.major_status)) {
       char *gss_err;
       edg_wll_gss_get_error(&gss_stat, "Cannot read client identification", &gss_err);
       edg_wll_ll_log(LOG_WARNING, "%s\n", gss_err);
       free(gss_err);
    } else {
       gss_stat.major_status = gss_display_name(&gss_stat.minor_status, client_name,
	                                        &gss_token, &name_type);
       if (GSS_ERROR(gss_stat.major_status)) {
	  char *gss_err;
	  edg_wll_gss_get_error(&gss_stat, "Cannot process client identification", &gss_err);
	  edg_wll_ll_log(LOG_WARNING, "%s\n", gss_err);
	  free(gss_err);
       }
    }

    if (GSS_ERROR(gss_stat.major_status) ||
        edg_wll_gss_oid_equal(name_type, GSS_C_NT_ANONYMOUS)) {
       edg_wll_ll_log(LOG_INFO,"  User not authenticated, setting as \"%s\". \n",EDG_WLL_LOG_USER_DEFAULT);
       subject=strdup(EDG_WLL_LOG_USER_DEFAULT);
    } else {
       edg_wll_ll_log(LOG_INFO,"  User successfully authenticated as:\n");
       edg_wll_ll_log(LOG_INFO, "   %s\n", (char *)gss_token.value);
       subject=gss_token.value;
       memset(&gss_token.value, 0, sizeof(gss_token.value));
    }

    ret = edg_wll_log_proto_server(&con,subject,file_name_prefix,noipc,noparse);

    edg_wll_gss_close(&con, NULL);
    if (subject) free(subject);
    if (gss_token.length)
       gss_release_buffer(&min_stat, &gss_token);
    if (client_name != GSS_C_NO_NAME)
       gss_release_name(&min_stat, &client_name);
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

   int listener_fd;
   int client_fd;
   struct sockaddr_in client_addr;
   int client_addr_len;

   char *my_subject_name = NULL;

   time_t	cert_mtime = 0, key_mtime = 0;
   OM_uint32	min_stat;
   edg_wll_GssStatus	gss_stat;
   gss_cred_id_t	cred = GSS_C_NO_CREDENTIAL;


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

   if (CAcert_dir)
      setenv("X509_CERT_DIR", CAcert_dir, 1);

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

#ifdef LB_PERF
   glite_wll_perftest_init(NULL, NULL, NULL, NULL, 0);
#endif
 
   edg_wll_gss_watch_creds(cert_file,&cert_mtime);
   /* XXX DK: support noAuth */
   ret = edg_wll_gss_acquire_cred_gsi(cert_file, key_file, &cred, &my_subject_name,
	 			      &gss_stat);
   if (ret) {
      /* XXX DK: call edg_wll_gss_get_error() */
      edg_wll_ll_log(LOG_CRIT, "Failed to get GSI credentials. Exiting.\n");
      exit(1);
   }

   if (my_subject_name!=NULL) {
       edg_wll_ll_log(LOG_INFO,"  server running with certificate: %s\n",my_subject_name);
       free(my_subject_name);
   } else if (noAuth) {
       edg_wll_ll_log(LOG_INFO,"  running without certificate\n");
#if 0
   /* XXX DK: */    
   } else {
       edg_wll_ll_log(LOG_CRIT,"No server credential found. Exiting.\n");
       exit(1);
#endif
   }

   /* do listen */
   edg_wll_ll_log(LOG_INFO,"Listening on port %d\n",port);
   listener_fd = do_listen(port);
   if (listener_fd == -1) {
       edg_wll_ll_log(LOG_CRIT,"Failed to listen on port %d\n",port);
       gss_release_cred(&min_stat, &cred);
       exit(-1);
   }

   client_addr_len = sizeof(client_addr);
   bzero((char *) &client_addr, client_addr_len);

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
          gss_release_cred(&min_stat, &cred);
          exit(-1);
       }

       switch (edg_wll_gss_watch_creds(cert_file,&cert_mtime)) {
	  gss_cred_id_t newcred;
	  case 0: break;
	  case 1:
	     ret = edg_wll_gss_acquire_cred_gsi(cert_file,key_file,&newcred,NULL,&gss_stat);
	     if (ret) {
		edg_wll_ll_log(LOG_WARNING, "Reloading credentials failed, continue with older\n");
	     } else {
		edg_wll_ll_log(LOG_INFO, "Reloading credentials\n");
		gss_release_cred(&min_stat, &cred);
		cred = newcred;
	     }
	     break;
	  case -1:
	     edg_wll_ll_log(LOG_WARNING, "edg_wll_gss_watch_creds failed\n");
	     break;
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
             ret=doit(client_fd,cred,prefix,noIPC,noParse);
             close(client_fd);
             goto end;
       }
       if (childpid > 0) {
             close(client_fd);
       }
#else
       ret=doit(client_fd,cred,prefix,noIPC,noParse);
       close(client_fd);
#endif
    } /* while */

end:
   close(listener_fd);
   gss_release_cred(&min_stat, &cred);
   exit(ret);
}
