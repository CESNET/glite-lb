#ident "$Header$"


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
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <limits.h>
#include <assert.h>
#include <syslog.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <ares.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <globus_common.h>

#include "glite/wmsutils/tls/ssl_helpers/ssl_inits.h"
#include "glite/lb/consumer.h"
#include "glite/lb/purge.h"
#include "glite/lb/context.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/context-int.h"
#include "glite/lb/lb_gss.h"

#include "lb_http.h"
#include "lb_proto.h"
#include "index.h"
#include "lbs_db.h"
#include "lb_authz.h"
#include "il_notification.h"

extern int edg_wll_StoreProto(edg_wll_Context ctx);
extern edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs);
edg_wll_ErrorCode edg_wll_Close(edg_wll_Context);

#define max(x,y) ((x)>(y)?(x):(y))

#define CON_QUEUE		20	/* accept() */
#define SLAVE_OVERLOAD		10	/* queue items per slave */
#define CLNT_TIMEOUT		10	/* keep idle connection that many seconds */
#define TOTAL_CLNT_TIMEOUT	60	/* one client may ask one slave multiple times */
					/* but only limited time to avoid DoS attacks */
#define CLNT_REJECT_TIMEOUT	100000	/* time limit for client rejection in !usec! */
#define DNS_TIMEOUT      	5	/* how long wait for DNS lookup */
#define SLAVE_CONNS_MAX		500	/* commit suicide after that many connections */
#define MASTER_TIMEOUT		30 	/* maximal time of one-round of master network communication */
#define SLAVE_TIMEOUT		30 	/* maximal time of one-round of slave network communication */
#define SLAVE_CHECK_SIGNALS	2 	/* how often to check signals while waiting for recv_mesg */
#define WATCH_TIMEOUT		1800	/* wake up to check updated credentials */

#ifndef EDG_PURGE_STORAGE
#define EDG_PURGE_STORAGE	"/tmp/purge"
#endif

#ifndef EDG_DUMP_STORAGE
#define EDG_DUMP_STORAGE	"/tmp/dump"
#endif

/* file to store pid and generate semaphores key */
#ifndef EDG_BKSERVERD_PIDFILE
#define EDG_BKSERVERD_PIDFILE	"/var/run/edg-bkserverd.pid"
#endif

#ifndef dprintf
#define dprintf(x) { if (debug) printf x; }
#endif

	static const int one = 1;

int debug  = 0;
int rgma_export = 0;
static int noAuth = 0;
static int noIndex = 0;
static int strict_locking = 0;
static int hardJobsLimit = 0;
static int hardEventsLimit = 0;
static int hardRespSizeLimit = 0;
static char	*dbstring = NULL,*fake_host = NULL;
static int fake_port = 0;

static char	** super_users = NULL;
char	*cadir = NULL, *vomsdir = NULL;


static int	slaves = 10, semaphores = -1, semset;
static char	*purgeStorage = EDG_PURGE_STORAGE;
static char	*dumpStorage = EDG_DUMP_STORAGE;
static time_t	purge_timeout[EDG_WLL_NUMBER_OF_STATCODES];
static time_t	notif_duration = 60*60*24*7;
static edg_wll_QueryRec	**job_index;
static edg_wll_IColumnRec *job_index_cols;

static volatile int die = 0, child_died = 0;

static int dispatchit(int,int,int);
static int do_sendmsg(int,int,unsigned long,int);
static int do_recvmsg(int,int *,unsigned long *,int *);

static void wait_for_open(edg_wll_Context,const char *);

static void catchsig(int sig)
{
	die = sig;
}

static void catch_chld(int sig)
{
	child_died = 1;
}

static int slave(void *,int);

static struct option opts[] = {
	{"cert",	1, NULL,	'c'},
	{"key",		1, NULL,	'k'},
	{"CAdir",	1, NULL,	'C'},
	{"VOMSdir",	1, NULL,	'V'},
	{"port",	1, NULL,	'p'},
	{"address",	1, NULL,	'a'},
	{"debug",	0, NULL,	'd'},
	{"rgmaexport",	0, NULL,	'r'},
	{"mysql",	1, NULL,	'm'},
	{"noauth",	0, NULL,	'n'},
	{"slaves",	1, NULL,	's'},
	{"semaphores",	1, NULL,	'l'},
	{"pidfile",	1, NULL,	'i'},
	{"purge-prefix",	1, NULL,	'S'},
	{"dump-prefix",	1, NULL,	'D'},
	{"super-user",	1, NULL,	'R'},
	{"super-users-file",	1, NULL,'F'},
	{"no-index",	1, NULL,	'x'},
	{"strict-locking",0, NULL,	'P'},
	{"limits",	1, NULL,	'L'},
	{"notif-dur",	1, NULL,	'N'},
	{"notif-il-sock",	1, NULL,	'X'},
	{NULL,0,NULL,0}
};

static void usage(char *me) 
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-a, --address\t use this server address (may be faked for debugging)\n"
		"\t-k, --key\t private key file\n"
		"\t-c, --cert\t certificate file\n"
		"\t-C, --CAdir\t trusted certificates directory\n"
		"\t-V, --VOMSdir\t trusted VOMS servers certificates directory\n"
		"\t-p, --port\t port to listen\n"
		"\t-m, --mysql\t database connect string\n"
		"\t-d, --debug\t don't run as daemon, additional diagnostics\n"
		"\t-r, --rgmaexport write state info to RGMA interface\n"
		"\t-n, --noauth\t don't check user identity with result owner\n"
		"\t-s, --slaves\t number of slave servers to fork\n"
		"\t-l, --semaphores number of semaphores (job locks) to use\n"
		"\t-i, --pidfile\t file to store master pid\n"
		"\t-L, --limits\t query limits numbers in format \"events_limit:jobs_limit:size_limit\"\n"
		"\t-N, --notif-dur\t Maximal duration of notification registrations in hours\n"
		"\t-S, --purge-prefix\t purge files full-path prefix\n"
		"\t-D, --dump-prefix\t dump files full-path prefix\n"
		"\t--super-user\t user allowed to bypass authorization and indexing\n"
		"\t--super-users-file\t the same but read the subjects from a file\n"
		"\t--no-index=1\t don't enforce indices for superusers\n"
		"\t          =2\t don't enforce indices at all\n"
		"\t--strict-locking=1\t lock jobs also on storing events (may be slow)\n"
		"\t--notif-il-sock\t socket to send notifications\n"
	,me);
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

