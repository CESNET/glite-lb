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


/*
   interlogger - collect events from local-logger and send them to logging and bookkeeping servers

*/
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "interlogd.h"
#include "glite/lb/log_proto.h"
#include "glite/security/glite_gss.h"
#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#endif

#define EXIT_FAILURE 1
#if defined(IL_NOTIFICATIONS)
#define DEFAULT_PREFIX "/tmp/notif_events"
#define DEFAULT_SOCKET "/tmp/notif_interlogger.sock"
#define DEFAULT_PIDFILE "/var/glite/glite-lb-notif-interlogd.pid"
#else
#define DEFAULT_PREFIX EDG_WLL_LOG_PREFIX_DEFAULT
#define DEFAULT_SOCKET "/tmp/interlogger.sock"
#define DEFAULT_PIDFILE  "/var/glite/glite-lb-interlogd.pid"
#endif



/* The name the program was run with, stripped of any leading path. */
char *program_name;
int killflg = 0;

int TIMEOUT = DEFAULT_TIMEOUT;

cred_handle_t *cred_handle = NULL;
pthread_mutex_t cred_handle_lock = PTHREAD_MUTEX_INITIALIZER;

time_t key_mtime = 0, cert_mtime = 0;

static char *pidfile = DEFAULT_PIDFILE;

static void usage (int status)
{
	printf("%s - \n"
	       "  collect events from local-logger and send them to logging and bookkeeping servers\n"
	       "Usage: %s [OPTION]... [FILE]...\n"
	       "Options:\n"
	       "  -h, --help                 display this help and exit\n"
	       "  -V, --version              output version information and exit\n"
	       "  -d, --debug                do not run as daemon\n"
	       "  -f, --file-prefix <prefix> path and prefix for event files\n"
	       "  -c, --cert <file>          location of server certificate\n"
	       "  -k, --key  <file>          location of server private key\n"
	       "  -C, --CAdir <dir>          directory containing CA certificates\n"
	       "  -b, --book                 send events to bookkeeping server only\n"
	       "  -i, --pidfile		         pid file\n"
	       "  -l, --log-server <host>    specify address of log server\n"
	       "  -s, --socket <path>        non-default path of local socket\n"
	       "  -L, --lazy [<timeout>]     be lazy when closing connections to servers (default, timeout==0 means turn lazy off)\n"
	       "  -p, --parallel [<num>]     use <num> parallel streams to the same server\n"
	       "  -q, --queue-low <num>      queue length that enables another insertions\n"
	       "  -Q, --queue-high <num>     max queue length\n"
		   "  -F, --conf <file>			 load configuration from config file\n"
#ifdef LB_PERF
	       "  -n, --nosend               PERFTEST: consume events instead of sending\n"
	       "  -S, --nosync               PERFTEST: do not check logd files for lost events\n"
	       "  -R, --norecover            PERFTEST: do not start recovery thread\n"
	       "  -P, --noparse              PERFTEST: do not parse messages, use built-in server address\n"
#ifdef PERF_EVENTS_INLINE
	       "  -e, --event_file <file>    PERFTEST: file to read test events from\n"
	       "  -j, --njobs <n>            PERFTEST: number of jobs to send\n"
#endif
#endif
	       , program_name, program_name);
	exit(status);
}


/* Option flags and variables */
static int debug;
char *file_prefix = DEFAULT_PREFIX;
int bs_only = 0;
int lazy_close = 1;
int default_close_timeout;
size_t max_store_size;
size_t queue_size_low = 0;
size_t queue_size_high = 0;
int parallel = 1;
#ifdef LB_PERF
int nosend = 0, norecover=0, nosync=0, noparse=0;
char *event_source = NULL;
int njobs = 0;
#endif

char *cert_file = NULL;
char *key_file  = NULL;
char *CAcert_dir = NULL;
char *log_server = NULL;
char *socket_path = DEFAULT_SOCKET;
static char *conf_file = NULL;
static char *config = NULL;

