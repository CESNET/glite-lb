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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <limits.h>
#include <assert.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <ares.h>
#include <ares_version.h>
#include <errno.h>

#ifdef GLITE_LB_SERVER_WITH_WS
#include "soap_version.h"
#include <stdsoap2.h>
#include "glite/security/glite_gsplugin.h"
#endif /* GLITE_LB_SERVER_WITH_WS */

#include "glite/jobid/cjobid.h"
#include "glite/security/glite_gss.h"
#include "glite/lbu/srvbones.h"
#include "glite/lbu/maildir.h"
#include "glite/lb/context.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/context-int.h"
#include "glite/lbu/log.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#include "srv_perf.h"

enum lb_srv_perf_sink sink_mode;
#endif

#include "lb_http.h"
#include "lb_proto.h"
#include "index.h"
#include "lb_authz.h"
#include "il_notification.h"
#include "stats.h"
#include "server_stats.h"
#include "db_calls.h"
#include "db_supp.h"
#include "openserver.h"
#include "authz_policy.h"

#ifdef GLITE_LB_SERVER_WITH_WS
#  if GSOAP_VERSION < 20700
	 /*	defined in <arpa/nameser.h> and it's includes
	  *	break the build
	  */
#    undef STATUS
#    undef REFUSED
#  endif
#include "LoggingAndBookkeeping.nsmap"
#endif /* GLITE_LB_SERVER_WITH_WS */

extern int edg_wll_StoreProto(edg_wll_Context ctx);

extern char *lbproxy_ilog_socket_path;
extern char *lbproxy_ilog_file_prefix;


#ifdef LB_PERF
extern void _start (void), etext (void);
#endif

#define CON_QUEUE		20	/* accept() */
#define SLAVE_OVERLOAD		10	/* queue items per slave */
#define CONNECT_TIMEOUT		30
#define IDLE_TIMEOUT		10	/* keep idle connection that many seconds */
#define REQUEST_TIMEOUT		120	/* one client may ask one slave multiple times */
					/* but only limited time to avoid DoS attacks */
#define DNS_TIMEOUT      	5	/* how long wait for DNS lookup */
#define SLAVE_CONNS_MAX		500	/* commit suicide after that many connections */

#ifndef EDG_PURGE_STORAGE
#define EDG_PURGE_STORAGE	"/tmp/purge"
#endif

#ifndef EDG_DUMP_STORAGE
#define EDG_DUMP_STORAGE	"/tmp/dump"
#endif

#ifndef JPREG_DEF_DIR
#define JPREG_DEF_DIR		"/tmp/jpreg"
#endif

/* file to store pid and generate semaphores key */
#ifndef EDG_BKSERVERD_PIDFILE
#define EDG_BKSERVERD_PIDFILE	"/var/run/glite/glite-lb-bkserverd.pid"
#endif

#ifndef GLITE_LBPROXY_SOCK_PREFIX
#define GLITE_LBPROXY_SOCK_PREFIX       "/tmp/lb_proxy_"
#endif

#define sizofa(a)		(sizeof(a)/sizeof((a)[0]))

#define	SERVICE_PROXY		DB_PROXY_JOB
#define SERVICE_SERVER		DB_SERVER_JOB
#define SERVICE_PROXY_SERVER	SERVICE_PROXY+SERVICE_SERVER



int					enable_lcas = 0;
int						debug  = 0;
int						rgma_export = 0;
static int				noAuth = 0;
static int				noIndex = 0;
static int				strict_locking = 0;
static int 				greyjobs = 0;
static int                              async_registrations = 0;
static int 				count_statistics = 1;
static int 				count_server_stats = 1;
static char 				*stats_file_prefix = NULL;
static int				hardJobsLimit = 0;
static int				hardEventsLimit = 0;
static int				hardRespSizeLimit = 0;
static char			   *dbstring = NULL,*fake_host = NULL;
int        				transactions = -1;
int					use_dbcaps = 0;
static int				fake_port = 0;
static int				slaves = 10;
static char			   *purgeStorage = EDG_PURGE_STORAGE;
static char			   *dumpStorage = EDG_DUMP_STORAGE;
static char			   *jpregDir = JPREG_DEF_DIR;
static int				jpreg = 0;
static char				*server_subject = NULL;


static time_t			purge_timeout[EDG_WLL_NUMBER_OF_STATCODES];
static time_t			notif_duration_max = 60*60*24,
				notif_duration = 60*60*2;
int				proxy_purge = 0;

static edg_wll_GssCred	mycred = NULL;
time_t					cert_mtime = 0;
char				   *cadir = NULL,
					   *vomsdir = NULL,
					   *server_key = NULL,
					   *server_cert = NULL;
static int		mode = SERVICE_SERVER;
static char             sock_store[PATH_MAX],
                        sock_serve[PATH_MAX];
static int		con_queue = CON_QUEUE;
static char             host[300];
static char *           port;
static time_t		rss_time = 60*60;
char *		policy_file = NULL;
struct _edg_wll_authz_policy	authz_policy = { NULL, 0};
static int 		exclusive_zombies = 1;
static char		**msg_brokers = NULL;
static char		**msg_prefixes = NULL;
char *		html_header = NULL;
static int	html_header_forced = 0;
static char     *gridmap = NULL;
struct _edg_wll_id_mapping id_mapping = {NULL, 0};


static struct option opts[] = {
        {"enable-lcas", 0, NULL,	'A'},
	{"cert",	1, NULL,	'c'},
	{"key",		1, NULL,	'k'},
	{"CAdir",	1, NULL,	'C'},
	{"VOMSdir",	1, NULL,	'V'},
	{"port",	1, NULL,	'p'},
#ifdef GLITE_LB_SERVER_WITH_WS
	{"wsport",  1, NULL,    'w'},
#endif	/* GLITE_LB_SERVER_WITH_WS */
	{"address",	1, NULL,	'a'},
	{"debug",	0, NULL,	'd'},
	{"rgmaexport",	0, NULL,	'r'},
	{"mysql",	1, NULL,	'm'},
	{"noauth",	0, NULL,	'n'},
	{"slaves",	1, NULL,	's'},
	{"pidfile",	1, NULL,	'i'},
	{"purge-prefix",	1, NULL,	'S'},
	{"dump-prefix",	1, NULL,	'D'},
	{"jpreg-dir",	1, NULL,	'J'},
	{"enable-jpreg-export",	1, NULL,	'j'},
	{"super-user",	1, NULL,	'R'},
//	{"super-users-file",	1, NULL,'F'},
	{"msg-conf",	1, NULL,'F'},
	{"html-header",	1, NULL,'H'},
	{"no-index",	1, NULL,	'x'},
	{"strict-locking",0, NULL,	'O'},
	{"limits",	1, NULL,	'L'},
	{"notif-dur",	1, NULL,	'N'},
	{"notif-il-sock",	1, NULL,	'X'},
	{"notif-il-fprefix",	1, NULL,	'Y'},
	{"count-statistics",	1, NULL,	'T'},
	{"count-server-stats",  1, NULL,        'e'},
        {"statistics-prefix",   1, NULL,        'f'},
	{"request-timeout",	1, NULL,	't'},
#ifdef LB_PERF
	{"perf-sink",           1, NULL,        'K'},
#endif
	{"transactions",	1,	NULL,	'b'},
	{"greyjobs",	0,	NULL,	'g'},
	{"async-registrations", 0, NULL, 'Q'},
	{"withproxy",	0,	NULL,	'B'},
	{"proxyonly",	0,	NULL,	'P'},
	{"sock",	1,	NULL,	'o'},
	{"con-queue",	1,	NULL,	'q'},
	{"proxy-il-sock",	1,	NULL,	'W'},
	{"proxy-il-fprefix",	1,	NULL,	'Z'},
	{"proxy-purge",	0,	NULL,	'G'},
	{"rss-time", 	1,	NULL,	'I'},
	{"policy",	1,	NULL,	'l'},
	{"exclusive-zombies-off",	0,	NULL,	'E'},
	{"gridmap-file",1,	NULL,   'M'},
	{NULL,0,NULL,0}
};

static const char *get_opt_string = "Ac:k:C:V:p:a:drm:M:ns:i:S:D:J:jR:F:xOL:N:X:Y:T:t:e:f:zb:gQPBo:q:W:Z:GI:l:EH:"
#ifdef GLITE_LB_SERVER_WITH_WS
	"w:"
#endif
#ifdef LB_PERF
	"K:"
#endif
;

static void usage(char *me) 
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-A, --enable-lcas\t activate LCAS-based authorization\n"
		"\t-a, --address\t use this server address (may be faked for debugging)\n"
		"\t-b, --transactions\t transactions switch (0, 1)\n"
		"\t-k, --key\t private key file\n"
		"\t-c, --cert\t certificate file\n"
		"\t-C, --CAdir\t trusted certificates directory\n"
		"\t-V, --VOMSdir\t trusted VOMS servers certificates directory\n"
		"\t-p, --port\t port to listen\n"
#ifdef GLITE_LB_SERVER_WITH_WS
		"\t-w, --wsport\t port to serve the web services requests\n"