static int check_timeout(struct timeval *timeout, struct timeval before, struct timeval after)
{
	return (timeout->tv_usec <= after.tv_usec - before.tv_usec) ? 
			(timeout->tv_sec <= after.tv_sec - before.tv_sec) :
			(timeout->tv_sec < after.tv_sec - before.tv_sec);
}

static void clnt_reject(void *mycred, int conn)
{
	int     flags = fcntl(conn, F_GETFL, 0);

	if (fcntl(conn, F_SETFL, flags | O_NONBLOCK) < 0) 
		return;

	edg_wll_gss_reject(conn);
	return;
}


#define MSG_BUFSIZ	15

/* send socket sock through socket to_sock */
static int do_sendmsg(int to_sock, int sock, unsigned long clnt_dispatched,int store) {

        struct msghdr msg = {0};
        struct cmsghdr *cmsg;
        int myfds; 	/* Contains the file descriptors to pass. */
        char buf[CMSG_SPACE(sizeof myfds)];  /* ancillary data buffer */
        int *fdptr;
        struct iovec sendiov;
        char sendbuf[MSG_BUFSIZ];  		/* to store unsigned int + \0 */


        snprintf(sendbuf,sizeof(sendbuf),"%c %lu",store?'S':'Q',clnt_dispatched);

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &sendiov;
        msg.msg_iovlen = 1;
        sendiov.iov_base = sendbuf;
        sendiov.iov_len = sizeof(sendbuf);

        msg.msg_control = buf;
        msg.msg_controllen = sizeof buf;

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        fdptr = (int *)CMSG_DATA(cmsg);
        *fdptr = sock;

        msg.msg_controllen = cmsg->cmsg_len;
        /* send fd to server-slave to do rest of communication */
	if (sendmsg(to_sock, &msg, 0) < 0)  
		return 1;
        
        return 0;
}


/* receive socket sock through socket from_sock */
static int do_recvmsg(int from_sock, int *sock, unsigned long *clnt_accepted,int *store) {

        struct msghdr msg = {0};
        struct cmsghdr *cmsg;
        int myfds; /* Contains the file descriptors to pass. */
        char buf[CMSG_SPACE(sizeof(myfds))];  /* ancillary data buffer */
        struct iovec recviov;
        char recvbuf[MSG_BUFSIZ],op;


        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &recviov;
        msg.msg_iovlen = 1;
        recviov.iov_base = recvbuf;
        recviov.iov_len = sizeof(recvbuf);

        msg.msg_control = buf;
        msg.msg_controllen = sizeof buf;

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        msg.msg_controllen = cmsg->cmsg_len;

        if (recvmsg(from_sock, &msg, 0) < 0) 
		return 1;
        
        *sock = *((int *)CMSG_DATA(cmsg));
	sscanf(recvbuf,"%c %lu",&op,clnt_accepted);
	*store = op == 'S';

        return 0;
}


struct asyn_result {
	struct hostent *ent;
	int		err;
};

