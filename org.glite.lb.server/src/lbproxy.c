#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/limits.h>
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
#include <syslog.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <ares.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "glite/lb/srvbones.h"
#include "glite/lb/context.h"
#include "glite/lb/context-int.h"

#include "il_lbproxy.h"
#include "lb_http.h"
#include "lbs_db.h"


extern edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs);
extern edg_wll_ErrorCode edg_wll_Close(edg_wll_Context);
extern int edg_wll_StoreProtoProxy(edg_wll_Context ctx);

#define DEFAULTCS			"lbserver/@localhost:lbserver20"
/*
#define DEFAULTCS			"lbserver/@localhost:lbproxy"
*/

#define CON_QUEUE			20	/* accept() */
#define SLAVE_OVERLOAD		10	/* queue items per slave */
#define CLNT_TIMEOUT		10	/* keep idle connection that many seconds */
#define TOTAL_CLNT_TIMEOUT	60	/* one client may ask one slave multiple times */
					/* but only limited time to avoid DoS attacks */
#define CLNT_REJECT_TIMEOUT	100000	/* time limit for client rejection in !usec! */
#define DNS_TIMEOUT      	5	/* how long wait for DNS lookup */
#define SLAVE_CONNS_MAX		500	/* commit suicide after that many connections */
#define MASTER_TIMEOUT		30 	/* maximal time of one-round of master network communication */
#define SLAVE_TIMEOUT		30 	/* maximal time of one-round of slave network communication */

/* file to store pid and generate semaphores key
 */
#ifndef GLITE_LBPROXY_PIDFILE
#define GLITE_LBPROXY_PIDFILE		"/var/run/glite-lbproxy.pid"
#endif

#ifndef GLITE_LBPROXY_SOCK_PREFIX
#define GLITE_LBPROXY_SOCK_PREFIX	"/tmp/lb_proxy_"
#endif

#ifndef dprintf
#define dprintf(x)			{ if (debug) printf x; }
#endif

#define sizofa(a)			(sizeof(a)/sizeof((a)[0]))


int						debug  = 0;
static const int		one = 1;
static char			   *dbstring = NULL;
static char				sock_store[PATH_MAX],
						sock_serve[PATH_MAX];
static int				slaves = 10,
						semaphores = -1,
						semset;


static struct option opts[] = {
	{"port",		1, NULL,	'p'},
	{"debug",		0, NULL,	'd'},
	{"mysql",		1, NULL,	'm'},
	{"slaves",		1, NULL,	's'},
	{"semaphores",		1, NULL,	'l'},
	{"pidfile",		1, NULL,	'i'},
	{"proxy-il-sock",	1, NULL,	'X'},
	{"proxy-il-fprefix",	1, NULL,	'Y'},
	{NULL,0,NULL,0}
};

static const char *get_opt_string = "p:dm:s:l:i:X:Y:";

static void usage(char *me) 
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-p, --sock\t path-name to the local socket\n"
		"\t-m, --mysql\t database connect string\n"
		"\t-d, --debug\t don't run as daemon, additional diagnostics\n"
		"\t-s, --slaves\t number of slave servers to fork\n"
		"\t-l, --semaphores number of semaphores (job locks) to use\n"
		"\t-i, --pidfile\t file to store master pid\n"
		"\t--proxy-il-sock\t socket to send events to\n"
		"\t--proxy-il-fprefix\t file prefix for events\n"
	,me);
}

static void wait_for_open(edg_wll_Context,const char *);


/*
 *	SERVER BONES structures and handlers
 */
int clnt_data_init(void **);

	/*
	 *	Serve & Store handlers
	 */
int clnt_reject(int);
int handle_conn(int, struct timeval, void *);
int accept_serve(int, void *);
int accept_store(int, void *);
int clnt_disconnect(int, void *);

#define SRV_SERVE		0
#define SRV_STORE		1
static struct glite_srvbones_service service_table[] = {
	{ "serve",	-1, handle_conn, accept_serve, clnt_reject, clnt_disconnect },
	{ "store",	-1, handle_conn, accept_store, clnt_reject, clnt_disconnect },
};

struct clnt_data_t {
	edg_wll_Context			ctx;
	void				   *mysql;
};