#endif	/* GLITE_LB_SERVER_WITH_WS */
		"\t-m, --mysql\t database connect string\n"
		"\t-d, --debug\t don't run as daemon\n"
		"\t-r, --rgmaexport write state info to RGMA interface\n"
		"\t-n, --noauth\t don't check user identity with result owner\n"
		"\t-s, --slaves\t number of slave servers to fork\n"
		"\t-i, --pidfile\t file to store master pid\n"
		"\t-L, --limits\t query limits numbers in format \"events_limit:jobs_limit:size_limit\"\n"
		"\t-M, --gridmap-file\tgridmap-file to map clients identities\"\n"
		"\t-N, --notif-dur default[:max]\t Duration of notification registrations in seconds (default and maximal)\n"
		"\t-S, --purge-prefix\t purge files full-path prefix\n"
		"\t-D, --dump-prefix\t dump files full-path prefix\n"
		"\t-J, --jpreg-dir\t JP registration temporary files prefix (implies '-j')\n"
		"\t-j, --enable-jpreg-export\t enable JP registration export (disabled by default)\n"
		"\t--super-user\t user allowed to bypass authorization and indexing\n"
		"\t--no-index=1\t don't enforce indices for superusers\n"
		"\t          =2\t don't enforce indices at all\n"
		"\t--strict-locking=1\t lock jobs also on storing events (may be slow)\n"
		"\t--notif-il-sock\t socket to send notifications\n"
		"\t--notif-il-fprefix\t file prefix for notifications\n"
		"\t--count-statistics=1\t count certain statistics on jobs\n"
		"\t                  =2\t ... and allow anonymous access\n"
		"\t--count-server-stats=0\t do not count server statistics\n"
                "\t                    =1\t count server statistics (default)\n"
                "\t                    =2\t count server statistics, restrict access only to superusers\n"
                "\t--statistics-prefix statistics persistency files full-path prefix\n"
		"\t-t, --request-timeout\t request timeout for one client\n"
#ifdef LB_PERF
		"\t-K, --perf-sink\t where to sink events\n"
#endif
		"\t-g,--greyjobs\t allow delayed registration (grey jobs), implies --strict-locking\n"
		"\t-Q,--async-registrations\t allow asynchronous registrations (weaker form of grey jobs) \n"
		"\t-P,--proxyonly\t	run only proxy service\n"
		"\t-B,--withproxy\t	run both server and proxy service\n"
		"\t-o,--sock\t path-name to the local socket for communication with LB proxy\n"
		"\t-q,--con-queue\t size of the connection queue (accept)\n"
		"\t-W,--proxy-il-sock\t socket to send events to\n"
		"\t-Z,--proxy-il-fprefix\t file prefix for events\n"
		"\t-G,--proxy-purge\t enable automatic purge on proxy service (disabled by default)\n"
		"\t-I,--rss-time\t age (in seconds) of job states published via RSS\n"
		"\t-l,--policy\tauthorization policy file\n"
		"\t-E,--exclusive-zombies-off\twith 'exclusive' flag, allow reusing IDs of purged jobs\n"
		"\t-F,--msg-conf\t path to configuration file with messaging settings\n"
		"\t-H,--html-header\t path to HTML header file for customized/branded HTML output\n"

	,me);
}

static int wait_for_open(edg_wll_Context,const char *);
static int decrement_timeout(struct timeval *, struct timeval, struct timeval);
static int asyn_gethostbyaddr(char **, char **, const struct sockaddr *, int, struct timeval *, int );
static int add_root(edg_wll_Context, char *, authz_action);
static int parse_limits(char *, int *, int *, int *);
static int check_mkdir(const char *);


/*
 *	SERVER BONES structures and handlers
 */
int bk_clnt_data_init(void **);

	/*
	 *	Serve & Store handlers
	 */
int bk_clnt_reject(int);
int bk_handle_connection(int, struct timeval *, void *);
int bk_accept_serve(int, struct timeval *, void *);
int bk_accept_store(int, struct timeval *, void *);
int bk_clnt_disconnect(int, struct timeval *, void *);

#ifdef GLITE_LB_SERVER_WITH_WS
	/*
	 *	WS handlers
	 */
int bk_handle_ws_connection(int, struct timeval *, void *);
int bk_accept_ws(int, struct timeval *, void *);
int bk_ws_clnt_reject(int);
int bk_ws_clnt_disconnect(int, struct timeval *, void *);
#endif 	/*GLITE_LB_SERVER_WITH_WS */

        /*
         *      Proxy handlers
         */
int bk_clnt_reject_proxy(int);
int bk_handle_connection_proxy(int, struct timeval *, void *);
int bk_clnt_disconnect_proxy(int, struct timeval *, void *);

  
  #define SERVICE_SERVER_START	0 
  #define SRV_SERVE			0
  #define SRV_STORE			1
#ifdef GLITE_LB_SERVER_WITH_WS
  #define SRV_WS			2
  #define SERVICE_SERVER_SIZE		3

  #define SERVICE_PROXY_START		3
  #define SRV_SERVE_PROXY		3
  #define SRV_STORE_PROXY		4
  #define SERVICE_PROXY_SIZE		2
#else
  #define SERVICE_SERVER_SIZE		2

  #define SERVICE_PROXY_START		2
  #define SRV_SERVE_PROXY		2
  #define SRV_STORE_PROXY		3
  #define SERVICE_PROXY_SIZE		2
#endif	/* GLITE_LB_SERVER_WITH_WS */



static struct glite_srvbones_service service_table[] = {
	{ "serve",	-1, bk_handle_connection, bk_accept_serve, bk_clnt_reject, bk_clnt_disconnect },
	{ "store",	-1, bk_handle_connection, bk_accept_store, bk_clnt_reject, bk_clnt_disconnect },
#ifdef GLITE_LB_SERVER_WITH_WS
	{ "WS",		-1, bk_handle_ws_connection, bk_accept_ws, bk_ws_clnt_reject, bk_ws_clnt_disconnect },
#endif	/* GLITE_LB_SERVER_WITH_WS */
	{ "serve_proxy",	-1, bk_handle_connection_proxy, bk_accept_serve, bk_clnt_reject_proxy, bk_clnt_disconnect_proxy },
	{ "store_proxy",	-1, bk_handle_connection_proxy, bk_accept_store, bk_clnt_reject_proxy, bk_clnt_disconnect_proxy }
};

struct clnt_data_t {
	edg_wll_Context			ctx;
#ifdef GLITE_LB_SERVER_WITH_WS
	struct soap			   *soap;
#endif	/* GLITE_LB_SERVER_WITH_WS */
	glite_lbu_DBContext	dbctx;
	int			dbcaps;
	edg_wll_QueryRec	  **job_index,**notif_index;
	edg_wll_IColumnRec	   *job_index_cols,*notif_index_cols;;
	int			mode;
};