/* ares callback handler for ares_gethostbyaddr()       */
static void callback_handler(void *arg, int status, struct hostent *h) {
	struct asyn_result *arp = (struct asyn_result *) arg;

	switch (status) {
	   case ARES_SUCCESS:
		if (h && h->h_name) {
			arp->ent->h_name = strdup(h->h_name);		
			if (arp->ent->h_name == NULL) {
				arp->err = NETDB_INTERNAL;
			} else {
				arp->err = NETDB_SUCCESS;
			}
		} else {
			arp->err = NO_DATA;
		}
		break;
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

static void free_hostent(struct hostent *h){
	int i;

	if (h) {
		if (h->h_name) free(h->h_name);
		if (h->h_aliases) {
			for (i=0; h->h_aliases[i]; i++) free(h->h_aliases[i]);
			free(h->h_aliases);
		}
		if (h->h_addr_list) {
			for (i=0; h->h_addr_list[i]; i++) free(h->h_addr_list[i]);
	        	free(h->h_addr_list);
		}
		free(h);
	}
}

static int asyn_gethostbyaddr(char **name, const char *addr,int len, int type, struct timeval *timeout) {
	struct asyn_result ar;
	ares_channel channel;
	int nfds;
	fd_set readers, writers;
	struct timeval tv, *tvp;
	struct timeval start_time,check_time;


/* start timer */
        gettimeofday(&start_time,0);

/* ares init */
        if ( ares_init(&channel) != ARES_SUCCESS ) return(NETDB_INTERNAL);
	ar.ent = (struct hostent *) malloc (sizeof(*ar.ent));
	memset((void *) ar.ent, 0, sizeof(*ar.ent));

/* query DNS server asynchronously */
	ares_gethostbyaddr(channel, addr, len, type, callback_handler, (void *) &ar);

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
			free_hostent(ar.ent);
			return(TRY_AGAIN);
		}
		start_time = check_time;

                tvp = ares_timeout(channel, timeout, &tv);

                switch ( select(nfds, &readers, &writers, NULL, tvp) ) {
			case -1: if (errno != EINTR) {
					ares_destroy(channel);
				  	free_hostent(ar.ent);
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

	
	ares_destroy(channel);
		
	if (ar.err == NETDB_SUCCESS) {
		*name = strdup(ar.ent->h_name); 
		free_hostent(ar.ent); 
	}
	return (ar.err);
}

static int read_roots(const char *file)
{
	FILE	*roots = fopen(file,"r");
	char	buf[BUFSIZ];
	int	cnt = 0;

	if (!roots) {
		perror(file);
		return 1;
	}

	while (!feof(roots)) {
		char	*nl;
		fgets(buf,sizeof buf,roots);
		nl = strchr(buf,'\n');
		if (nl) *nl = 0;

		super_users = realloc(super_users, (cnt+1) * sizeof super_users[0]);
		super_users[cnt] = strdup(buf);
		super_users[++cnt] = NULL;
	}

	fclose(roots);

	return 0;
}

static int amIroot(const char *subj)
{
	int	i;

	if (!subj) return 0;
	for (i=0; super_users && super_users[i]; i++) 
		if (strcmp(subj,super_users[i]) == 0) return 1;

	return 0;
}

static int parse_limits(char *opt, int *j_limit, int *e_limit, int *size_limit)
{
	return (sscanf(opt, "%d:%d:%d", j_limit, e_limit, size_limit) == 3);
}

static unsigned long	clnt_dispatched=0, clnt_accepted=0;
static gss_cred_id_t	mycred = GSS_C_NO_CREDENTIAL;
static int	server_sock,store_sock;

static int check_mkdir(const char *);

int main(int argc,char *argv[])
{
	int	fd,i;
	struct sockaddr_in	a;
	struct sigaction	sa;
	sigset_t	sset;
	char	*mysubj = NULL;
	int	opt;
	char	*cert,*key,*port;
	char	*name,pidfile[PATH_MAX] = EDG_BKSERVERD_PIDFILE;
	int	sock_slave[2];
	FILE	*fpid;
	key_t	semkey;
	time_t	cert_mtime = 0,key_mtime = 0;
	edg_wll_Context	ctx;
	OM_uint32	min_stat;
	edg_wll_GssStatus gss_code;

	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	asprintf(&port,"%d",GLITE_WMSC_JOBID_DEFAULT_PORT);
	cert = key = cadir = vomsdir = NULL;

/* no magic here: 1 month, 3 and 7 days */
	purge_timeout[EDG_WLL_PURGE_JOBSTAT_OTHER] = 60*60*24*31;	
	purge_timeout[EDG_WLL_JOB_CLEARED] = 60*60*24*3;
	purge_timeout[EDG_WLL_JOB_ABORTED] = 60*60*24*7;
	purge_timeout[EDG_WLL_JOB_CANCELLED] = 60*60*24*7;

/* no magic here: 1 month, 3 and 7 days */
	purge_timeout[EDG_WLL_PURGE_JOBSTAT_OTHER] = 60*60*24*31;	
	purge_timeout[EDG_WLL_JOB_CLEARED] = 60*60*24*3;
	purge_timeout[EDG_WLL_JOB_ABORTED] = 60*60*24*7;
	purge_timeout[EDG_WLL_JOB_CANCELLED] = 60*60*24*7;

/* no magic here: 1 month, 3 and 7 days */
	purge_timeout[EDG_WLL_PURGE_JOBSTAT_OTHER] = 60*60*24*31;	
	purge_timeout[EDG_WLL_JOB_CLEARED] = 60*60*24*3;
	purge_timeout[EDG_WLL_JOB_ABORTED] = 60*60*24*7;
	purge_timeout[EDG_WLL_JOB_CANCELLED] = 60*60*24*7;

	if (geteuid()) snprintf(pidfile,sizeof pidfile,"%s/edg-bkserverd.pid",
			getenv("HOME"));

	while ((opt = getopt_long(argc,argv,"a:c:k:C:V:p:drm:ns:l:L:N:i:S:D:X:",opts,NULL)) != EOF) switch (opt) {
		case 'a': fake_host = strdup(optarg); break;
		case 'c': cert = optarg; break;
		case 'k': key = optarg; break;
		case 'C': cadir = optarg; break;
		case 'V': vomsdir = optarg; break;
		case 'p': free(port); port = strdup(optarg); break;
		case 'd': debug = 1; break;
		case 'r': rgma_export = 1; break;
		case 'm': dbstring = optarg; break;
		case 'n': noAuth = 1; break;
		case 's': slaves = atoi(optarg); break;
		case 'l': semaphores = atoi(optarg); break;
		case 'S': purgeStorage = optarg; break;
		case 'D': dumpStorage = optarg; break;
		case 'L':
			if ( !parse_limits(optarg, &hardJobsLimit, &hardEventsLimit, &hardRespSizeLimit) )
			{
				usage(name);
				return 1;
			}
			break;
		case 'N': notif_duration = atoi(optarg) * (60*60); break;
		case 'X': notif_ilog_socket_path = strdup(optarg); break;
		case 'i': strcpy(pidfile,optarg); break;
		case 'R': if (super_users) {
				  fprintf(stderr,"%s: super-users already defined, second occurence ignored\n",
						  argv[0]);
				  break;
			  }
			  super_users = malloc(2 * sizeof super_users[0]);
			  super_users[0] = optarg;
			  super_users[1] = NULL;
			  break;
		case 'F': if (super_users) {
				  fprintf(stderr,"%s: super-users already defined, second occurence ignored\n",
						  argv[0]);
				  break;
			  }
			  if (read_roots(optarg)) return 1;
			  break;
		case 'x': noIndex = atoi(optarg);
			  if (noIndex < 0 || noIndex > 2) { usage(name); return 1; }
			  break;
		case 'P': strict_locking = 1;
			  break;
		case '?': usage(name); return 1;
	}

	if (optind < argc) { usage(name); return 1; }

	setlinebuf(stdout);
	setlinebuf(stderr);

	dprintf(("Master pid %d\n",getpid()));
	fpid = fopen(pidfile,"r");
	if (fpid) {
		int	opid = -1;
		if (fscanf(fpid,"%d",&opid) == 1) {
			if (!kill(opid,0)) {
				fprintf(stderr,"%s: another instance running, pid = %d\n",argv[0],opid);
				return 1;
			} else if (errno != ESRCH) {
				perror("kill()"); return 1;
			}
		}
		fclose(fpid);
	} else if (errno != ENOENT) { perror(pidfile); return 1; }

	fpid = fopen(pidfile,"w");
	if (!fpid) { perror(pidfile); return 1; }
	fprintf(fpid,"%d",getpid());
	fclose(fpid);

	semkey = ftok(pidfile,0);

	if (!debug) for (fd=3; fd<OPEN_MAX; fd++) close(fd);

	if (check_mkdir(dumpStorage)) exit(1);
	if (check_mkdir(purgeStorage)) exit(1);

	if (semaphores == -1) semaphores = slaves;
	semset = semget(semkey,0,0);
	if (semset >= 0) semctl(semset,0,IPC_RMID);
	semset = semget(semkey,semaphores,IPC_CREAT | 0600);
	if (semset < 0) { perror("semget()"); return 1; }
	dprintf(("Using %d semaphores, set id %d\n",semaphores,semset));
	for (i=0; i<semaphores; i++) {
		struct sembuf	s;

		s.sem_num = i; s.sem_op = 1; s.sem_flg = 0;
		if (semop(semset,&s,1) == -1) { perror("semop()"); return 1; }
	}

	if (fake_host) {
		char	*p = strchr(fake_host,':');
		if (p) {
			*p = 0;
			fake_port = atoi(p+1);
		} else fake_port = atoi(port);
	}
	else {
		char	buf[300];

		if (globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS)   {
			dprintf(("[%d]: Unable to initialize Globus common module\n",getpid()));
			if (!debug) syslog(LOG_CRIT,"Unable to initialize Globus common module\n");
		}

		globus_libc_gethostname(buf,sizeof buf);
		buf[sizeof buf - 1] = 0;
		fake_host = strdup(buf);
		fake_port = atoi(port); 
	}

	dprintf(("server address: %s:%d\n",fake_host,fake_port));

	server_sock = socket(PF_INET,SOCK_STREAM,0);
	if (server_sock<0) { perror("socket()"); return 1; }

	a.sin_family = AF_INET;
	a.sin_port = htons(atoi(port));
	a.sin_addr.s_addr = INADDR_ANY;

	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(server_sock,(struct sockaddr *) &a,sizeof(a))) { 
		char	buf[100];
		snprintf(buf,sizeof(buf),"bind(%d)",atoi(port));
		perror(buf);
		return 1;
	}

	if (listen(server_sock,CON_QUEUE)) { perror("listen()"); return 1; }

	store_sock = socket(PF_INET,SOCK_STREAM,0);
	if (store_sock<0) { perror("socket()"); return 1; }

	a.sin_family = AF_INET;
	a.sin_port = htons(atoi(port)+1);
	a.sin_addr.s_addr = INADDR_ANY;

	setsockopt(store_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(store_sock,(struct sockaddr *) &a,sizeof(a))) {
		char	buf[100];
		snprintf(buf,sizeof(buf),"bind(%d)",atoi(port)+1);
		perror(buf);
		return 1;
	}
	if (listen(store_sock,CON_QUEUE)) { perror("listen()"); return 1; }

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sock_slave)) {
		perror("socketpair()");
		return 1;
	}

	dprintf(("Initializing SSL ...\n"));
	edg_wlc_SSLInitialization();
	if (!cert || !key) fprintf(stderr,"%s: key or certificate file not specified - unable to watch them for changes!\n",argv[0]);

	if (cadir) setenv("X509_CERT_DIR",cadir,1);
	if (edg_wll_gss_acquire_cred_gsi(cert, &mycred, &mysubj, &gss_code)) {
	   dprintf(("Running unauthenticated\n"));
	} else {
		int	i;
		dprintf(("Server identity: %s\n",mysubj));

		for (i=0; super_users && super_users[i]; i++);
		super_users = realloc(super_users,(i+2) * sizeof *super_users);
		super_users[i] = mysubj;
		super_users[i+1] = NULL;
	}

	if (noAuth) dprintf(("Promiscuous mode\n"));
	dprintf(("Listening at %d,%d (accepting protocols: " COMP_PROTO " and compatible) ...\n",atoi(port),atoi(port)+1));

	if (!dbstring) dbstring = getenv("LBDB");

	/* Just check the database and let it be. The slaves do the job. */
	edg_wll_InitContext(&ctx);
	wait_for_open(ctx,dbstring);

	if (edg_wll_DBCheckVersion(ctx)) {
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);

		fprintf(stderr,"%s: open database: %s (%s)\n",argv[0],et,ed);
		return 1;
	}
	edg_wll_Close(ctx);
	edg_wll_FreeContext(ctx);

	if (!debug) {
		if (daemon(1,0) == -1) {
			perror("deamon()");
			exit(1);
		}

		fpid = fopen(pidfile,"w");
		if (!fpid) { perror(pidfile); return 1; }
		fprintf(fpid,"%d",getpid());
		fclose(fpid);

		openlog(name,LOG_PID,LOG_DAEMON);
	} else {
		setpgid(0, getpid());
	}

	memset(&sa,0,sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = catchsig;
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);

	sa.sa_handler = catch_chld;
	sigaction(SIGCHLD,&sa,NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1,&sa,NULL);

	sigemptyset(&sset);
	sigaddset(&sset,SIGCHLD);
	sigaddset(&sset,SIGTERM);
	sigaddset(&sset,SIGINT);
	sigprocmask(SIG_BLOCK,&sset,NULL);

	for (i=0; i<slaves; i++) slave(mycred,sock_slave[1]);

	while (!die) {
		fd_set	fds;
		int	ret,mx;
		struct timeval	watch_to = { WATCH_TIMEOUT, 0 };
		gss_cred_id_t	newcred = GSS_C_NO_CREDENTIAL;
		edg_wll_GssStatus	gss_code;
		

		FD_ZERO(&fds);
		FD_SET(server_sock,&fds);
		FD_SET(store_sock,&fds);
		FD_SET(sock_slave[0],&fds);

		switch (edg_wll_gss_watch_creds(cert,&cert_mtime)) {
			case 0: break;
			case 1: 
				ret = edg_wll_gss_acquire_cred_gsi(cert, &newcred, NULL, &gss_code);
				if (ret == 0) {
					dprintf(("reloading credentials"));
					gss_release_cred(&min_stat, &mycred);
					mycred = newcred;
					kill(0,SIGUSR1);
				}
				else {
					dprintf(("reloading credentials failed"));
				}

				break;
			case -1: dprintf(("edg_wll_ssl_watch_creds failed\n"));
				 break;
		}
			
		mx = server_sock;
		if (mx < store_sock) mx = store_sock;
		if (mx < sock_slave[0]) mx = sock_slave[0];
		sigprocmask(SIG_UNBLOCK,&sset,NULL);
		ret = select(mx+1,&fds,NULL,NULL,&watch_to);
		sigprocmask(SIG_BLOCK,&sset,NULL);

// XXX is needed?		ctx->p_tmp_timeout.tv_sec = MASTER_TIMEOUT;

		if (ret == -1 && errno != EINTR) {
			if (debug) perror("select()");
			else syslog(LOG_CRIT,"select(): %m");
			return 1;
		}

		if (child_died) {
			int	pid;
			while ((pid=waitpid(-1,NULL,WNOHANG))>0) {
				if (!die) {
					int newpid = slave(mycred,sock_slave[1]);
					dprintf(("[master] Servus mortuus [%d] miraculo resurrexit [%d]\n",pid,newpid));
				}
			}
			child_died = 0;
			continue;
		}

		if (die) continue;

		if (FD_ISSET(server_sock,&fds) && dispatchit(sock_slave[0],server_sock,0)) continue;
		if (FD_ISSET(store_sock,&fds) && dispatchit(sock_slave[0],store_sock,1)) continue;
		
		if (FD_ISSET(sock_slave[0],&fds)) {
/* slave accepted a request */
			unsigned long	a;

			if ((recv(sock_slave[0],&a,sizeof(a),MSG_WAITALL) == sizeof(a))
				&& a<=clnt_dispatched
				&& (a>clnt_accepted || clnt_accepted>clnt_dispatched)
			) clnt_accepted = a;
		}

	}

	dprintf(("[master] Terminating on signal %d\n",die));
	if (!debug) syslog(LOG_INFO,"Terminating on signal %d\n",die);
	kill(0,die);
	semctl(semset,0,IPC_RMID,0);
	unlink(pidfile);
	free(port);
	gss_release_cred(&min_stat, &mycred);
	return 0;
}