int main(int argc, char *argv[])
{
	int					i;
	struct sockaddr_un	a;
	int					opt;
	char				pidfile[PATH_MAX] = GLITE_LBPROXY_PIDFILE,
						socket_path_prefix[PATH_MAX] = GLITE_LBPROXY_SOCK_PREFIX,
					   *name;
	FILE			   *fpid;
	key_t				semkey;
	edg_wll_Context		ctx;
	struct timeval		to;



	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	if (geteuid()) snprintf(pidfile,sizeof pidfile,"%s/glite_lb_proxy.pid", getenv("HOME"));

	while ((opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF) switch (opt) {
		case 'p': strcpy(socket_path_prefix, optarg); break;
		case 'd': debug = 1; break;
		case 'm': dbstring = optarg; break;
		case 's': slaves = atoi(optarg); break;
		case 'l': semaphores = atoi(optarg); break;
		case 'X': lbproxy_ilog_socket_path = strdup(optarg); break;
		case 'Y': lbproxy_ilog_file_prefix = strdup(optarg); break;
		case 'i': strcpy(pidfile, optarg); break;
		case '?': usage(name); return 1;
	}

	if ( optind < argc ) { usage(name); return 1; }

	setlinebuf(stdout);
	setlinebuf(stderr);

	fpid = fopen(pidfile,"r");
	if ( fpid ) {
		int	opid = -1;

		if ( fscanf(fpid,"%d",&opid) == 1 ) {
			if ( !kill(opid,0) ) {
				fprintf(stderr,"%s: another instance running, pid = %d\n",argv[0],opid);
				return 1;
			}
			else if (errno != ESRCH) { perror("kill()"); return 1; }
		}
		fclose(fpid);
	} else if (errno != ENOENT) { perror(pidfile); return 1; }

	fpid = fopen(pidfile, "w");
	if ( !fpid ) { perror(pidfile); return 1; }
	fprintf(fpid, "%d", getpid());
	fclose(fpid);

	semkey = ftok(pidfile,0);

	if ( semaphores == -1 ) semaphores = slaves;
	semset = semget(semkey, 0, 0);
	if ( semset >= 0 ) semctl(semset, 0, IPC_RMID);
	semset = semget(semkey, semaphores, IPC_CREAT | 0600);
	if ( semset < 0 ) { perror("semget()"); return 1; }
	dprintf(("Using %d semaphores, set id %d\n", semaphores, semset));
	for ( i = 0; i < semaphores; i++ ) {
		struct sembuf	s;

		s.sem_num = i; s.sem_op = 1; s.sem_flg = 0;
		if (semop(semset,&s,1) == -1) { perror("semop()"); return 1; }
	}

	service_table[SRV_SERVE].conn = socket(PF_UNIX, SOCK_STREAM, 0);
	if ( service_table[SRV_SERVE].conn < 0 ) { perror("socket()"); return 1; }
	memset(&a, 0, sizeof(a));
	a.sun_family = AF_UNIX;
	sprintf(sock_serve, "%s%s", socket_path_prefix, "serve.sock");
	strcpy(a.sun_path, sock_serve);

	if( connect(service_table[SRV_SERVE].conn, (struct sockaddr *)&a, sizeof(a.sun_path)) < 0) {
		if( errno == ECONNREFUSED ) {
			dprintf(("removing stale input socket %s\n", sock_serve));
			unlink(sock_serve);
		}
	} else { perror("another instance of lb-proxy is running"); return 1; }

	if ( bind(service_table[SRV_SERVE].conn, (struct sockaddr *) &a, sizeof(a)) < 0 ) {
		char	buf[100];

		snprintf(buf, sizeof(buf), "bind(%s)", sock_serve);
		perror(buf);
		return 1;
	}

	if ( listen(service_table[SRV_SERVE].conn, CON_QUEUE) ) { perror("listen()"); return 1; }

	service_table[SRV_STORE].conn = socket(PF_UNIX, SOCK_STREAM, 0);
	if ( service_table[SRV_STORE].conn < 0 ) { perror("socket()"); return 1; }
	memset(&a, 0, sizeof(a));
	a.sun_family = AF_UNIX;
	sprintf(sock_store, "%s%s", socket_path_prefix, "store.sock");
	strcpy(a.sun_path, sock_store);

	if( connect(service_table[SRV_STORE].conn, (struct sockaddr *)&a, sizeof(a.sun_path)) < 0) {
		if( errno == ECONNREFUSED ) {
			dprintf(("removing stale input socket %s\n", sock_store));
			unlink(sock_store);
		}
	} else { perror("another instance of lb-proxy is running"); return 1; }

	if ( bind(service_table[SRV_STORE].conn, (struct sockaddr *) &a, sizeof(a))) {
		char	buf[100];

		snprintf(buf, sizeof(buf), "bind(%s)", sock_store);
		perror(buf);
		return 1;
	}
	if ( listen(service_table[SRV_STORE].conn, CON_QUEUE) ) { perror("listen()"); return 1; }

	dprintf(("Listening at %s, %s ...\n", sock_store, sock_serve));

	if (!dbstring) dbstring = getenv("LBPROXYDB");
	if (!dbstring) dbstring = DEFAULTCS;


	/* Just check the database and let it be. The slaves do the job. */
	/* XXX: InitContextProxy() !!!
	 * edg_wll_InitContext(&ctx) causes segfault
	 */
	if ( !(ctx = (edg_wll_Context) malloc(sizeof(*ctx))) ) {
		perror("InitContext()");
		return -1;
	}
	memset(ctx, 0, sizeof(*ctx));
	wait_for_open(ctx, dbstring);
	if (edg_wll_DBCheckVersion(ctx)) {
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);

		fprintf(stderr,"%s: open database: %s (%s)\n",argv[0],et,ed);
		return 1;
	}
	edg_wll_Close(ctx);
	edg_wll_FreeContext(ctx);

	if ( !debug ) {
		if ( daemon(1,0) == -1 ) { perror("deamon()"); exit(1); }

		fpid = fopen(pidfile,"w");
		if ( !fpid ) { perror(pidfile); return 1; }
		fprintf(fpid, "%d", getpid());
		fclose(fpid);
		openlog(name, LOG_PID, LOG_DAEMON);
	} else { setpgid(0, getpid()); }


	glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_CT, slaves);
	glite_srvbones_set_param(GLITE_SBPARAM_SLAVE_OVERLOAD, SLAVE_OVERLOAD);
	glite_srvbones_set_param(GLITE_SBPARAM_SLAVE_CONNS_MAX, SLAVE_CONNS_MAX);
	to = (struct timeval){CLNT_TIMEOUT, 0};
	glite_srvbones_set_param(GLITE_SBPARAM_CLNT_TIMEOUT, &to);
	to = (struct timeval){TOTAL_CLNT_TIMEOUT, 0};
	glite_srvbones_set_param(GLITE_SBPARAM_TOTAL_CLNT_TIMEOUT, &to);

	glite_srvbones_run(clnt_data_init, service_table, sizofa(service_table), debug);


	semctl(semset, 0, IPC_RMID, 0);
	unlink(pidfile);
	for ( i = 0; i < sizofa(service_table); i++ )
		if ( service_table[i].conn >= 0 ) close(service_table[i].conn);
	unlink(sock_serve);
	unlink(sock_store);


	return 0;
}


