#ident "$Header$"

/*
   interlogger - collect events from local-logger and send them to logging and bookkeeping servers

*/
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <globus_common.h>

#include "interlogd.h"
#include "glite/lb/consumer.h"
#include "glite/security/glite_gss.h"
#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#endif

#define EXIT_FAILURE 1
#if defined(IL_NOTIFICATIONS)
#define DEFAULT_PREFIX "/tmp/notif_events"
#define DEFAULT_SOCKET "/tmp/notif_interlogger.sock"
#else
#define DEFAULT_PREFIX "/tmp/dglogd.log"
#define DEFAULT_SOCKET "/tmp/interlogger.sock"
#endif


/* The name the program was run with, stripped of any leading path. */
char *program_name;
int killflg = 0;

int TIMEOUT = DEFAULT_TIMEOUT;

gss_cred_id_t cred_handle = GSS_C_NO_CREDENTIAL;
pthread_mutex_t cred_handle_lock = PTHREAD_MUTEX_INITIALIZER;

time_t key_mtime = 0, cert_mtime = 0;

static void usage (int status)
{
	printf("%s - \n"
	       "  collect events from local-logger and send them to logging and bookkeeping servers\n"
	       "Usage: %s [OPTION]... [FILE]...\n"
	       "Options:\n"
	       "  -h, --help                 display this help and exit\n"
	       "  -V, --version              output version information and exit\n"
	       "  -d, --debug                do not run as daemon\n"
	       "  -v, --verbose              print extensive debug output\n"
	       "  -f, --file-prefix <prefix> path and prefix for event files\n"
	       "  -c, --cert <file>          location of server certificate\n"
	       "  -k, --key  <file>          location of server private key\n"
	       "  -C, --CAdir <dir>          directory containing CA certificates\n"
	       "  -b, --book                 send events to bookkeeping server only\n"
	       "  -l, --log-server <host>    specify address of log server\n"
	       "  -s, --socket <path>        non-default path of local socket\n"
	       "  -L, --lazy [<timeout>]     be lazy when closing connections to servers\n"
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
static int verbose = 0;
char *file_prefix = DEFAULT_PREFIX;
int bs_only = 0;
int lazy_close = 0;
int default_close_timeout;
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

static struct option const long_options[] =
{
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},
  {"debug", no_argument, 0, 'd'},
  {"file-prefix", required_argument, 0, 'f'},
  {"cert", required_argument, 0, 'c'},
  {"key", required_argument, 0, 'k'},
  {"book", no_argument, 0, 'b'},
  {"CAdir", required_argument, 0, 'C'},
  {"log-server", required_argument, 0, 'l'},
  {"socket", required_argument, 0, 's'},
  {"lazy", optional_argument, 0, 'L'},
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
			   "v"  /* verbose */
			   "c:"          /* certificate */
			   "k:"          /* key */
			   "C:"          /* CA dir */
			   "b"  /* only bookeeping */
                           "l:" /* log server */
			   "d" /* debug */
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
			   "s:", /* socket */
			   long_options, (int *) 0)) != EOF)
    {
      switch (c)
 	{
	case 'V':
	  printf ("interlogger %s\n", VERSION);
	  exit (0);

	case 'v':
	  verbose = 1;
	  break;

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
		else
			default_close_timeout = TIMEOUT;
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


void handle_signal(int num) {
    il_log(LOG_DEBUG, "Received signal %d\n", num);
    killflg++;
}

int
main (int argc, char **argv)
{
  int i;
  char *p;
  edg_wll_GssStatus gss_stat;
  int ret;

  program_name = argv[0];

  setlinebuf(stdout);
  setlinebuf(stderr);

  if ((p = getenv("EDG_WL_INTERLOG_TIMEOUT"))) TIMEOUT = atoi(p);

  i = decode_switches (argc, argv);

  /* force -b if we do not have log server */
  if(log_server == NULL) {
    log_server = strdup(DEFAULT_LOG_SERVER);
    bs_only = 1;
  }

  if(init_errors(verbose ? LOG_DEBUG : LOG_WARNING)) {
    fprintf(stderr, "Failed to initialize error message subsys. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGPIPE, handle_signal) == SIG_ERR
      || signal(SIGABRT, handle_signal) == SIG_ERR
      || signal(SIGTERM, handle_signal) == SIG_ERR
      || signal(SIGINT, handle_signal) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

#ifdef LB_PERF
  /* this must be called after installing signal handlers */
  glite_wll_perftest_init(NULL, /* host */
			  NULL, /* user */
			  NULL, /* test name */
			  event_source,
			  njobs);
#endif

  il_log(LOG_INFO, "Initializing input queue...\n");
  if(input_queue_attach() < 0) {
    il_log(LOG_CRIT, "Failed to initialize input queue: %s\n", error_get_msg());
    exit(EXIT_FAILURE);
  }

  /* initialize output queues */
  il_log(LOG_INFO, "Initializing event queues...\n");
  if(queue_list_init(log_server) < 0) {
    il_log(LOG_CRIT, "Failed to initialize output event queues: %s\n", error_get_msg());
    exit(EXIT_FAILURE);
  }
  if(lazy_close)
	  il_log(LOG_DEBUG, "  using lazy mode when closing connections, timeout %d\n",
		 default_close_timeout);

  if (CAcert_dir)
     setenv("X509_CERT_DIR", CAcert_dir, 1);

  edg_wll_gss_watch_creds(cert_file,&cert_mtime);
  ret = edg_wll_gss_acquire_cred_gsi(cert_file, key_file, &cred_handle, NULL, &gss_stat);
  if (ret) {
     char *gss_err = NULL;
     char *str;

     if (ret == EDG_WLL_GSS_ERROR_GSS)
	edg_wll_gss_get_error(&gss_stat, "edg_wll_gss_acquire_cred_gsi()", &gss_err);
     asprintf(&str, "Failed to load GSI credential: %s\n",
	      (gss_err) ? gss_err : "edg_wll_gss_acquire_cred_gsi() failed"); 
     il_log(LOG_CRIT, str);
     free(str);
     if (gss_err)
	free(gss_err);
     exit(EXIT_FAILURE);
  }
  
  if(!debug &&
     (daemon(0,0) < 0)) {
    perror("daemon");
    exit(EXIT_FAILURE);
  }

  if (globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS)	{
  	il_log(LOG_CRIT, "Failed to initialize Globus common module\n");
  	exit(EXIT_FAILURE);
  }

#ifndef PERF_EMPTY
  /* find all unsent events waiting in files */
#ifdef LB_PERF
  if(norecover) {
	  if(event_store_init(file_prefix) < 0) {
		  il_log(LOG_CRIT, "Failed to init event stores: %s\n", error_get_msg());
		  exit(EXIT_FAILURE);
	  }
  } else
#endif
  { 
	  pthread_t rid;

	  il_log(LOG_INFO, "Starting recovery thread...\n");
	  if(pthread_create(&rid, NULL, recover_thread, NULL) < 0) {
		  il_log(LOG_CRIT, "Failed to start recovery thread: %s\n", strerror(errno));
		  exit(EXIT_FAILURE);
	  }
	  pthread_detach(rid);
  }
#endif

  il_log(LOG_INFO, "Entering main loop...\n");

  /* do the work */
  if(loop() < 0) {
    il_log(LOG_CRIT, "Fatal error: %s\n", error_get_msg());
    if (killflg) {
      input_queue_detach();
      exit(EXIT_FAILURE);
    }
  }
  il_log(LOG_INFO, "Done!\n");
  input_queue_detach();

  exit (0);
}