static int dispatchit(int sock_slave,int sock,int store)
{
	struct sockaddr_in a;
	int	conn;
	unsigned char *pom;
	int	alen,ret;

	alen=sizeof(a);
	conn = accept(sock,(struct sockaddr *) &a,&alen);

	if (conn<0) { 
		if (debug) {
			perror("accept()"); return 1; 
		} else {
			syslog(LOG_ERR,"accept(): %m");
			sleep(5);
			return -1;
		}
	}

	alen=sizeof(a);
	getpeername(conn,(struct sockaddr *)&a,&alen);
	pom = (char *) &a.sin_addr.s_addr;

	dprintf(("[master] %s connection from %d.%d.%d.%d:%d\n",store?"store":"query",
		(int) pom[0],(int) pom[1],(int) pom[2],(int) pom[3], ntohs(a.sin_port)));

	ret = 0;
	if ((clnt_dispatched<clnt_accepted	/* wraparound */
		|| clnt_dispatched-clnt_accepted < slaves*SLAVE_OVERLOAD)
		&& !(ret=do_sendmsg(sock_slave,conn,clnt_dispatched++,store))) {
			/* all done */; 
			dprintf(("[master] Dispatched %lu, last known served %lu\n",clnt_dispatched-1,clnt_accepted));
		}
	else {
		clnt_reject(mycred,conn);
		dprintf(("[master] Reject due to overload\n"));
	}
	close(conn);
	if (ret) {
		perror("sendmsg()");
		if (!debug) syslog(LOG_ERR,"sendmsg(): %m");
	}
	return 0;
}