int clnt_data_init(void **data)
{
	edg_wll_Context			ctx;
	struct clnt_data_t	   *cdata;


	if ( !(cdata = calloc(1, sizeof(*cdata))) )
		return -1;

	if ( !(ctx = (edg_wll_Context) malloc(sizeof(*ctx))) ) { free(cdata); return -1; }
	memset(ctx, 0, sizeof(*ctx));

	dprintf(("[%d] opening database ...\n", getpid()));
	wait_for_open(ctx, dbstring);
	cdata->mysql = ctx->mysql;
	edg_wll_FreeContext(ctx);

	*data = cdata;
	return 0;
}

	
int handle_conn(int conn, struct timeval client_start, void *data)
{
	struct clnt_data_t *cdata = (struct clnt_data_t *)data;
	edg_wll_Context		ctx;
	struct timeval		total_to = { TOTAL_CLNT_TIMEOUT,0 };
	char				buf[300];


	if ( !(ctx = (edg_wll_Context) calloc(1, sizeof(*ctx))) ) {
		fprintf(stderr, "Couldn't create context");
		return -1;
	}
	cdata->ctx = ctx;

	/* Shared structures (pointers)
	 */
	ctx->mysql = cdata->mysql;
	
	/*	set globals
	 */
	ctx->allowAnonymous = 1;
	ctx->isProxy = 1;
	ctx->noAuth = 1;
	ctx->noIndex = 1;
	ctx->semset = semset;
	ctx->semaphores = semaphores;

	ctx->p_tmp_timeout.tv_sec = SLAVE_TIMEOUT;
	ctx->p_tmp_timeout.tv_usec = 0;
	if ( total_to.tv_sec < ctx->p_tmp_timeout.tv_sec ) {
		ctx->p_tmp_timeout.tv_sec = total_to.tv_sec;
		ctx->p_tmp_timeout.tv_usec = total_to.tv_usec;
	}
	
	ctx->connPlain = (edg_wll_Connection *) calloc(1, sizeof(edg_wll_Connection));
	if ( !ctx->connPlain ) {
		perror("calloc");
		edg_wll_FreeContext(ctx);

		return -1;
	}

	if ( edg_wll_plain_accept(conn, ctx->connPlain) ) {
		perror("accept");
		edg_wll_FreeContext(ctx);

		return -1;
	} 


	return 0;
}