int main(int argc, char *argv[])
{
	int			i;
	char *portstr = NULL;
	int					opt;
	char				pidfile[PATH_MAX] = EDG_BKSERVERD_PIDFILE,
					   *name;
#ifdef GLITE_LB_SERVER_WITH_WS
	char			   *ws_port;
#endif	/* GLITE_LB_SERVER_WITH_WS */
	FILE			   *fpid;
	edg_wll_Context		ctx;
	edg_wll_GssStatus	gss_code;
	struct timeval		to;
	int 			request_timeout = REQUEST_TIMEOUT;
	char 			socket_path_prefix[PATH_MAX] = GLITE_LBPROXY_SOCK_PREFIX;
	char *			msg_conf = NULL;


	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	asprintf(&port, "%d", GLITE_JOBID_DEFAULT_PORT);
#ifdef GLITE_LB_SERVER_WITH_WS
	asprintf(&ws_port, "%d", GLITE_JOBID_DEFAULT_PORT+3);
#endif 	/* GLITE_LB_SERVER_WITH_WS */
	server_cert = server_key = cadir = vomsdir = NULL;

/* no magic here: 1 month, 3 and 7 days */
	for (i=0; i < EDG_WLL_NUMBER_OF_STATCODES; i++) purge_timeout[i] = 60*60*24*31;	
	purge_timeout[EDG_WLL_JOB_CLEARED] = 60*60*24*3;
	purge_timeout[EDG_WLL_JOB_ABORTED] = 60*60*24*7;
	purge_timeout[EDG_WLL_JOB_CANCELLED] = 60*60*24*7;


	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context, exiting.\n");
		exit(1);
	}

	memset(host, 0, sizeof host);
	edg_wll_gss_gethostname(host,sizeof host);
	host[sizeof host - 1] = 0;

	while ((opt = getopt_long(argc,argv,get_opt_string,opts,NULL)) != EOF) switch (opt) {
		case 'A': enable_lcas = 1; break;
		case 'a': fake_host = strdup(optarg); break;
		case 'b': transactions = atoi(optarg); break;
		case 'c': server_cert = optarg; break;
		case 'k': server_key = optarg; break;
		case 'C': cadir = optarg; break;
		case 'V': vomsdir = optarg; break;
		case 'p': free(port); port = strdup(optarg); break;
#ifdef GLITE_LB_SERVER_WITH_WS
		case 'w': free(ws_port); ws_port = strdup(optarg); break;
#endif /* GLITE_LB_SERVER_WITH_WS */
		case 'd': debug = 1; break;
		case 'r': rgma_export = 1; break;
		case 'm': dbstring = optarg; break;
		case 'n': noAuth = 1; break;
		case 's': slaves = atoi(optarg); break;
		case 'S': purgeStorage = optarg; break;
		case 'D': dumpStorage = optarg; break;
		case 'J': jpregDir = optarg; jpreg = 1; break;
		case 'j': jpreg = 1; break;
		case 'L':
			if ( !parse_limits(optarg, &hardJobsLimit, &hardEventsLimit, &hardRespSizeLimit) )
			{
				usage(name);
				return 1;
			}
			break;
		case 'M': gridmap = strdup(optarg); break;
		case 'N': {
				int	std,max;
				switch (sscanf(optarg,"%d:%d",&std,&max)) {
					case 2: notif_duration_max = max;
						/* fallthrough */
					case 1: notif_duration = std;
						break;
					default: 
						usage(name);
						return 1;
				}
			}  break;
		case 'X': notif_ilog_socket_path = strdup(optarg); break;
		case 'Y': notif_ilog_file_prefix = strdup(optarg); break;
		case 'i': strcpy(pidfile,optarg); break;
		case 'R': add_root(ctx, optarg, ADMIN_ACCESS); break;
		case 'F': msg_conf = strdup(optarg); break;
		case 'H': html_header = strdup(optarg); html_header_forced = 1; break;
		case 'x': noIndex = atoi(optarg);
			  if (noIndex < 0 || noIndex > 2) { usage(name); return 1; }
			  break;
		case 'O': strict_locking = 1;
			  break;
		case 'T': count_statistics = atoi(optarg);
			  break;
		case 'e': count_server_stats = atoi(optarg);
                          break;
                case 'f': stats_file_prefix = strdup(optarg);
                          break;
		case 't': request_timeout = atoi(optarg);
			  break;
#ifdef LB_PERF
		case 'K': sink_mode = atoi(optarg);
			  break;
#endif
		case 'g': greyjobs = strict_locking = 1; 
			  break;
	        case 'Q': async_registrations = 1; /* XXX should set strict locking too? */
		          break;
		case 'P': mode = SERVICE_PROXY;
			  break;
		case 'B': mode = SERVICE_PROXY_SERVER;
			  break;
		case 'o': strcpy(socket_path_prefix, optarg);
			  break;
		case 'q': con_queue = atoi(optarg); 
			  break;
		case 'W': lbproxy_ilog_socket_path = strdup(optarg); 
			  break;
		case 'Z': lbproxy_ilog_file_prefix = strdup(optarg);
			  break;
		case 'G': proxy_purge = 1;
			  break;
		case 'I': rss_time = atol(optarg);
			  break;
		case 'l': policy_file = strdup(optarg);
			  break;
		case 'E': exclusive_zombies = 0;
			  break;
		case '?': usage(name); return 1;
	}

	if ( optind < argc ) { usage(name); return 1; }

	setlinebuf(stdout);
	setlinebuf(stderr);

	if (glite_common_log_init()) {
		fprintf(stderr,"glite_common_log_init() failed, exiting.");
		exit(1);
	}
	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Initializing...");

	if (mode & SERVICE_PROXY) glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Starting LB proxy service");
	if (mode & SERVICE_SERVER) glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Starting LB server service");

	// XXX: workaround for only preudoparallel job registration
	//	we need at least 2 slaves to avoid locking misbehaviour
	if ((mode == SERVICE_PROXY_SERVER) && (slaves == 1)) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "Running both proxy and server services enforces at least 2 slaves");
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "Starting 2 slaves");
		slaves = 2;
	}

	fpid = fopen(pidfile,"r");
	if ( fpid )
	{
		int	opid = -1;

		if ( fscanf(fpid,"%d",&opid) == 1 )
		{
			if ( !kill(opid,0) )
			{
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "%s: another instance running, pid = %d", argv[0], opid);
				return 1;
			}
			else if (errno != ESRCH) { perror("kill()"); return 1; }
		}
		fclose(fpid);
	} else if (errno != ENOENT) { perror(pidfile); return 1; }

	fpid = fopen(pidfile, "w");
	if (!fpid) { perror(pidfile); return 1; }
	if (fprintf(fpid, "%d", getpid()) <= 0) { perror(pidfile); return 1; }
	if (fclose(fpid) != 0) { perror(pidfile); return 1; }

	umask(S_IRWXG | S_IRWXO);

	if (policy_file && parse_server_policy(ctx, policy_file, &authz_policy)) {
		char *et, *ed;

		edg_wll_Error(ctx,&et,&ed);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Cannot load server policy: %s: %s\n", et, ed);
		return 1;
	}

	if (gridmap && parse_gridmap(ctx, gridmap, &id_mapping)) {
		char *et, *ed;

		edg_wll_Error(ctx,&et,&ed);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Cannot load identity mapping: %s: %s\n", et, ed);
		return 1;
	}

	if (!html_header) {
		char *html_header_prefix = getenv("GLITE_LB_LOCATION_ETC");
		if (!html_header_prefix) html_header_prefix="/etc";
		asprintf(&html_header, "%s/glite-lb/html-header.html", html_header_prefix);
	}
	if (html_header) {
		FILE *fp = fopen(html_header, "r");
		if( fp ) {
			fclose(fp);
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "Using HTML header file %s", html_header);
		} else {
			glite_common_log(LOG_CATEGORY_CONTROL, html_header_forced ? LOG_PRIORITY_ERROR : LOG_PRIORITY_INFO, "Cannot open HTML header file %s", html_header);
			free(html_header), html_header = NULL;
		}
	}

	if (msg_conf) {
		int retv_msg;
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "Parsing MSG conf file: %s", msg_conf);
 		retv_msg = edg_wll_ParseMSGConf(msg_conf, &msg_brokers, &msg_prefixes); 
 		if (retv_msg) {
			switch(retv_msg) {
				case -1: glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "Error opening MSG conf file: %s", msg_conf); break;
				case -2: glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "Error parsing MSG conf file: %s", msg_conf); break;
			}
		}
	}
	if (!msg_prefixes) { // Prefixes not extracted from file, put in defaults
		msg_prefixes = (char**) calloc(sizeof(char**), 2);
		asprintf(&(msg_prefixes[0]), "grid.emi.");
	}

	if (enable_lcas) {
		char s[3];

		switch (glite_common_log_get_priority(LOG_CATEGORY_LB_AUTHZ)) {
		case LOG_PRIORITY_FATAL:
		case LOG_PRIORITY_ERROR:
		case LOG_PRIORITY_WARN:
			i = 0;
			break;
		case LOG_PRIORITY_INFO:
			i = 1;
			break;
		case LOG_PRIORITY_DEBUG:
			i = 2;
			break;
		default:
			i = 0;
		}
		snprintf(s, 3, "%d", i);
		setenv("LCAS_DEBUG_LEVEL", s, 1);
	}

	if (mode & SERVICE_SERVER) {
		if (check_mkdir(dumpStorage)){
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Directory for dump files not ready!");
			exit(1);
		}
		if (check_mkdir(purgeStorage)){
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "Directory for purge files not ready!");
			exit(1);
		}
		if ( jpreg ) {
			if ( glite_lbu_MaildirInit(jpregDir) ) {
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "[%d] glite_lbu_MaildirInit failed: %s", getpid(), lbm_errdesc);
				exit(1);
			}
		}
	}

	if (mode & SERVICE_SERVER) {
		if ( fake_host )
		{
			char	*p = strrchr(fake_host,':');

			if (p)
			{
				*p = 0;
				fake_port = atoi(p+1);
			}
			else fake_port = atoi(port);
		}
		else {
			fake_host = strdup(host);
			fake_port = atoi(port); 
		}

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Server address: %s:%d", fake_host, fake_port);
	}

	if ((mode & SERVICE_SERVER)) {
		if (glite_srvbones_daemon_listen(NULL, port, &service_table[SRV_SERVE].conn) != 0) {
			return 1;
		}

		asprintf(&portstr, "%d", atoi(port)+1);
		if (glite_srvbones_daemon_listen(NULL, portstr, &service_table[SRV_STORE].conn) != 0) {
			free(portstr);
			return 1;
		}
		free(portstr); portstr = NULL;

#ifdef GLITE_LB_SERVER_WITH_WS
		if (glite_srvbones_daemon_listen(NULL, ws_port, &service_table[SRV_WS].conn) != 0)
			return 1;
#endif	/* GLITE_LB_SERVER_WITH_WS */

		if (!server_cert || !server_key)
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "%s: key or certificate file not specified  - unable to watch them for changes!", argv[0]);

		if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
		int ret;
		ret = edg_wll_gss_watch_creds(server_cert, &cert_mtime);
		if (ret < 0)
        		glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"edg_wll_gss_watch_creds failed, unable to access credentials\n");
		if ( !edg_wll_gss_acquire_cred(server_cert, server_key, GSS_C_ACCEPT, &mycred, &gss_code) && mycred->name != NULL)
		{
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Server identity: %s", mycred->name);
			server_subject = strdup(mycred->name);
			add_root(ctx, server_subject, READ_ALL);
			add_root(ctx, server_subject, PURGE);
		}
		else {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Server running unauthenticated");
			server_subject = strdup("anonymous LB");
		}
		if ( noAuth ) 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Server in promiscuous mode");
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, 
			"Server listening at %d,%d (accepting protocols: " COMP_PROTO " and compatible) ...", atoi(port), atoi(port)+1);