static struct option const long_options[] =
{
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"debug", no_argument, 0, 'd'},
  {"file-prefix", required_argument, 0, 'f'},
  {"cert", required_argument, 0, 'c'},
  {"key", required_argument, 0, 'k'},
  {"book", no_argument, 0, 'b'},
  {"CAdir", required_argument, 0, 'C'},
  {"pidfile", required_argument, 0, 'i'},
  {"log-server", required_argument, 0, 'l'},
  {"socket", required_argument, 0, 's'},
  {"lazy", optional_argument, 0, 'L'},
  {"max-store", required_argument, 0, 'M'},
  {"parallel", optional_argument, 0, 'p'},
  {"queue_size_low", required_argument, 0, 'q'},
  {"queue_size_high", required_argument, 0, 'Q'},
  {"conf", required_argument, 0, 'F'},
#ifdef LB_PERF
  {"nosend", no_argument, 0, 'n'},
  {"nosync", no_argument, 0, 'S'},
  {"norecover", no_argument, 0, 'R'},
  {"noparse", no_argument, 0, 'P'},
#ifdef PERF_EVENTS_INLINE
  {"event_file", required_argument, 0, 'e'},
  {"njobs", required_argument, NULL, 'j'},
#endif
#endif
  {NULL, 0, NULL, 0}
};



/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.  */
static int
decode_switches (int argc, char **argv)
{
  int c;

  debug = 0;

  while ((c = getopt_long (argc, argv,
			   "f:"  /* file prefix */
			   "h"	/* help */
			   "V"	/* version */
			   "c:"          /* certificate */
			   "k:"          /* key */
			   "C:"          /* CA dir */
			   "b"  /* only bookeeping */
			   "i:"  /* pidfile*/
			   "l:" /* log server */
			   "d" /* debug */
			   "p::" /* parallel */
			   "q:"
			   "Q:"
			   "F:" /* conf file */
#ifdef LB_PERF
			   "n" /* nosend */
			   "S" /* nosync */
			   "R" /* norecover */
			   "P" /* noparse */
#ifdef PERF_EVENTS_INLINE
			   "e:" /* event file */
			   "j:" /* num jobs */
#endif
#endif
			   "L::" /* lazy */
			   "s:" /* socket */
			   "M:" /* max-store */,
			   long_options, (int *) 0)) != EOF)
    {
      switch (c)
 	{
	case 'V':
	  printf ("interlogger %s\n", VERSION);
	  exit (0);

	case 'h':
	  usage (0);

	case 'd':
	  debug = 1;
	  break;

	case 'f':
	  file_prefix = strdup(optarg);
	  break;

	case 'c':
	  cert_file = strdup(optarg);
	  break;

	case 'k':
	  key_file = strdup(optarg);
	  break;

	case 'b':
	  bs_only = 1;
	  break;

	case 'l':
	  log_server = strdup(optarg);
	  break;

	case 'i': 
	  pidfile = strdup(optarg);
	  break;

	case 'C':
	  CAcert_dir = strdup(optarg);
	  break;

	case 's':
	  socket_path = strdup(optarg);
	  break;

	case 'L':
		lazy_close = 1;
		if(optarg)
		        default_close_timeout = atoi(optarg);
			if(default_close_timeout == 0) {
				default_close_timeout = TIMEOUT;
				lazy_close = 0;
			}
		else
			default_close_timeout = TIMEOUT;
		break;

	case 'M':
		max_store_size = atoi(optarg);
		break;

	case 'p':
		if(optarg)
			parallel = atoi(optarg);
		else
			parallel = 4;
		break;

	case 'q':
		queue_size_low = atoi(optarg);
		break;

	case 'Q':
		queue_size_high = atoi(optarg);
		break;

	case 'F':
		conf_file = strdup(optarg);
		break;

#ifdef LB_PERF
	case 'n':
		nosend = 1;
		break;

	case 'R':
		norecover = 1;
		break;

	case 'S':
		nosync = 1;
		break;

	case 'P':
		noparse = 1;
		break;

#ifdef PERF_EVENTS_INLINE
	case 'e':
		event_source = strdup(optarg);
		break;

	case 'j':
		njobs = atoi(optarg);
		break;
#endif
#endif

	default:
	  usage (EXIT_FAILURE);
	}
    }

  return optind;
}