int accept_store(int conn, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;

	if ( edg_wll_StoreProtoProxy(ctx) ) { 
		char    *errt, *errd;

		errt = errd = NULL;
		switch ( edg_wll_Error(ctx, &errt, &errd) ) {
		case ETIMEDOUT:
		case EPIPE:
			dprintf(("[%d] %s (%s)\n", getpid(), errt, errd));
			if (!debug) syslog(LOG_ERR,"%s (%s)", errt, errd);
			/*	fallthrough
			 */
		case ENOTCONN:
			edg_wll_FreeContext(ctx);
			ctx = NULL;
			free(errt); free(errd);
			dprintf(("[%d] Connection closed\n", getpid()));
			return 1;
			break;

		case ENOENT:
		case EINVAL:
		case EPERM:
		case EEXIST:
		case EDG_WLL_ERROR_NOINDEX:
		case E2BIG:
			dprintf(("[%d] %s (%s)\n", getpid(), errt, errd));
			if ( !debug ) syslog(LOG_ERR, "%s (%s)", errt, errd);
			break;
			
		default:
			dprintf(("[%d] %s (%s)\n", getpid(), errt, errd));
			if ( !debug ) syslog(LOG_CRIT, "%s (%s)", errt, errd);
			return -1;
		} 
		free(errt); free(errd);
	}

	return 0;
}

int accept_serve(int conn, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;

	/*
	 *	serve the request
	 */
	if ( edg_wll_ServerHTTP(ctx) ) { 
		char    *errt, *errd;

		
		errt = errd = NULL;
		switch ( edg_wll_Error(ctx, &errt, &errd) ) {
		case ETIMEDOUT:
		case EPIPE:
			dprintf(("[%d] %s (%s)\n", getpid(), errt, errd));
			if (!debug) syslog(LOG_ERR,"%s (%s)", errt, errd);
			/*	fallthrough
			 */
		case ENOTCONN:
			edg_wll_FreeContext(ctx);
			ctx = NULL;
			free(errt); free(errd);
			dprintf(("[%d] Connection closed\n", getpid()));
			/*
			 *	"recoverable" error - return (>0)
			 */
			return 1;
			break;

		case ENOENT:
		case EINVAL:
		case EPERM:
		case EEXIST:
		case EDG_WLL_ERROR_NOINDEX:
		case E2BIG:
			dprintf(("[%d] %s (%s)\n", getpid(), errt, errd));
			if ( !debug ) syslog(LOG_ERR,"%s (%s)", errt, errd);
			/*
			 *	no action for non-fatal errors
			 */
			break;
			
		default:
			dprintf(("[%d] %s (%s)\n", getpid(), errt, errd));
			if (!debug) syslog(LOG_CRIT,"%s (%s)",errt,errd);
			/*
			 *	unknown error - do rather return (<0) (slave will be killed)
			 */
			return -1;
		} 
		free(errt); free(errd);
	}

	return 0;
}


int clnt_disconnect(int conn, void *cdata)
{
	edg_wll_Context		ctx = ((struct clnt_data_t *) cdata)->ctx;

	edg_wll_FreeContext(ctx);

	return 0;
}

int clnt_reject(int conn)
{
	return 0;
}

static void wait_for_open(edg_wll_Context ctx, const char *dbstring)
{
	char	*dbfail_string1, *dbfail_string2;

	dbfail_string1 = dbfail_string2 = NULL;

	while (edg_wll_Open(ctx, (char *) dbstring)) {
		char	*errt,*errd;

		if (dbfail_string1) free(dbfail_string1);
		edg_wll_Error(ctx,&errt,&errd);
		asprintf(&dbfail_string1,"%s (%s)\n",errt,errd);
		if (dbfail_string1 != NULL) {
			if (dbfail_string2 == NULL || strcmp(dbfail_string1,dbfail_string2)) {
				if (dbfail_string2) free(dbfail_string2);
				dbfail_string2 = dbfail_string1;
				dbfail_string1 = NULL;
				dprintf(("[%d]: %s\nStill trying ...\n",getpid(),dbfail_string2));
				if (!debug) syslog(LOG_ERR,dbfail_string2);
			}
		}
		sleep(5);
	}

	if (dbfail_string1) free(dbfail_string1);
	if (dbfail_string2 != NULL) {
		free(dbfail_string2);
		dprintf(("[%d]: DB connection established\n",getpid()));
		if (!debug) syslog(LOG_INFO,"DB connection established\n");
	}
}