#ifdef GLITE_LB_SERVER_WITH_WS
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
		"Server listening at %d (accepting web service protocol) ...", atoi(ws_port));
#endif	/* GLITE_LB_SERVER_WITH_WS */

	}
#ifdef GLITE_LB_SERVER_WITH_WS
	free(ws_port);
	ws_port = NULL;
#endif
	if (mode & SERVICE_PROXY) {	/* proxy stuff */
		struct sockaddr_un      a;

		service_table[SRV_SERVE_PROXY].conn = socket(PF_UNIX, SOCK_STREAM, 0);
		if ( service_table[SRV_SERVE_PROXY].conn < 0 ) { 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "socket()");
			return 1; 
		}
		memset(&a, 0, sizeof(a));
		a.sun_family = AF_UNIX;
		sprintf(sock_serve, "%s%s", socket_path_prefix, "serve.sock");
		strcpy(a.sun_path, sock_serve);

		if( connect(service_table[SRV_SERVE_PROXY].conn, (struct sockaddr *)&a, sizeof(a.sun_path)) < 0) {
			if( errno == ECONNREFUSED ) {
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "removing stale input socket %s", sock_serve);
				unlink(sock_serve);
			}
		} else { 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "another instance of lb-proxy is running");
			return 1; 
		}

		if ( bind(service_table[SRV_SERVE_PROXY].conn, (struct sockaddr *) &a, sizeof(a)) < 0 ) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "bind(%s)", sock_serve);
			return 1;
		}

		if ( listen(service_table[SRV_SERVE_PROXY].conn, con_queue) ) { 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "listen()");
			return 1; 
		}

		service_table[SRV_STORE_PROXY].conn = socket(PF_UNIX, SOCK_STREAM, 0);
		if ( service_table[SRV_STORE_PROXY].conn < 0 ) { 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "socket()");
			return 1; 
		}
		memset(&a, 0, sizeof(a));
		a.sun_family = AF_UNIX;
		sprintf(sock_store, "%s%s", socket_path_prefix, "store.sock");
		strcpy(a.sun_path, sock_store);

		if( connect(service_table[SRV_STORE_PROXY].conn, (struct sockaddr *)&a, sizeof(a.sun_path)) < 0) {
			if( errno == ECONNREFUSED ) {
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "removing stale input socket %s", sock_store);
				unlink(sock_store);
			}
		} else { 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "another instance of lb-proxy is running");
			return 1; 
		}

		if ( bind(service_table[SRV_STORE_PROXY].conn, (struct sockaddr *) &a, sizeof(a))) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "bind(%s)", sock_store);
			
			return 1;
		}
		if ( listen(service_table[SRV_STORE_PROXY].conn, con_queue) ) { 
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "listen()");
			return 1; 
		}

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "Proxy listening at %s, %s ...", sock_store, sock_serve);
	}

	if (!dbstring) dbstring = getenv("LBDB");
	if (!dbstring) dbstring = DEFAULTCS;
		
	/* Just check the database and let it be. The slaves do the job. */
	if (wait_for_open(ctx, dbstring)) {
		edg_wll_Close(ctx);
		edg_wll_FreeContext(ctx);
		return 1;
	}

	if ((ctx->dbcaps = glite_lbu_DBQueryCaps(ctx->dbctx)) == -1)
	{
		char	*et,*ed;
		glite_lbu_DBError(ctx->dbctx,&et,&ed);

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "%s: open database: %s (%s)", argv[0], et, ed);
		free(et); free(ed);
		return 1;
	}
	edg_wll_Close(ctx);
	ctx->dbctx = NULL;
	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "DB '%s'", dbstring ? : "default");

	if ((ctx->dbcaps & GLITE_LBU_DB_CAP_INDEX) == 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "%s: missing index support in DB layer", argv[0]);
		return 1;
	}
	if ((ctx->dbcaps & GLITE_LBU_DB_CAP_TRANSACTIONS) == 0)
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "transactions aren't supported!");
	if (transactions >= 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "transactions forced from %d to %d", ctx->dbcaps & GLITE_LBU_DB_CAP_TRANSACTIONS ? 1 : 0, transactions);
		ctx->dbcaps &= ~GLITE_LBU_DB_CAP_TRANSACTIONS;
		ctx->dbcaps |= transactions ? GLITE_LBU_DB_CAP_TRANSACTIONS : 0;
	}
	use_dbcaps = ctx->dbcaps;

	if (count_statistics) edg_wll_InitStatistics(ctx);

	if (count_server_stats) edg_wll_InitServerStatistics(ctx, stats_file_prefix);

	edg_wll_FreeContext(ctx);

	if ( !debug ) {
		if (daemon(1,0) == -1) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "deamon()");
			exit(1);
		}
#ifdef LB_PERF
		monstartup((u_long)&_start, (u_long)&etext);
#endif

		fpid = fopen(pidfile,"w");
		if (!fpid) { perror(pidfile); return 1; }
		fprintf(fpid,"%d",getpid());
		fclose(fpid);
	} else {
		setpgid(0, getpid());
	}

	glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT, slaves);
	glite_srvbones_set_param(GLITE_SBPARAM_SLAVE_OVERLOAD, SLAVE_OVERLOAD);
	glite_srvbones_set_param(GLITE_SBPARAM_SLAVE_CONNS_MAX, SLAVE_CONNS_MAX);

	to = (struct timeval){CONNECT_TIMEOUT, 0};
	if (mode & SERVICE_SERVER) {
		glite_srvbones_set_param(GLITE_SBPARAM_CONNECT_TIMEOUT, &to);
	}
	// proxy using default from srvbones, 5s

	to.tv_sec = request_timeout;
	glite_srvbones_set_param(GLITE_SBPARAM_REQUEST_TIMEOUT, &to);
	to = (struct timeval){IDLE_TIMEOUT, 0};
	glite_srvbones_set_param(GLITE_SBPARAM_IDLE_TIMEOUT, &to);
	glite_srvbones_set_param(GLITE_SBPARAM_LOG_REQ_CATEGORY, LOG_CATEGORY_LB_SERVER_REQUEST);

	switch (mode) {
		case SERVICE_PROXY:
			glite_srvbones_run(bk_clnt_data_init,service_table+SERVICE_PROXY_START,
				SERVICE_PROXY_SIZE, debug);
			break;
		case SERVICE_SERVER:
			glite_srvbones_run(bk_clnt_data_init,service_table+SERVICE_SERVER_START,
				SERVICE_SERVER_SIZE, debug);
			break;
		case SERVICE_PROXY_SERVER:
			glite_srvbones_run(bk_clnt_data_init,service_table+SERVICE_SERVER_START,
				SERVICE_PROXY_SIZE+SERVICE_SERVER_SIZE, debug);
			break;
		default:
			assert(0);
			break;
	}


	unlink(pidfile);

	for ( i = 0; i < sizofa(service_table); i++ )
		if ( service_table[i].conn >= 0 ) close(service_table[i].conn);

	if (mode & SERVICE_PROXY) {
		unlink(sock_serve);
		unlink(sock_store);
	}

        if (port) free(port);
	edg_wll_gss_release_cred(&mycred, NULL);

	return 0;
}

static void list_index_cols(edg_wll_QueryRec **index,edg_wll_IColumnRec **index_cols_out)
{
	int i,j, k, maxncol, ncol;
	edg_wll_IColumnRec	*index_cols;

	ncol = maxncol = 0;
	for ( i = 0; index[i]; i++ )
		for ( j = 0; index[i][j].attr; j++ )
			maxncol++;

	index_cols = calloc(maxncol+1, sizeof(edg_wll_IColumnRec));
	for ( i = 0; index[i]; i++ )
	{
		for ( j = 0; index[i][j].attr; j++)
		{
			for ( k = 0;
				  k < ncol && edg_wll_CmpColumn(&index_cols[k].qrec, &index[i][j]);
				  k++);

			if ( k == ncol)
			{
				index_cols[ncol].qrec = index[i][j];
				if ( index[i][j].attr == EDG_WLL_QUERY_ATTR_USERTAG )
				{
					index_cols[ncol].qrec.attr_id.tag =
							strdup(index[i][j].attr_id.tag);
				}
				index_cols[ncol].colname =
						edg_wll_QueryRecToColumn(&index_cols[ncol].qrec);
				ncol++;
			}
		}
	}
	index_cols[ncol].qrec.attr = EDG_WLL_QUERY_ATTR_UNDEF;
	index_cols[ncol].colname = NULL;
	*index_cols_out = index_cols;
}

