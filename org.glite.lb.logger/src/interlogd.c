#ident "$Header$"

/*
   interlogger - collect events from local-logger and send them to logging and bookkeeping servers

*/
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <globus_common.h>

#include "glite/wmsutils/tls/ssl_helpers/ssl_inits.h"
#include "glite/wmsutils/tls/ssl_helpers/ssl_pthreads.h"
#include "interlogd.h"
#include "glite/lb/consumer.h"
#include "glite/lb/dgssl.h"

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
static int killflg = 0;

int TIMEOUT = DEFAULT_TIMEOUT;

proxy_cred_desc *cred_handle;
pthread_mutex_t cred_handle_lock = PTHREAD_MUTEX_INITIALIZER;

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
	       , program_name, program_name);
	exit(status);
}


/* Option flags and variables */
static int debug;
static int verbose = 0;
char *file_prefix = DEFAULT_PREFIX;
int bs_only = 0;

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
  char *dummy = NULL,*p;

  program_name = argv[0];

  setlinebuf(stdout);
  setlinebuf(stderr);

  i = decode_switches (argc, argv);

  if ((p = getenv("EDG_WL_INTERLOG_TIMEOUT"))) TIMEOUT = atoi(p);

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

  /* try to get default credential file names from globus */
  if(proxy_get_filenames(NULL,0, &dummy, &CAcert_dir, &dummy, &cert_file, &key_file) < 0) {
    il_log(LOG_CRIT, "Failed to acquire credential file names. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  il_log(LOG_INFO, "Initializing SSL...\n");
  if(edg_wlc_SSLInitialization() < 0) {
    il_log(LOG_CRIT, "Failed to initialize SSL. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  cred_handle = edg_wll_ssl_init(SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0, cert_file, key_file, 0, 0);
  if(cred_handle == NULL) {
    il_log(LOG_CRIT, "Failed to initialize SSL certificates. Exiting.\n");
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

  if (edg_wlc_SSLLockingInit() != 0) {
  	il_log(LOG_CRIT, "Failed to initialize SSL locking. Exiting.\n");
  	exit(EXIT_FAILURE);
  }

  /* find all unsent events waiting in files */
  { 
	  pthread_t rid;

	  il_log(LOG_INFO, "Starting recovery thread...\n");
	  if(pthread_create(&rid, NULL, recover_thread, NULL) < 0) {
		  il_log(LOG_CRIT, "Failed to start recovery thread: %s\n", strerror(errno));
		  exit(EXIT_FAILURE);
	  }
	  pthread_detach(rid);
  }

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