char *load_conf_file(char *filename)
{
	struct stat fs;
	FILE *cf;
	char *s;

	if(stat(filename, &fs) < 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
				"Could not stat config file %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}
	s = malloc(fs.st_size + 1);
	if(s == NULL) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Not enough memory for config file");
		exit(EXIT_FAILURE);
	}
	cf = fopen(filename, "r");
	if(cf == NULL) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
				"Error opening config file %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(fread(s, fs.st_size, 1, cf) != 1) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
				"Error reading config file %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fclose(cf);
	s[fs.st_size] = 0;
	return s;
}

static int	received_signal = 0;

static void handle_signal(int num)
{
	received_signal	= num;
}


void do_handle_signal() {

	if (received_signal == 0) return;

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Received signal %d\n", received_signal);

	switch(received_signal) {
	case SIGHUP:
		/* TODO: reload all external configurations, see
		https://rt3.cesnet.cz/rt/Ticket/Display.html?id=24879 */
		glite_common_log_reread();
		break;

	case SIGUSR1:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
				 "Logging priority is now %s for %s, %s for %s and %s for %s\n", 
				 glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_SECURITY)),
				 LOG_CATEGORY_SECURITY,
				 glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_ACCESS)),
				 LOG_CATEGORY_ACCESS,
				 glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_CONTROL)),
				 LOG_CATEGORY_CONTROL);
		break;

	case SIGUSR2:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
				 "Logging priority is now %s for %s and %s for %s\n", 
				 glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_LB)),
				 LOG_CATEGORY_LB,
				 glite_common_log_priority_to_string(glite_common_log_get_priority(LOG_CATEGORY_LB_IL)),
				 IL_LOG_CATEGORY);
		break;

	case SIGPIPE:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Broken pipe, lost communication channel.\n");
		break;

	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		killflg++;
		break;

	}

	received_signal = 0;
}


int
main (int argc, char **argv)
{
  int i;
  char *p;
  edg_wll_GssStatus gss_stat;
  int ret;
  FILE *pidf;

  program_name = argv[0];

  setlinebuf(stdout);
  setlinebuf(stderr);

  if ((p = getenv("EDG_WL_INTERLOG_TIMEOUT"))) TIMEOUT = atoi(p);

  i = decode_switches (argc, argv);

  if(glite_common_log_init()) {
	  fprintf(stderr, "glite_common_log_init() failed, exiting.\n");
	  exit(EXIT_FAILURE);
  }

  /* parse config file, if any */
  if(conf_file != NULL) {
	  config = load_conf_file(conf_file);
  }

  /* check for reasonable queue lengths */
  if((queue_size_low == 0 && queue_size_high > 0) ||
     (queue_size_low > queue_size_high)) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "max queue length -Q must be greater than low queue length -q, both or none must be specified!");
	  exit(EXIT_FAILURE);
  }

  /* force -b if we do not have log server */
  if(log_server == NULL) {
    log_server = strdup(DEFAULT_LOG_SERVER);
    bs_only = 1;
  }

  /* initialize error reporting */
  if(init_errors()) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to initialize error message subsystem. Exiting.");
	  exit(EXIT_FAILURE);
  }

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR
      || signal(SIGABRT, handle_signal) == SIG_ERR
      || signal(SIGTERM, handle_signal) == SIG_ERR
      || signal(SIGINT, handle_signal) == SIG_ERR) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to setup signal handlers: %s, exiting.",
			   strerror(errno));
	  exit(EXIT_FAILURE);
  }

/* just try it before deamonizing to be able to complain aloud */
  if (!(pidf = fopen(pidfile,"w"))) {
	perror(pidfile);
	exit(EXIT_FAILURE);
  }
  fclose(pidf);

  if(!debug &&
     (daemon(0,0) < 0)) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to daemonize itself: %s, exiting.",
			   strerror(errno));
	  exit(EXIT_FAILURE);
  }

  pidf = fopen(pidfile,"w"); assert(pidf); /* XXX */
  fprintf(pidf,"%d\n",getpid());
  fclose(pidf);

  umask(S_IRWXG | S_IRWXO);