int bk_clnt_data_init(void **data)
{
	edg_wll_Context			ctx;
	struct clnt_data_t	   *cdata;
	edg_wll_QueryRec	  **job_index,**notif_index;


	if ( !(cdata = calloc(1, sizeof(*cdata))) )
		return -1;

	cdata->mode = mode;

	if ( edg_wll_InitContext(&ctx) ) 
	{
		free(cdata);
		return -1;
	}

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] opening database ...", getpid());
	wait_for_open(ctx, dbstring);
	glite_lbu_DBSetCaps(ctx->dbctx, use_dbcaps);
	cdata->dbctx = ctx->dbctx;
	cdata->dbcaps = use_dbcaps;

	if ( edg_wll_QueryJobIndices(ctx, &job_index, NULL) )
	{
		char	   *et, *ed;

		edg_wll_Error(ctx,&et,&ed);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "[%d]: query_job_indices(): %s: %s, no custom indices available", getpid(), et, ed);
		free(et);
		free(ed);
	}
	cdata->job_index = job_index;
	if ( job_index ) list_index_cols(job_index,&cdata->job_index_cols);

	if (edg_wll_QueryNotifIndices(ctx,&notif_index,NULL)) {
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "[%d]: query notif indices: %s: %s", getpid(), et, ed);
	
		free(et); free(ed);
	}
	cdata->notif_index = notif_index;
	if (notif_index) list_index_cols(notif_index,&cdata->notif_index_cols);

	edg_wll_FreeContext(ctx);
		
#ifdef LB_PERF
	glite_wll_perftest_init(NULL, NULL, NULL, NULL, 0);
#endif

	*data = cdata;
	return 0;
}


/*
 *	Creates context (initializes it from global vatiables and data given
 *	from server_bones)
 *	gets the connection info
 *	and accepts the gss connection
 */
int bk_handle_connection(int conn, struct timeval *timeout, void *data)
{
	struct clnt_data_t *cdata = (struct clnt_data_t *)data;
	edg_wll_Context		ctx;
	edg_wll_GssPrincipal	client = NULL;
	edg_wll_GssCred		newcred = NULL;
	edg_wll_GssStatus	gss_code;
	struct timeval		dns_to = {DNS_TIMEOUT, 0},
						conn_start, now;
	struct sockaddr_storage	a;
	socklen_t					alen;
	char			*server_name = NULL,
				*port =NULL,
				*name_num = NULL,
				*name = NULL;
	int					h_errno, ret;
	int			npref, totpref, i;


#if 0
	switch ( edg_wll_gss_watch_creds(server_cert, &cert_mtime) ) {
	case 0: break;
	case 1:
		if ( !edg_wll_gss_acquire_cred(server_cert, server_key, GSS_C_ACCEPT, &newcred, &gss_code) ) {
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, "[%d] reloading credentials successful", getpid());
			edg_wll_gss_release_cred(&mycred, NULL);
			mycred = newcred;
		} else { 
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_WARN, "[%d] reloading credentials failed, using old ones", getpid());
		}
		break;
	case -1: 
		glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_ERROR, "[%d] edg_wll_gss_watch_creds failed, unable to access credentials", getpid());
		break;
	}
#else
	if ( !edg_wll_gss_acquire_cred(server_cert, server_key, GSS_C_ACCEPT, &newcred, &gss_code) ) {
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, "[%d] reloading credentials successful", getpid());
			edg_wll_gss_release_cred(&mycred, NULL);
			mycred = newcred;
		} else { 
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_WARN, "[%d] reloading credentials failed, using old ones", getpid());
		}
#endif

	if ( edg_wll_InitContext(&ctx) )
	{
		//fprintf(stderr, "Couldn't create context");
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Couldn't create context");
		return -1;
	}
	cdata->ctx = ctx;

	/* Shared structures (pointers)
	 */
	ctx->serverRunning = cdata->mode & SERVICE_SERVER;
	ctx->proxyRunning = cdata->mode & SERVICE_PROXY;
	ctx->dbctx = cdata->dbctx;
	ctx->dbcaps = cdata->dbcaps;
	ctx->job_index_cols = cdata->job_index_cols;
	ctx->job_index = cdata->job_index; 
	ctx->notif_index_cols = cdata->notif_index_cols;
	ctx->notif_index = cdata->notif_index; 
	
	/*	set globals
	 */
	ctx->notifDuration = notif_duration;
	ctx->notifDurationMax = notif_duration_max;
	ctx->purgeStorage = strdup(purgeStorage);
	ctx->dumpStorage = strdup(dumpStorage);
	if ( jpreg ) ctx->jpreg_dir = strdup(jpregDir); else ctx->jpreg_dir = NULL;
	ctx->hardJobsLimit = hardJobsLimit;
	ctx->hardEventsLimit = hardEventsLimit;
	if ( noAuth ) ctx->noAuth = 1;
	if ( authz_policy.actions_num ) {
		int i,j;
		for (i=0; i < authz_policy.actions_num; i++)
			for (j = 0; j < authz_policy.actions[i].rules_num; j++)
				edg_wll_add_authz_rule(ctx,
					&ctx->authz_policy,
					authz_policy.actions[i].id,
					&authz_policy.actions[i].rules[j]);
	}
	ctx->rgma_export = rgma_export;
	memcpy(ctx->purge_timeout, purge_timeout, sizeof(ctx->purge_timeout));

	ctx->p_tmp_timeout.tv_sec = timeout->tv_sec;
	ctx->p_tmp_timeout.tv_usec = timeout->tv_usec;
	
	edg_wll_initConnections();

	ctx->count_statistics = count_statistics;

	ctx->count_server_stats = count_server_stats;

	ctx->serverIdentity = strdup(server_subject);

	ctx->rssTime = rss_time;

	if (policy_file) ctx->authz_policy_file = strdup(policy_file);
	if (html_header) ctx->html_header_file = strdup(html_header);
	else ctx->html_header_file = NULL;

	ctx->id_mapping.num = id_mapping.num;
	if (id_mapping.num) {
		ctx->id_mapping.rules = (_edg_wll_mapping_rule*)malloc(ctx->id_mapping.num * sizeof(_edg_wll_mapping_rule));
		for ( i = 0; i < ctx->id_mapping.num; i++ ) {
			ctx->id_mapping.rules[i].a = strdup(id_mapping.rules[i].a);
			ctx->id_mapping.rules[i].b = strdup(id_mapping.rules[i].b);
		}
		ctx->id_mapping.mapfile = id_mapping.mapfile;
	}
	else ctx->id_mapping.mapfile = NULL;

	gettimeofday(&conn_start, 0);

	alen = sizeof(a);
	memset(&a, 0, sizeof a);
	if (getpeername(conn, (struct sockaddr *)&a, &alen) != 0) {
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_ERROR, "getpeername(): %s", strerror(errno));
		return -1;
	}
	if ((h_errno = asyn_gethostbyaddr(&name_num, &port, (struct sockaddr *)&a, alen, &dns_to, 1)) != 0) {
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_ERROR, "gethostbyaddr(): %s", hstrerror(h_errno));
		return -1;
	}
	ctx->connections->serverConnection->peerPort = atoi(port);
	h_errno = asyn_gethostbyaddr(&name, NULL, (struct sockaddr *)&a, alen, &dns_to, 0);
	switch ( h_errno )
	{
	case NETDB_SUCCESS:
		if (name) 
			glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_INFO, "[%d] connection from %s:%s (%s)", getpid(), name_num, port, name); 
		free(ctx->connections->serverConnection->peerName);
		ctx->connections->serverConnection->peerName = name;
		name = NULL;
		break;

	default:
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_DEBUG, "gethostbyaddr(%s): %s", name_num, hstrerror(h_errno));
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_INFO,"[%d] connection from %s:%s", getpid(), name_num, port);
		free(ctx->connections->serverConnection->peerName);
		ctx->connections->serverConnection->peerName = strdup(name_num);
		break;
	}
	
	free(port);

	gettimeofday(&now, 0);
	if ( decrement_timeout(timeout, conn_start, now) )
	{
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_WARN, "gethostbyaddr() timeout"); 
		free(name);

		return -1;
	}
				
	if (fake_host)
	{
		ctx->srvName = strdup(fake_host);
		ctx->srvPort = fake_port;
	}
	else
	{
		alen = sizeof(a);
		getsockname(conn,(struct sockaddr *) &a,&alen);
	
		dns_to.tv_sec = DNS_TIMEOUT;
		dns_to.tv_usec = 0;
		h_errno = asyn_gethostbyaddr(&name, &port,
						(struct sockaddr *) &a, alen,
						&dns_to, 0);

		switch ( h_errno )
		{
		case NETDB_SUCCESS:
			ctx->srvName = name;
			if ( server_name != NULL )
			{
				if ( strcmp(name, server_name))
				{
					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "different server endpoint names (%s,%s), check DNS PTR records", name, server_name);
				}
			}
			else server_name = strdup(name);
			break;

		default:
				glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_ERROR, "gethostbyaddr(%s): %s", name_num, hstrerror(h_errno));
			if ( server_name != NULL )
				ctx->srvName = strdup(server_name);
			break;
		}
		ctx->srvPort = atoi(port);
		free(port); port = NULL;
	}