static int slave(void *mycred,int sock)
{
	edg_wll_Context	ctx;
	int	conn = -1,pid;
	sigset_t	sset;
	void	*mysql;
	char	*server_name = NULL;
	int	conn_cnt = 0;
	int	h_errno;
	struct sigaction	sa;
	int	sockflags;
	struct timeval	client_done,client_start;

	if ((pid = fork())) return pid;

	srandom(getpid()+time(NULL));

	close(server_sock);
	close(store_sock);

	sigemptyset(&sset);
	sigaddset(&sset,SIGTERM);
	sigaddset(&sset,SIGINT);
	sigaddset(&sset,SIGUSR1);

	memset(&sa,0,sizeof sa);
	sa.sa_handler = catchsig;
	sigaction(SIGUSR1,&sa,NULL);

       	if ((sockflags = fcntl(sock, F_GETFL, 0)) < 0 ||
			fcntl(sock, F_SETFL, sockflags | O_NONBLOCK) < 0)
	{
		dprintf(("[%d] fcntl(master_sock): %s\n",getpid(),strerror(errno)));
		if (!debug) syslog(LOG_CRIT,"fcntl(master_sock): %m");
		exit(1);
	}

	edg_wll_InitContext(&ctx);

	dprintf(("[%d] Spawned, opening database ...\n",getpid()));
	if (!dbstring) dbstring = getenv("LBDB");

	wait_for_open(ctx,dbstring);

	mysql = ctx->mysql;
	if (edg_wll_QueryJobIndices(ctx,&job_index,NULL)) {
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);

		dprintf(("[%d]: query_job_indices(): %s: %s, no custom indices available\n",getpid(),et,ed));
		if (!debug) syslog(LOG_ERR,"[%d]: query_job_indices(): %s: %s, no custom indices available\n",getpid(),et,ed);
		free(et);
		free(ed);
		job_index = NULL;
	}
	edg_wll_FreeContext(ctx);

	if (job_index) {
		int i,j, k, maxncol, ncol;
		ncol = maxncol = 0;
		for (i=0; job_index[i]; i++) {
			for (j=0; job_index[i][j].attr; j++) maxncol++;
		}
		job_index_cols = calloc(maxncol+1, sizeof(edg_wll_IColumnRec));
		for (i=0; job_index[i]; i++) {
			for (j=0; job_index[i][j].attr; j++) {
				for (k=0; k<ncol && edg_wll_CmpColumn(&job_index_cols[k].qrec, &job_index[i][j]); k++);
				if (k==ncol) {
					job_index_cols[ncol].qrec = job_index[i][j];
					if (job_index[i][j].attr == EDG_WLL_QUERY_ATTR_USERTAG) {
						job_index_cols[ncol].qrec.attr_id.tag =
							strdup(job_index[i][j].attr_id.tag);
					}
					job_index_cols[ncol].colname =
						edg_wll_QueryRecToColumn(&job_index_cols[ncol].qrec);
					ncol++;
				}
			}
		}
		job_index_cols[ncol].qrec.attr = EDG_WLL_QUERY_ATTR_UNDEF;
		job_index_cols[ncol].colname = NULL;
	}

	
	while (!die && (conn_cnt < SLAVE_CONNS_MAX || conn >= 0)) {
		fd_set	fds;
		int	alen,store,max = sock,newconn = -1;
		int	connflags,kick_client = 0;
		unsigned long	seq;
		struct 	timeval dns_to = {DNS_TIMEOUT, 0},
				check_to = {SLAVE_CHECK_SIGNALS, 0},
				total_to = { TOTAL_CLNT_TIMEOUT,0 },
				client_to = { CLNT_TIMEOUT,0 }, now;
		char 	*name = NULL;
		struct 	sockaddr_in	a;
		edg_wll_GssStatus	gss_code;


		FD_ZERO(&fds);
		FD_SET(sock,&fds);
		if (conn >= 0) FD_SET(conn,&fds);
		if (conn > sock) max = conn;
	
		sigprocmask(SIG_UNBLOCK,&sset,NULL);
		switch (select(max+1,&fds,NULL,NULL,&check_to)) {
			case -1:
				if (errno != EINTR) {
					dprintf(("[%d] select(): %s\n",getpid(),strerror(errno)));
					if (!debug) syslog(LOG_CRIT,"select(): %m");
					exit(1);
				}
				continue;
			case 0: if (conn < 0) continue;
			default: break;
		}
		sigprocmask(SIG_BLOCK,&sset,NULL);

		gettimeofday(&now,NULL);
	       	if (conn >= 0 && (check_timeout(&client_to,client_done,now) ||
				check_timeout(&total_to,client_start,now)))
					kick_client = 1;

		if (conn >= 0 && !kick_client && FD_ISSET(conn,&fds)) {
	/* serve the request */

			dprintf(("[%d] incoming request\n",getpid()));

			ctx->p_tmp_timeout.tv_sec = SLAVE_TIMEOUT;
			ctx->p_tmp_timeout.tv_usec = 0;

			if (total_to.tv_sec < ctx->p_tmp_timeout.tv_sec)
				/* XXX: sec = && usec < never happens */
			{
				ctx->p_tmp_timeout.tv_sec = total_to.tv_sec;
				ctx->p_tmp_timeout.tv_usec = total_to.tv_usec;
			}

			if (store ? edg_wll_StoreProto(ctx) : edg_wll_ServerHTTP(ctx)) { 
				char    *errt,*errd;
				errt = errd = NULL;

				switch (edg_wll_Error(ctx,&errt,&errd)) {
					case ETIMEDOUT:
						/* fallthrough */
					case EDG_WLL_ERROR_GSS:
					case EPIPE:
						dprintf(("[%d] %s (%s)\n",getpid(),errt,errd));
						if (!debug) syslog(LOG_ERR,"%s (%s)",errt,errd);
						/* fallthrough */
					case ENOTCONN:
						edg_wll_gss_close(&ctx->connPool[ctx->connToUse].gss, NULL);
						edg_wll_FreeContext(ctx);
						ctx = NULL;
						close(conn);
						conn = -1;
						free(errt); free(errd);
						dprintf(("[%d] Connection closed\n",getpid()));
						continue;
						break;
					case ENOENT:
						/* fallthrough */
					case EINVAL:
						/* fallthrough */
					case EPERM:
					case EEXIST:
					case EDG_WLL_ERROR_NOINDEX:
					case E2BIG:
						dprintf(("[%d] %s (%s)\n",getpid(),errt,errd));
						if (!debug) syslog(LOG_ERR,"%s (%s)",errt,errd);
						break; /* no action for non-fatal errors */
					default:
						dprintf(("[%d] %s (%s)\n",getpid(),errt,errd));
						if (!debug) syslog(LOG_CRIT,"%s (%s)",errt,errd);
						exit(1);
				} 
				free(errt); free(errd);
			}
			
			dprintf(("[%d] request done\n",getpid()));
			gettimeofday(&client_done,NULL);
			continue;

		} 


		if (FD_ISSET(sock,&fds) && conn_cnt < SLAVE_CONNS_MAX) {
			if (conn >= 0) usleep(100000 + 1000 * (random() % 200));
			if (do_recvmsg(sock,&newconn,&seq,&store)) switch (errno) {
				case EINTR: /* XXX: signals are blocked */
			 	case EAGAIN: continue;
				default: dprintf(("[%d] recvmsg(): %s\n",getpid(),strerror(errno)));
					if (!debug) syslog(LOG_CRIT,"recvmsg(): %m\n");
					exit(1);
			}
			kick_client = 1;
		}

		if (kick_client && conn >= 0) {
			if (ctx->connPool[ctx->connToUse].gss.context != GSS_C_NO_CONTEXT) {
				struct timeval	to = { 0, CLNT_REJECT_TIMEOUT };
				edg_wll_gss_close(&ctx->connPool[ctx->connToUse].gss,&to);
			}
			edg_wll_FreeContext(ctx);
			close(conn); /* XXX: should not harm */
			conn = -1;
			dprintf(("[%d] Idle connection closed\n",getpid()));
		}

		if (newconn >= 0) {
			OM_uint32       min_stat, maj_stat;
			gss_name_t      client_name = GSS_C_NO_NAME;
			gss_buffer_desc token = GSS_C_EMPTY_BUFFER;

			conn = newconn;
			gettimeofday(&client_start,NULL);
			client_done.tv_sec = client_start.tv_sec;
			client_done.tv_usec = client_start.tv_usec;

			switch (send(sock,&seq,sizeof(seq),0)) {
				case -1:
					if (debug) perror("send()");
					else syslog(LOG_CRIT,"send(): %m\n");
					exit(1);
				case sizeof(seq): break;
				default: dprintf(("[%d] send(): incomplete message\n",getpid()));
					exit(1);
			}
	
			dprintf(("[%d] serving %s %lu\n",getpid(),store?"store":"query",seq));
			conn_cnt++;
	
			connflags = fcntl(conn, F_GETFL, 0);
		       	if (fcntl(conn, F_SETFL, connflags | O_NONBLOCK) < 0) {
				dprintf(("[%d] can't set O_NONBLOCK mode (%s), closing.\n",
							getpid(), strerror(errno)));
				if (!debug) syslog(LOG_ERR,"can't set O_NONBLOCK mode (%s), closing.\n",
						strerror(errno));
	
				close(conn);
				conn = -1;
				continue;
			}
	
			edg_wll_InitContext(&ctx);
	
			/* Shared structures (pointers) */
			ctx->mysql = mysql;
			ctx->job_index_cols = (void*) job_index_cols;
			ctx->job_index = job_index; 
	
			ctx->notifDuration = notif_duration;
			ctx->purgeStorage = strdup(purgeStorage);
			ctx->dumpStorage = strdup(dumpStorage);
			ctx->hardJobsLimit = hardJobsLimit;
			ctx->hardEventsLimit = hardEventsLimit;
			ctx->semset = semset;
			ctx->semaphores = semaphores;
			if (noAuth) ctx->noAuth = 1;
			ctx->rgma_export = rgma_export;
			memcpy(ctx->purge_timeout,purge_timeout,sizeof ctx->purge_timeout);
	
			ctx->p_tmp_timeout.tv_sec = SLAVE_TIMEOUT;
	
			ctx->poolSize = 1;
			ctx->connPool = calloc(1,sizeof(edg_wll_ConnPool));
			ctx->connToUse = 0;
	
			alen = sizeof(a);
	 		getpeername(conn,(struct sockaddr *)&a,&alen);
			ctx->connPool[ctx->connToUse].peerName = strdup(inet_ntoa(a.sin_addr));
			ctx->connPool[ctx->connToUse].peerPort = ntohs(a.sin_port);

			/* not a critical operation, do not waste all SLAVE_TIMEOUT */
			switch (h_errno = asyn_gethostbyaddr(&name,(char *) &a.sin_addr.s_addr,sizeof(a.sin_addr.s_addr),
						AF_INET,&dns_to)) {
				case NETDB_SUCCESS:
					if (name) dprintf(("[%d] connection from %s:%d (%s)\n", getpid(),
								inet_ntoa(a.sin_addr), ntohs(a.sin_port), name));
					free(ctx->connPool[ctx->connToUse].peerName);
					ctx->connPool[ctx->connToUse].peerName = name;
					name = NULL;
					break;
				default:
					if (debug) fprintf(stderr,"gethostbyaddr(%s): %s", inet_ntoa(a.sin_addr), hstrerror(h_errno));
					/* else syslog(LOG_ERR,"gethostbyaddr(%s): %s", inet_ntoa(a.sin_addr), hstrerror(h_errno)); */
					dprintf(("[%d] connection from %s:%d\n", getpid(), inet_ntoa(a.sin_addr), ntohs(a.sin_port)));
					break;
			}
	
			gettimeofday(&now,0);
			if ( decrement_timeout(&ctx->p_tmp_timeout, client_start, now) ) {
				if (debug) fprintf(stderr,"gethostbyaddr() timeout");
				else syslog(LOG_ERR,"gethostbyaddr(): timeout");
				free(name);
				continue;
			}
				
	
			if (fake_host) {
				ctx->srvName = strdup(fake_host);
				ctx->srvPort = fake_port;
			}
			else {
				alen = sizeof(a);
				getsockname(conn,(struct sockaddr *) &a,&alen);
	
				dns_to.tv_sec = DNS_TIMEOUT;
				dns_to.tv_usec = 0;
				switch (h_errno = asyn_gethostbyaddr(&name,(char *) &a.sin_addr.s_addr,sizeof(a.sin_addr.s_addr),
							AF_INET,&dns_to)) {
					case NETDB_SUCCESS:
						ctx->srvName = name;
						if (server_name != NULL) {
							if (strcmp(name, server_name)) {
								if (debug) fprintf(stderr, "different server endpoint names (%s,%s),"
									       		" check DNS PTR records\n", name, server_name);
								else syslog(LOG_ERR,"different server endpoint names (%s,%s),"
											" check DNS PTR records\n", name, server_name);
							}
						} else {
							server_name = strdup(name);
						}
						break;
					default:
						if (debug) fprintf(stderr, "gethostbyaddr(%s): %s", inet_ntoa(a.sin_addr), hstrerror(h_errno));
						else syslog(LOG_ERR,"gethostbyaddr(%s): %s", inet_ntoa(a.sin_addr), hstrerror(h_errno));
						if (server_name != NULL) {
							 ctx->srvName = strdup(server_name);
						} else {
							/* only "GET /jobid" requests refused */
						}
						break;
				}
				ctx->srvPort = ntohs(a.sin_port);
			}
	
			if (edg_wll_gss_accept(mycred, conn, &ctx->p_tmp_timeout, &ctx->connPool[ctx->connToUse].gss, &gss_code)) {
				dprintf(("[%d] Client authentication failed, closing.\n",getpid()));
				if (!debug) syslog(LOG_ERR,"Client authentication failed");
	
				close(conn);
				conn = -1;
				edg_wll_FreeContext(ctx);
				continue;
			} 
			maj_stat = gss_inquire_context(&min_stat, ctx->connPool[ctx->connToUse].gss.context, &client_name, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!GSS_ERROR(maj_stat))
				maj_stat = gss_display_name(&min_stat, client_name, &token, NULL);

			if (!GSS_ERROR(maj_stat)) {
				if (ctx->peerName) free(ctx->peerName);
				ctx->peerName = (char *)token.value;
				memset(&token, 0, sizeof(token));
/* XXX DK: pujde pouzit lifetime z inquire_context()?
				ctx->peerProxyValidity = ASN1_UTCTIME_mktime(X509_get_notAfter(peer));
*/
  
				dprintf(("[%d] client DN: %s\n",getpid(),ctx->peerName));
			} else {
				/* XXX DK: Check if the ANONYMOUS flag is set ? */
				dprintf(("[%d] annonymous client\n",getpid()));
			}
		  
			if (client_name != GSS_C_NO_NAME)
			     gss_release_name(&min_stat, &client_name);
			if (token.value)
			     gss_release_buffer(&min_stat, &token);

			edg_wll_SetVomsGroups(ctx, &ctx->connPool[ctx->connToUse].gss, vomsdir, cadir);
			if (debug && ctx->vomsGroups.len > 0) {
			    int i;
  
			    dprintf(("[%d] client's VOMS groups:\n",getpid()));
			    for (i = 0; i < ctx->vomsGroups.len; i++)
				 dprintf(("\t%s:%s\n", 
					 ctx->vomsGroups.val[i].vo,
					 ctx->vomsGroups.val[i].name));
			}
	
			/* used also to reset start_time after edg_wll_ssl_accept! */
			/* gettimeofday(&start_time,0); */
	
	
			ctx->noAuth = noAuth || amIroot(ctx->peerName);
			switch (noIndex) {
				case 0: ctx->noIndex = 0; break;
				case 1: ctx->noIndex = amIroot(ctx->peerName); break;
				case 2: ctx->noIndex = 1; break;
			}
			ctx->strict_locking = strict_locking;
		}
	}

	if (die) {
		dprintf(("[%d] Terminating on signal %d\n",getpid(),die));
		if (!debug) syslog(LOG_INFO,"Terminating on signal %d",die);
	}
	dprintf(("[%d] Terminating after %d connections\n",getpid(),conn_cnt));
	if (!debug) syslog(LOG_INFO,"Terminating after %d connections",conn_cnt);
	exit(0);
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

static int check_mkdir(const char *dir)
{
	struct stat	sbuf;
	
	if (stat(dir,&sbuf)) {
		if (errno == ENOENT) {
			if (mkdir(dir, S_IRWXU)) {
				dprintf(("[%d] %s: %s\n",
					getpid(),dir,strerror(errno)));

				if (!debug) syslog(LOG_CRIT,"%s: %m",dir);
				return 1;
			}
		}
		else {
			dprintf(("[%d] %s: %s\n",getpid(),strerror(errno)));
			if (!debug) syslog(LOG_CRIT,"%s: %m",dir);
			return 1;
		}
	}
	else if (S_ISDIR(sbuf.st_mode)) return 0;
	else {
		dprintf(("[%d] %s: not a directory\n", getpid(),dir));
		if (!debug) syslog(LOG_CRIT,"%s: not a directory",dir);
		return 1;
	}
}