#ifdef LB_PERF
  /* this must be called after installing signal handlers */
  glite_wll_perftest_init(NULL, /* host */
			  NULL, /* user */
			  NULL, /* test name */
			  event_source,
			  njobs);
#endif

  if(input_queue_attach() < 0) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to initialize input queue: %s", 
			   error_get_msg());
	  exit(EXIT_FAILURE);
  }
  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Initialized input queue.");

  /* initialize output queues */
  if(queue_list_init(log_server) < 0) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to initialize output event queues: %s", 
			   error_get_msg());
	  exit(EXIT_FAILURE);
  }
  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Initialized event queues.");
  if(lazy_close)
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "  using lazy mode when closing connections, timeout %d",
			   default_close_timeout);

  /* get credentials */
  if (CAcert_dir)
     setenv("X509_CERT_DIR", CAcert_dir, 1);
  edg_wll_gss_watch_creds(cert_file,&cert_mtime);
  cred_handle = malloc(sizeof(*cred_handle));
  if(cred_handle == NULL) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to allocate structure for credentials.");
	  exit(EXIT_FAILURE);
  }
  cred_handle->creds = NULL;
  cred_handle->counter = 0;
  ret = edg_wll_gss_acquire_cred_gsi(cert_file, key_file, &cred_handle->creds, &gss_stat);
  if (ret) {
     char *gss_err = NULL;

     if (ret == EDG_WLL_GSS_ERROR_GSS)
	edg_wll_gss_get_error(&gss_stat, "edg_wll_gss_acquire_cred_gsi()", &gss_err);
     glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_FATAL, "Failed to load GSI credential: %s, exiting.",
		      (gss_err) ? gss_err : "edg_wll_gss_acquire_cred_gsi() failed");
     if (gss_err)
	free(gss_err);
     exit(EXIT_FAILURE);
  }
  glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, "Using certificate %s", cred_handle->creds->name);

  /* parse config, initialize plugins */
  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Initializing plugins:\n");
  if(config) {
	  char *s = strstr(config, "[interlogd]");
	  char *p;
	  char name[MAXPATHLEN+1];

	  if(s) {
		  /* next line */
		  s = strchr(s, '\n');
		  if(s) s++;
		  while(s) {
			  if(*s == 0 || *s == '[')
				  break;
			  /* parse line */
			  p = strchr(s, '\n');
			  if(p) {
				  *p = 0;
			  }
			  /* XXX possible overflow by long line in config file */
			  ret = sscanf(s, " plugin =%s", name);
			  if(p) *p = '\n';
			  if(ret > 0) {
				  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "  loading plugin %s\n", name);
				  if(plugin_mgr_init(name, config) < 0) {
					  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Failed to load plugin %s: %s\n", name, error_get_msg());
				  }
			  }
			  s = p + 1;
		  }
	  }
  }

#ifndef PERF_EMPTY
  /* find all unsent events waiting in files */
#ifdef LB_PERF
  if(norecover) {
	  if(event_store_init(file_prefix) < 0) {
		  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to initialize event stores: %s", 
				   error_get_msg());
		  exit(EXIT_FAILURE);
	  }
  } else
#endif
  {
	  pthread_t rid;

	  if(pthread_create(&rid, NULL, recover_thread, NULL) < 0) {
		  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Failed to start recovery thread: %s", strerror(errno));
		  exit(EXIT_FAILURE);
	  }
	  pthread_detach(rid);
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Started recovery thread.");
  }
#endif

  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Using %d threads for parallel delivery.", parallel);
  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Entering main loop.");

  /* do the work */
  if(loop() < 0) {
	  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Fatal error: %s", error_get_msg());
	  if (killflg) {
		  input_queue_detach();
		  unlink(pidfile);
		  exit(EXIT_FAILURE);
	  }
  }
  glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Done!");
  input_queue_detach();
  unlink(pidfile);

  exit (0);
}