/* XXX: ugly workaround, we may detect false expired certificated
 * probably due to bug in Globus GSS/SSL. Treated as fatal,
 * restarting the server solves the problem */ 
 
#define _EXPIRED_CERTIFICATE_MESSAGE "certificate has expired"

	if ( (ret = edg_wll_gss_accept(mycred, conn, timeout, &ctx->connections->serverConnection->gss, &gss_code)) )
	{
		if ( ret == EDG_WLL_GSS_ERROR_TIMEOUT )
		{
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_WARN, "[%d] %s: Client authentication failed - timeout reached, closing.", getpid(),ctx->connections->serverConnection->peerName);
		}
		else if (ret == EDG_WLL_GSS_ERROR_GSS) {
			edg_wll_SetErrorGss(ctx,"Client authentication",&gss_code);
			if (strstr(ctx->errDesc,_EXPIRED_CERTIFICATE_MESSAGE)) {
				glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_WARN, "[%d] %s: false expired certificate: %s",getpid(),ctx->connections->serverConnection->peerName,ctx->errDesc);
				edg_wll_FreeContext(ctx);
				return -1;
			}
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_WARN, "[%d] %s: GSS error: %s", getpid(), ctx->connections->serverConnection->peerName, ctx->errDesc);
		}
		else
		{
			 glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_WARN, "[%d] %s: Client authentication failed", getpid(), ctx->connections->serverConnection->peerName);
		}
		edg_wll_FreeContext(ctx);
		return 1;
	} 

	ret = edg_wll_gss_get_client_conn(&ctx->connections->serverConnection->gss, &client, NULL);
	if (ret || client->flags & EDG_WLL_GSS_FLAG_ANON) {
		glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, "[%d] anonymous client",getpid());
		ctx->peerName = NULL;
	} else {
		if (ctx->peerName) free(ctx->peerName);
		ctx->peerName = strdup(client->name);
		edg_wll_gss_free_princ(client);
		glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, "[%d] client DN: %s",getpid(),ctx->peerName);
	}

#if 0
	if ( edg_wll_SetVomsGroups(ctx, &ctx->connections->serverConnection->gss, server_cert, server_key, vomsdir, cadir) )
	{
		char *errt, *errd;

		edg_wll_Error(ctx, &errt, &errd);
		glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, "[%d] %s (%s)\n[%d]\tignored, continuing without VOMS", getpid(), errt, errd,getpid());
		free(errt); free(errd);
		edg_wll_ResetError(ctx); 
	}
#endif

	if (ctx->vomsGroups.len > 0)
	{
		int i;
  
		 glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_DEBUG, "[%d] client's VOMS groups:",getpid());
		for ( i = 0; i < ctx->vomsGroups.len; i++ )
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_DEBUG, "\t %s:%s", ctx->vomsGroups.val[i].vo, ctx->vomsGroups.val[i].name);
	}
	if (ctx->fqans && *(ctx->fqans))
	{
		char **f;

		glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_DEBUG, "[%d] client's FQANs:",getpid());
		for (f = ctx->fqans; f && *f; f++)
			glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_DEBUG, "\t%s", *f);
	}
	
	/* used also to reset start_time after edg_wll_ssl_accept! */
	/* gettimeofday(&start_time,0); */
	
	ctx->noAuth = noAuth || edg_wll_amIroot(ctx->peerName, ctx->fqans,&ctx->authz_policy);
	switch ( noIndex )
	{
	case 0: ctx->noIndex = 0; break;
	case 1: ctx->noIndex = edg_wll_amIroot(ctx->peerName, ctx->fqans,&ctx->authz_policy); break;
	case 2: ctx->noIndex = 1; break;
	}
	ctx->strict_locking = strict_locking;
	ctx->greyjobs = greyjobs;
	ctx->async_registrations = async_registrations;
	ctx->exclusive_zombies = exclusive_zombies;

	for (totpref = 0; msg_prefixes[totpref]; totpref++);
	ctx->msg_prefixes = (char**) calloc(sizeof(char*), totpref+1);
	for (npref = 0; npref<totpref; npref++) 
		ctx->msg_prefixes[npref]=strdup(msg_prefixes[npref]);

	for (totpref = 0; msg_brokers && msg_brokers[totpref]; totpref++);
	ctx->msg_brokers = (char**) calloc(sizeof(char*), totpref+1);
	for (npref = 0; npref<totpref; npref++) 
		ctx->msg_brokers[npref]=strdup(msg_brokers[npref]);

	return 0;
}

#ifdef GLITE_LB_SERVER_WITH_WS
int bk_init_ws_connection(struct clnt_data_t *cdata)
{
	struct soap             *soap = NULL;
	glite_gsplugin_Context  gsplugin_ctx = NULL;
	int err = 0;

	if ( glite_gsplugin_init_context(&gsplugin_ctx) ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
			"Couldn't create gSOAP plugin context");
                return -1;
        }

        if ( !(soap = soap_new()) ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"Couldn't create soap environment");
                goto err;
        }

        soap_init2(soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
	if ( soap_set_namespaces(soap, namespaces) ) { 
                soap_done(soap);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"Couldn't set soap namespaces");
                goto err;
        }
	if ( soap_register_plugin_arg(soap, glite_gsplugin, gsplugin_ctx) ) {
                soap_done(soap);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"Couldn't set soap namespaces");
                goto err;
        }

	glite_gsplugin_use_credential(gsplugin_ctx, mycred);
        cdata->soap = soap;

	return 0;
err:
	if ( gsplugin_ctx ) glite_gsplugin_free_context(gsplugin_ctx);
        if ( soap ) soap_destroy(soap);

	return err;
}

int bk_handle_ws_connection(int conn, struct timeval *timeout, void *data)
{
    struct clnt_data_t	   *cdata = (struct clnt_data_t *) data;
	glite_gsplugin_Context	gsplugin_ctx = NULL;
	int			rv = 0;
	int 			err = 0;

	if ((err = bk_init_ws_connection(cdata)))
		return -1;

	if ( (rv = bk_handle_connection(conn, timeout, data)) ){
		soap_done(cdata->soap);
		goto err;
	}

	gsplugin_ctx = glite_gsplugin_get_context(cdata->soap);
	glite_gsplugin_set_connection(gsplugin_ctx, &cdata->ctx->connections->serverConnection->gss);

	return 0;

err:
	if ( cdata->soap ) soap_destroy(cdata->soap);

	return rv ? : -1;
}
#endif	/* GLITE_LB_SERVER_WITH_WS */

int bk_handle_connection_proxy(int conn, struct timeval *timeout, void *data)
{
	struct clnt_data_t *cdata = (struct clnt_data_t *)data;
	edg_wll_Context		ctx;
	struct timeval		conn_start, now;

        if ( edg_wll_InitContext(&ctx) ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"Couldn't create context");
		return -1;
	}
	cdata->ctx = ctx;

	/* Shared structures (pointers)
	 */
	ctx->serverRunning = cdata->mode & SERVICE_SERVER;
	ctx->proxyRunning = cdata->mode & SERVICE_PROXY;
	ctx->dbctx = cdata->dbctx;
	ctx->dbcaps = cdata->dbcaps;
	
	/*	set globals
	 */
	ctx->notifDuration = notif_duration;
	ctx->notifDurationMax = notif_duration_max;
	if ( jpreg ) ctx->jpreg_dir = strdup(jpregDir); else ctx->jpreg_dir = NULL;
	ctx->allowAnonymous = 1;
	ctx->isProxy = 1;
	ctx->noAuth = 1;
	ctx->noIndex = 1;

	if ( authz_policy.actions_num ) {
		int i,j;
		for (i=0; i < authz_policy.actions_num; i++)
			for (j = 0; j < authz_policy.actions[i].rules_num; j++)
				edg_wll_add_authz_rule(ctx,
					&ctx->authz_policy,
					authz_policy.actions[i].id,
					&authz_policy.actions[i].rules[j]);
	}

	memcpy(ctx->purge_timeout, purge_timeout, sizeof(ctx->purge_timeout));

	/* required to match superuser-authorized notifications */

	if (fake_host)
	{
		ctx->srvName = strdup(fake_host);
		ctx->srvPort = fake_port;
	}
	else {
		ctx->srvName = strdup(host);
		ctx->srvPort = atoi(port);
	}
	
	ctx->connProxy = (edg_wll_ConnProxy *) calloc(1, sizeof(edg_wll_ConnProxy));
	if ( !ctx->connProxy ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"calloc");
		edg_wll_FreeContext(ctx);

		return -1;
	}

	gettimeofday(&conn_start, 0);
	if ( edg_wll_plain_accept(conn, &ctx->connProxy->conn) ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"accept");
		edg_wll_FreeContext(ctx);

		return -1;
	} 

	gettimeofday(&now, 0);
	if ( decrement_timeout(timeout, conn_start, now) ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN,
			"edg_wll_plain_accept(): timeout");
		return -1;
	}

	ctx->rssTime = rss_time;

	return 0;
}


static int handle_server_error(edg_wll_Context ctx)
{ 
	char    *errt = NULL, *errd = NULL;
	int		err,ret = 0;

	
	switch ( (err = edg_wll_Error(ctx, &errt, &errd)) )
	{
	case ETIMEDOUT:
	case EDG_WLL_ERROR_GSS:
	case EPIPE:
	case EIO:
	case EDG_WLL_IL_PROTO:
	case E2BIG:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN,
			"[%d] %s (%s)", getpid(), errt, errd);
		/*	fallthrough
		 */
	case ENOTCONN:
	case ECONNREFUSED:
		/*
		 *	"recoverable" error - return (>0)
		 */
		ret = err;
		break;

	case ENOENT:
	case EPERM:
	case EEXIST:
	case EDG_WLL_ERROR_NOINDEX:
	case EIDRM:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN,
			"[%d] %s (%s)", getpid(), errt, errd);
		break;
	case EINVAL:
	case EDG_WLL_ERROR_PARSE_BROKEN_ULM:
	case EDG_WLL_ERROR_PARSE_EVENT_UNDEF:
	case EDG_WLL_ERROR_PARSE_MSG_INCOMPLETE:
	case EDG_WLL_ERROR_PARSE_KEY_DUPLICITY:
	case EDG_WLL_ERROR_PARSE_KEY_MISUSE:
	case EDG_WLL_ERROR_PARSE_OK_WITH_EXTRA_FIELDS:
	case EDG_WLL_ERROR_JOBID_FORMAT:
	case EDG_WLL_ERROR_MD5_CLASH:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN,
			"[%d] %s (%s)", getpid(), errt, errd);
		/*
		 *	no action for non-fatal errors
		 */
		break;

	case EDG_WLL_ERROR_ACCEPTED_OK:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
			"[%d] %s (%s)", getpid(), errt, errd);
		/*
		 * 	all OK, but slave needs to be restarted
		 */
		ret = -EINPROGRESS;
		break;

	case EDG_WLL_ERROR_DB_INIT:
	case EDG_WLL_ERROR_DB_CALL:
	case EDG_WLL_ERROR_SERVER_RESPONSE:
	default:
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL,
		"[%d] %s (%s)", getpid(), errt, errd);
		/*
		 *	unknown error - do rather return (<0) (slave will be killed)
		 */
		ret = -EIO; 
	} 
	free(errt); free(errd);
	return ret;
}

int bk_accept_store(int conn, struct timeval *timeout, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;
	struct timeval		before, after;
	int	err;

	/*
	 *	serve the request
	 */
	memcpy(&ctx->p_tmp_timeout, timeout, sizeof(ctx->p_tmp_timeout));
	gettimeofday(&before, NULL);
	if ( edg_wll_StoreProto(ctx) && (err = handle_server_error(ctx))) return err;

	gettimeofday(&after, NULL);
	if ( decrement_timeout(timeout, before, after) ) {
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_WARN,
			"Serving store connection timed out");
		return ETIMEDOUT;
	}

	return 0;
}

static int
try_accept_ws(int conn, struct timeval *timeout, void *cdata, char *req, size_t len)
{
	edg_wll_Context ctx = ((struct clnt_data_t *) cdata)->ctx;
	glite_gsplugin_Context  gsplugin_ctx = NULL;
	struct soap *soap = NULL;
	int err;

	err = bk_init_ws_connection(cdata);
	if (err)
		return err;
	soap = ((struct clnt_data_t *) cdata)->soap;
	gsplugin_ctx = glite_gsplugin_get_context(soap);
	err = edg_wll_gss_unread(&ctx->connections->serverConnection->gss, req, len);
	if (err)
		goto end;
	glite_gsplugin_set_connection(gsplugin_ctx, &ctx->connections->serverConnection->gss);
	bk_accept_ws(conn, timeout, cdata);

	err = 0;
end:
	soap_destroy(soap);
	glite_gsplugin_free_context(gsplugin_ctx);

	return err;
}



int bk_accept_serve(int conn, struct timeval *timeout, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;
	struct timeval		before, after;
	int	err;
	char    *body = NULL, *resp = NULL, **hdrOut = NULL, *bodyOut = NULL;
	int 	httpErr;

	/*
	 *	serve the request
	 */
	memcpy(&ctx->p_tmp_timeout, timeout, sizeof(ctx->p_tmp_timeout));
	gettimeofday(&before, NULL);
	err = edg_wll_AcceptHTTP(ctx, &body, &resp, &hdrOut, &bodyOut, &httpErr);
	if (err && (err = handle_server_error(ctx))){
		switch (err) {
			case EIO:
				glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_INFO, "I/O error on answer to client '%s'", ctx->connections->serverConnection->peerName);
				break;
			case ECONNREFUSED:
				glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_INFO, "Connection refused on answer to client '%s'", ctx->connections->serverConnection->peerName);
				break;
			case ENOTCONN:
				glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_INFO, "Client '%s' closed connection", ctx->connections->serverConnection->peerName);
				break;
			default:
				edg_wll_DoneHTTP(ctx, resp, hdrOut, bodyOut);
		}
	        free(resp);
	        free(bodyOut);
                if (body)
			free(body);
	        // hdrOut are static
		return err;
	}

	// additional actions (notification stream)
	if (ctx->processRequest_cb) {
		ctx->processRequest_cb(ctx);
		ctx->processRequest_cb = NULL;
	}

	if (httpErr == HTTP_BADREQ && body)
		err = try_accept_ws(conn, timeout, cdata, body, strlen(body) + 1);
	if (httpErr != HTTP_BADREQ || err)
		edg_wll_DoneHTTP(ctx, resp, hdrOut, bodyOut);

	free(resp);
	free(bodyOut);
	if (body)
		free(body);
	// hdrOut are static

	gettimeofday(&after, NULL);
	if ( decrement_timeout(timeout, before, after) ) {
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_WARN,
			"Serving store connection timed out");
		
		return ETIMEDOUT;
	}

	return 0;
}


#ifdef GLITE_LB_SERVER_WITH_WS
int bk_accept_ws(int conn, struct timeval *timeout, void *cdata)
{
	struct soap			   *soap = ((struct clnt_data_t *) cdata)->soap;
	edg_wll_Context			ctx = ((struct clnt_data_t *) cdata)->ctx;
	glite_gsplugin_Context	gsplugin_ctx;
	int						err;


	gsplugin_ctx = glite_gsplugin_get_context(soap);
	glite_gsplugin_set_timeout(gsplugin_ctx, timeout);
	glite_gsplugin_set_udata(soap, ctx);
	/*	soap->max_keep_alive must be higher tha 0,
	 *	because on 0 value soap closes the connection
	 */
	soap->max_keep_alive = 10;
	soap->keep_alive = 1;
	soap_begin(soap);
	err = 0;
	if ( soap_begin_recv(soap) ) {
		if ( soap->error == SOAP_EOF ) {
			soap_send_fault(soap);
			return ENOTCONN;
		}
		if ( soap->error < SOAP_STOP ) err = soap_send_fault(soap);
		else soap_closesock(soap);	/*	XXX: Do close the socket here? */
	} else {
		/* XXX: An ugly hack!
		 * soap->keep_alive is reset to 0 by soap->fparse (http_parse)
		 * handler
		 * Disabling http_parse function would be nice :)
		 */
		soap->keep_alive = 1;
		if (   soap_envelope_begin_in(soap)
			   || soap_recv_header(soap)
			   || soap_body_begin_in(soap)
			   || soap_serve_request(soap)
#if GSOAP_VERSION >= 20700
			   /* XXX: Is it really neccesary ? */
			   || (soap->fserveloop && soap->fserveloop(soap))
#endif
			   )
		err = soap_send_fault(soap);
	}

	if ( err ) {
		// soap_print_fault(struct soap *soap, FILE *fd) maybe useful here
		glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_WARN,
			"[%d] SOAP error (bk_accept_ws)", getpid());
		return ECANCELED;
	}

	// additional actions (notification stream)
	if (ctx->processRequest_cb) {
		ctx->processRequest_cb(ctx);
		ctx->processRequest_cb = NULL;
	}

	edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_WS_QUERIES);

	return ENOTCONN;
}
#endif	/* GLITE_LB_SERVER_WITH_WS */


int bk_clnt_disconnect(int conn, struct timeval *timeout, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;


	if ( ctx->connections->serverConnection->gss.context != NULL)
		edg_wll_gss_close(&ctx->connections->serverConnection->gss, timeout);
	edg_wll_FreeContext(ctx);
	ctx = NULL;

	return 0;
}

#ifdef GLITE_LB_SERVER_WITH_WS
int bk_ws_clnt_disconnect(int conn, struct timeval *timeout, void *cdata)
{
	struct soap			   *soap = ((struct clnt_data_t *) cdata)->soap;
	glite_gsplugin_Context	gsplugin_ctx;
	int						rv;


	gsplugin_ctx = glite_gsplugin_get_context(soap);
	glite_gsplugin_set_connection(gsplugin_ctx, NULL);
	glite_gsplugin_use_credential(gsplugin_ctx, NULL);
	if ( (rv = bk_clnt_disconnect(conn, timeout, cdata)) )
		return rv;

	soap_destroy(((struct clnt_data_t *)cdata)->soap);
	glite_gsplugin_free_context(gsplugin_ctx);

	return 0;
}
#endif	/* GLITE_LB_SERVER_WITH_WS */


int bk_clnt_disconnect_proxy(int conn, struct timeval *timeout, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;

	/* XXX: handle the timeout
	 */
    if ( ctx->connProxy && ctx->connProxy->conn.sock >= 0 )
		edg_wll_plain_close(&ctx->connProxy->conn);

	edg_wll_FreeContext(ctx);
	ctx = NULL;

	return 0;
}

int bk_clnt_reject(int conn)
{
	int		flags = fcntl(conn, F_GETFL, 0);

	if ( fcntl(conn, F_SETFL, flags | O_NONBLOCK) < 0 ) 
		return 1;

	edg_wll_gss_reject(conn);

	return 0;
}

#ifdef GLITE_LB_SERVER_WITH_WS
int bk_ws_clnt_reject(int conn)
{
	return bk_clnt_reject(conn);
}
#endif	/* GLITE_LB_SERVER_WITH_WS */

int bk_clnt_reject_proxy(int conn)
{
	return 0;
}


static int wait_for_open(edg_wll_Context ctx, const char *dbstring)
{
	char	*dbfail_string1, *dbfail_string2;
	char	*errt,*errd;
	int err;

	dbfail_string1 = dbfail_string2 = NULL;

	while (((err = edg_wll_Open(ctx, (char *) dbstring)) != EDG_WLL_ERROR_DB_INIT) && err) {
		if (dbfail_string1) free(dbfail_string1);
		edg_wll_Error(ctx,&errt,&errd);
		asprintf(&dbfail_string1,"%s (%s)",errt,errd);
		free(errt);
		free(errd);
		if (dbfail_string1 != NULL) {
			if (dbfail_string2 == NULL || strcmp(dbfail_string1,dbfail_string2)) {
				if (dbfail_string2) free(dbfail_string2);
				dbfail_string2 = dbfail_string1;
				dbfail_string1 = NULL;
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[%d]: %s\nStill trying ...",getpid(),dbfail_string2);
				
			}
		}
		sleep(5);
	}

	if (dbfail_string1) free(dbfail_string1);
	if (dbfail_string2 != NULL) {
		free(dbfail_string2);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
			"[%d]: DB connection established",getpid());
	}

	if (err) {
		edg_wll_Error(ctx,&errt,&errd);
		asprintf(&dbfail_string1,"%s (%s)",errt,errd);
		free(errt);
		free(errd);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL,
			"[%d]: %s", getpid(), dbfail_string1);
		free(dbfail_string1);
	}

	return err;
}

struct asyn_result {
	char		*host;
	char		*service;
	int		err;
};

/* ares callback handler for ares_getnameinfo() */
#if ARES_VERSION >= 0x010500
void callback_ares_getnameinfo(void *arg, int status, int timeouts, char *node, char *service)
#else
void callback_ares_getnameinfo(void *arg, int status, char *node, char *service)
#endif
{
	struct asyn_result *arp = (struct asyn_result *) arg;

	switch (status) {
	   case ARES_SUCCESS:
		if (node||service) {
			if (node) arp->host = strdup(node);		
			if (service) arp->service = strdup(service);		
			if (arp->host == NULL && arp->service == NULL) {
				arp->err = NETDB_INTERNAL;
			} else {
				arp->err = NETDB_SUCCESS;
			}
		} else {
			arp->err = NO_DATA;
		}
		break;
	    case ARES_ENODATA:
	    case ARES_EBADNAME:
	    case ARES_ENOTFOUND:
		arp->err = HOST_NOT_FOUND;
		break;
	    case ARES_ENOTIMP:
		arp->err = NO_RECOVERY;
		break;
	    case ARES_ENOMEM:
	    case ARES_EDESTRUCTION:
	    default:
		arp->err = NETDB_INTERNAL;
		break;
	}
}

static int asyn_gethostbyaddr(char **name, char **service, const struct sockaddr *addr, int len, struct timeval *timeout, int numeric)
{
	struct asyn_result ar;
	ares_channel channel;
	int nfds;
	fd_set readers, writers;
	struct timeval tv, *tvp;
	struct timeval start_time,check_time;
	int 	flags = 0;
	int	err = NETDB_INTERNAL;
	struct sockaddr_in	v4;

	if (addr->sa_family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)addr)->sin6_addr)) {
		v4.sin_family = AF_INET;
		v4.sin_port = ((struct sockaddr_in6 *)addr)->sin6_port;
		v4.sin_addr.s_addr = *(in_addr_t *) &((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr[12];
		addr = (struct sockaddr *) &v4;
		len = sizeof(v4);
	} 

	if (!numeric && addr->sa_family == AF_INET6) {
		/* don't bother, c-ares up to version 1.7.3 has fatal bug */
		return NETDB_INTERNAL;
	}

/* start timer */
        gettimeofday(&start_time,0);

/* ares init */
        if ( ares_init(&channel) != ARES_SUCCESS ) return(NETDB_INTERNAL);
	memset((void *) &ar, 0, sizeof(ar));

/* query DNS server asynchronously */
	if (name) flags |= ARES_NI_LOOKUPHOST | ( numeric? ARES_NI_NUMERICHOST : 0);
	if (service) flags |= ARES_NI_LOOKUPSERVICE | ( numeric? ARES_NI_NUMERICSERV : 0);
	ares_getnameinfo(channel, addr, len, flags, (ares_nameinfo_callback)callback_ares_getnameinfo, (void *) &ar);

/* wait for result */
        while (1) {
                FD_ZERO(&readers);
                FD_ZERO(&writers);
                nfds = ares_fds(channel, &readers, &writers);
                if (nfds == 0)
                        break;

                gettimeofday(&check_time,0);
		if (decrement_timeout(timeout, start_time, check_time)) {
			ares_destroy(channel);
			return(TRY_AGAIN);
		}
		start_time = check_time;

                tvp = ares_timeout(channel, timeout, &tv);

                switch ( select(nfds, &readers, &writers, NULL, tvp) ) {
			case -1: if (errno != EINTR) {
					ares_destroy(channel);
				  	return NETDB_INTERNAL;
				 } else
					continue;
			case 0: 
				FD_ZERO(&readers);
				FD_ZERO(&writers);
				/* fallthrough */
                        default : ares_process(channel, &readers, &writers);
                }

        }

	if (ar.err == NETDB_SUCCESS) {
		if (name) {
			if (numeric && addr->sa_family == AF_INET6) {
				asprintf(name,"[%s]",ar.host);
				free(ar.host);
			} else {
				*name = ar.host;
			}
		}
		if (service) *service = ar.service;
	}
	err = ar.err;

	ares_destroy(channel);
	return err;
}

static int add_root(edg_wll_Context ctx, char *root, authz_action action)
{
	struct _edg_wll_authz_attr attr;
	struct _edg_wll_authz_rule rule;

	attr.id = ATTR_SUBJECT;
	if (strncmp(root, "FQAN:", 5) == 0){
		root += 5;
		attr.id = ATTR_FQAN;
	}
	attr.value = root;
	rule.attrs = &attr;
	rule.attrs_num = 1;
	edg_wll_add_authz_rule(ctx, &authz_policy, action, &rule);

	return 0;
}

static int parse_limits(char *opt, int *j_limit, int *e_limit, int *size_limit)
{
	return (sscanf(opt, "%d:%d:%d", j_limit, e_limit, size_limit) == 3);
}


static int check_mkdir(const char *dir)
{
	struct stat	sbuf;

	if ( stat(dir, &sbuf) )
	{
		if ( errno == ENOENT )
		{
			if ( mkdir(dir, S_IRWXU) )
			{
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[%d] %s: %s", getpid(), dir, strerror(errno));
				return 1;
			}
		}
		else
		{
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[%d] %s: %s", getpid(), dir, strerror(errno));
			return 1;
		}
	}

	if (!S_ISDIR(sbuf.st_mode))
	{
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN,
			"[%d] %s: not a directory", getpid(),dir);
		return 1;
	}

	if (access(dir, R_OK | W_OK))
	{
		 glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN,
			"[%d] %s: directory is not readable/writable", getpid(),dir);
		return 1;
	}
		

	return 0;
}

static int decrement_timeout(struct timeval *timeout, struct timeval before, struct timeval after)
{
        (*timeout).tv_sec = (*timeout).tv_sec - (after.tv_sec - before.tv_sec);
        (*timeout).tv_usec = (*timeout).tv_usec - (after.tv_usec - before.tv_usec);
        while ( (*timeout).tv_usec < 0) {
                (*timeout).tv_sec--;
                (*timeout).tv_usec += 1000000;
        }
        if ( ((*timeout).tv_sec < 0) || (((*timeout).tv_sec == 0) && ((*timeout).tv_usec == 0)) ) return(1);
        else return(0);
}
