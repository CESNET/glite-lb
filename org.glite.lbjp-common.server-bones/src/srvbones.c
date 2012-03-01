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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <errno.h>
#include <netdb.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>

#include "glite/lbu/log.h"

#include "srvbones.h"

/* defaults for GLITE_SBPARAM_* */

#define SLAVES_COUNT		5		/* default number of slaves */
#define SLAVE_OVERLOAD		10		/* queue items per slave */
#define SLAVE_REQS_MAX		500		/* commit suicide after that many connections */
#define IDLE_TIMEOUT		30		/* keep idle connection that many seconds */
#define CONNECT_TIMEOUT		5		/* timeout for establishing a connection */
#define REQUEST_TIMEOUT		10		/* timeout for a single request */ 
#define NEW_CLIENT_DURATION	10		/* how long a client is considered new, i.e. busy
						   connection is not closed to serve other clients */
#define CON_QUEUE		10		/* listen() connection queue */

#ifdef LB_PROF
extern void _start (void), etext (void);
#endif

static int		running = 0;
static volatile int	die = 0,
			child_died = 0;
static unsigned long	clnt_dispatched = 0,
			clnt_accepted = 0;

static struct glite_srvbones_service	*services;
static int				services_ct;

static int		set_slaves_ct = SLAVES_COUNT;
static int		set_slave_overload = SLAVE_OVERLOAD;
static int		set_slave_reqs_max = SLAVE_REQS_MAX;
static struct timeval	set_idle_to = {IDLE_TIMEOUT, 0};
static struct timeval	set_connect_to = {CONNECT_TIMEOUT, 0};
static struct timeval	set_request_to = {REQUEST_TIMEOUT, 0};
static char		*set_log_category = NULL;

static int dispatchit(int, int, int);
static int do_sendmsg(int, int, unsigned long, int);
static int do_recvmsg(int, int *, unsigned long *, int *);
static int check_timeout(struct timeval, struct timeval, struct timeval);
static void catchsig(int);
static void catch_chld(int sig);
static int slave(int (*)(void **), int);

static void glite_srvbones_set_slaves_ct(int);
static void glite_srvbones_set_slave_overload(int);
static void glite_srvbones_set_slave_conns_max(int);
static void set_timeout(struct timeval *,struct timeval *);
static void glite_srvbones_set_log_category(char *);

int glite_srvbones_set_param(glite_srvbones_param_t param, ...)
{
	va_list ap;

	if ( running ) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Attempting to set srv-bones parameter on running server");
		return -1;
	}

	va_start(ap, param);
	switch ( param ) {
	case GLITE_SBPARAM_SLAVES_COUNT:
		glite_srvbones_set_slaves_ct(va_arg(ap,int)); break;
	case GLITE_SBPARAM_SLAVE_OVERLOAD:
		glite_srvbones_set_slave_overload(va_arg(ap,int)); break;
	case GLITE_SBPARAM_SLAVE_CONNS_MAX:
		glite_srvbones_set_slave_conns_max(va_arg(ap,int)); break;
	case GLITE_SBPARAM_IDLE_TIMEOUT:
		set_timeout(&set_idle_to,va_arg(ap,struct timeval *)); break;
	case GLITE_SBPARAM_CONNECT_TIMEOUT:
		set_timeout(&set_connect_to,va_arg(ap,struct timeval *)); break;
	case GLITE_SBPARAM_REQUEST_TIMEOUT:
		set_timeout(&set_request_to,va_arg(ap,struct timeval *)); break;
	case GLITE_SBPARAM_LOG_REQ_CATEGORY:
		glite_srvbones_set_log_category(va_arg(ap,char*)); break;
	}
	va_end(ap);

	return 0;
}

int glite_srvbones_run(
	slave_data_init_hnd				slave_data_init,
	struct glite_srvbones_service  *service_table,
	size_t							table_sz,
	int								dbg)
{
	struct sigaction	sa;
	sigset_t			sset;
	int					sock_slave[2], i;
	int		pstat;


	assert(service_table);
	assert(table_sz > 0);

	services = service_table;
	services_ct = table_sz;

	setlinebuf(stdout);
	setlinebuf(stderr);
	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
		"Master pid %d", getpid());

	if ( socketpair(AF_UNIX, SOCK_STREAM, 0, sock_slave) )
	{
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
			"socketpair()");
		return 1;
	}

	memset(&sa, 0, sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = catchsig;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sa.sa_handler = catch_chld;
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGXFSZ, &sa, NULL);

	sigemptyset(&sset);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigprocmask(SIG_BLOCK, &sset, NULL);

	for ( i = 0; i < set_slaves_ct; i++ )
		slave(slave_data_init, sock_slave[1]);

	while ( !die )
	{
		fd_set			fds;
		int				ret, mx;
		

		FD_ZERO(&fds);
		FD_SET(sock_slave[0], &fds);
		for ( i = 0, mx = sock_slave[0]; i < services_ct; i++ )
		{
			FD_SET(services[i].conn, &fds);
			if ( mx < services[i].conn ) mx = services[i].conn;
		}

		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		ret = select(mx+1, &fds, NULL, NULL, NULL);
		sigprocmask(SIG_BLOCK, &sset, NULL);

		if ( ret == -1 && errno != EINTR )
		{
			glite_common_log(LOG_CATEGORY_ACCESS, LOG_PRIORITY_ERROR, "select()");

			return 1;
		}

		if ( child_died )
		{
			int		pid;

			while ( (pid = waitpid(-1, &pstat, WNOHANG)) > 0 )
			{
				if (WIFEXITED(pstat)) {
					glite_common_log(LOG_CATEGORY_CONTROL, WEXITSTATUS(pstat) ? LOG_PRIORITY_ERROR : LOG_PRIORITY_INFO, "[master] Slave %d exited with return code %d.", pid, WEXITSTATUS(pstat));
					
				} 
				if (WIFSIGNALED(pstat)) {
					int logged = 0;
					switch (WTERMSIG(pstat)) {
						case SIGINT:
						case SIGTERM:
						case SIGUSR1: if (die) break;
						default:
							glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[master] Slave %d terminated with signal %d.", pid, WTERMSIG(pstat));
							logged = 1;
							break;
					}
					if (! logged)
						glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[master] Slave %d terminated with signal %d.", pid, WTERMSIG(pstat));
				}
				if ( !die )
				{
					int newpid = slave(slave_data_init, sock_slave[1]);
					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[master] Servus mortuus [%d] miraculo resurrexit [%d]", pid, newpid);
				}
			}
			child_died = 0;
			continue;
		}

		if ( die ) continue;

		
		if (FD_ISSET(sock_slave[0],&fds)) {
			/* slave accepted a request
			 */
			unsigned long	a;

			if (    (recv(sock_slave[0], &a, sizeof(a), MSG_WAITALL) == sizeof(a))
				 && (a <= clnt_dispatched)
				 && (a > clnt_accepted || clnt_accepted > clnt_dispatched) )
				clnt_accepted = a;
		}

		for ( i = 0; i < services_ct; i++ )
			if (   FD_ISSET(services[i].conn, &fds)
				&& dispatchit(sock_slave[0], services[i].conn ,i) )
				/* Be carefull!!!
				 * This must break this for cykle but start the
				 * while (!die) master cykle from the top also
				 */
				break;
	}

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[master] Terminating on signal %d", die);
	kill(0, die);

	return 0;
}

int glite_srvbones_daemonize(const char *servername, const char *custom_pidfile, const char *custom_logfile) {
	int lfd, opid;
	FILE *fpid;
	pid_t master;
	char *pidfile, *logfile;

	if (!custom_logfile) {
		asprintf(&logfile, "%s/%s.log", geteuid() == 0 ? "/var/log" : getenv("HOME"), servername);
	} else {
		logfile = NULL;
	}
	lfd = open(logfile ? logfile : custom_logfile,O_CREAT|O_TRUNC|O_WRONLY,0600);
	if (lfd < 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, 
			"%s: %s: %s",servername,logfile,strerror(errno));
		free(logfile);
		return 0;
	}
	free(logfile);

	if (daemon(0,1) == -1) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL,
			"can't daemonize");
		return 0;
	}
	dup2(lfd,1);
	dup2(lfd,2);

	if (!custom_pidfile) {
		asprintf(&pidfile, "%s/%s.pid", geteuid() == 0 ? "/var/run" : getenv("HOME"), servername);
	} else {
		pidfile = strdup(custom_pidfile);
	}
	setpgid(0, 0); /* needs for signalling */
	master = getpid();
	fpid = fopen(pidfile,"r");
	if ( fpid )
	{
		opid = -1;

		if ( fscanf(fpid,"%d",&opid) == 1 )
		{
			if ( !kill(opid,0) )
			{
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "%s: another instance running, pid = %d", servername, opid);
				return 0;
			}
			else if (errno != ESRCH) { 
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "kill()");
				return 0; 
			}
		}
		fclose(fpid);
	} else if (errno != ENOENT) { 
		glite_common_log_msg(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, pidfile);
		free(pidfile); 
		return 0; 
	}

	if (((fpid = fopen(pidfile, "w")) == NULL) || 
	    (fprintf(fpid, "%d", getpid()) <= 0) ||
	    (fclose(fpid) != 0)) { 
		glite_common_log_msg(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, pidfile);
		free(pidfile); 
		return 0;
	}

	free(pidfile);
	return 1;
}

int glite_srvbones_daemon_listen(const char *name, char *port, int *conn_out) {
	struct	addrinfo *ai;
	struct	addrinfo hints;
	int	conn;
	int 	gaie;
	static const int zero = 0;
	static const int one = 1;

	memset (&hints, '\0', sizeof (hints));
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET6;

	gaie = getaddrinfo (name, port, &hints, &ai);
	if (gaie != 0 || ai == NULL) {
		hints.ai_family = 0;
		gaie = getaddrinfo (NULL, port, &hints, &ai);
	}

	gaie = getaddrinfo (name, port, &hints, &ai);
	if (gaie != 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "getaddrinfo: %s", gai_strerror (gaie));
		return 1;
	}
	if (ai == NULL) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "getaddrinfo: no result");
		return 1;
	}

	conn = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if ( conn < 0 ) { 
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "socket(): %s", strerror(errno));
		freeaddrinfo(ai);
		return 1; 
	}
	setsockopt(conn, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (ai->ai_family == AF_INET6)
		setsockopt(conn, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero));

	if ( bind(conn, ai->ai_addr, ai->ai_addrlen) )
	{
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "bind(%s): %s", port, strerror(errno));
		close(conn);
		freeaddrinfo(ai);
		return 1;
	}

	if ( listen(conn, CON_QUEUE) ) { 
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "listen(): %s", strerror(errno));
		close(conn);
		freeaddrinfo(ai);
		return 1; 
	}

	freeaddrinfo(ai);

	*conn_out = conn;
	return 0;
}

static int dispatchit(int sock_slave, int sock, int sidx)
{
	struct sockaddr_storage	a;
	char			peerhost[64], peerserv[16];
	int			conn, ret;
	socklen_t		alen;


	alen = sizeof(a);
	if ( (conn = accept(sock, (struct sockaddr *)&a, &alen)) < 0 )
	{
		glite_common_log(set_log_category, LOG_PRIORITY_WARN, "accept()");
		sleep(5);
		return -1;
	}

	getpeername(conn, (struct sockaddr *)&a, &alen);
	if (a.ss_family  == PF_LOCAL) {
		glite_common_log(set_log_category,
			 LOG_PRIORITY_DEBUG, 
			"[master] %s connection from local socket", 
			services[sidx].id ? services[sidx].id : "");
	}
	else {
		ret = getnameinfo ((struct sockaddr *) &a, alen,
			peerhost, sizeof(peerhost), peerserv, sizeof(peerserv), NI_NUMERICHOST | NI_NUMERICSERV);
    		if (ret) {
        		glite_common_log(set_log_category, LOG_PRIORITY_WARN, "getnameinfo: %s", gai_strerror (ret));
        			strcpy(peerhost, "unknown"); strcpy(peerserv, "unknown");
    		}

		glite_common_log(set_log_category, 
                         LOG_PRIORITY_DEBUG,
			"[master] %s connection from %s:%s",
                        services[sidx].id ? services[sidx].id : "",
                        peerhost, peerserv);
	}

	ret = 0;
	if (    (   clnt_dispatched < clnt_accepted	/* wraparound */
		     || clnt_dispatched - clnt_accepted < set_slaves_ct * set_slave_overload)
		&& !(ret = do_sendmsg(sock_slave, conn, clnt_dispatched++, sidx)) )
	{
		/*	all done
		 */ 
		glite_common_log(set_log_category,
                         LOG_PRIORITY_DEBUG,
			"[master] Dispatched %lu, last known served %lu",
			clnt_dispatched-1, clnt_accepted);
	}
	else
	{
		services[sidx].on_reject_hnd(conn);
		glite_common_log(set_log_category,
                         LOG_PRIORITY_ERROR,
			"[master] Rejected new connection due to overload");
	}

	close(conn);
	if (ret)
	{
		glite_common_log(set_log_category,
                         LOG_PRIORITY_WARN, "sendmsg()");
	}


	return 0;
}


static int slave(slave_data_init_hnd data_init_hnd, int sock)
{
	sigset_t		sset;
	struct sigaction	sa;
	struct timeval		client_done,
				client_start,
				new_client_duration = { NEW_CLIENT_DURATION, 0 };

	void	*clnt_data = NULL;
	int	conn = -1,
		srv = -1,
		req_cnt = 0,
		sockflags,
		h_errno,
		pid, i,
		first_request = 0; /* 1 -> first request from connected client expected */



	if ( (pid = fork()) ) return pid;

#ifdef LB_PROF
	monstartup((u_long)&_start, (u_long)&etext);
#endif

	srandom(getpid()+time(NULL));

	for ( i = 0; i < services_ct; i++ )
		close(services[i].conn);

	sigemptyset(&sset);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigaddset(&sset, SIGUSR1);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = catchsig;
	sigaction(SIGUSR1, &sa, NULL);

	if (   (sockflags = fcntl(sock, F_GETFL, 0)) < 0
		|| fcntl(sock, F_SETFL, sockflags | O_NONBLOCK) < 0 )
	{
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
			"[%d] fcntl(master_sock): %s", 
			getpid(), strerror(errno));
		exit(1);
	}

	if ( data_init_hnd && data_init_hnd(&clnt_data) )
		/*
		 *	XXX: what if the error remains and master will start new slave
		 *	again and again?
		 *
		 *	Then we are in a deep shit.
		 */
		exit(1);

	while ( !die && (req_cnt < set_slave_reqs_max || (conn >= 0 && first_request)))
	{
		fd_set				fds;
		int					max = sock,
							connflags,
							newconn = -1,
							newsrv = -1;

		enum { KICK_DONT = 0, KICK_IDLE, KICK_LOAD, KICK_HANDLER, KICK_COUNT }
			kick_client = KICK_DONT;

		static char * kicks[] = {
			"don't kick",
			"idle client",
			"high load",
			"no request handler",
			"request count limit reached",
		};
		unsigned long		seq;
		struct timeval		now,to;


		FD_ZERO(&fds);
		if ( conn < 0 || !first_request) FD_SET(sock, &fds);
		if ( conn >= 0 ) FD_SET(conn, &fds);
		if ( conn > sock ) max = conn;
	
		to = set_idle_to;
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		switch (select(max+1, &fds, NULL, NULL, to.tv_sec >= 0 ? &to : NULL))
		{
		case -1:
			if ( errno != EINTR )
			{
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[%d] select(): %s", getpid(), strerror(errno));
				exit(1);
			}
			continue;
			
		case 0:
			if ( conn < 0 ) continue;
			
		default:
			break;
		}
		sigprocmask(SIG_BLOCK, &sset, NULL);

		gettimeofday(&now,NULL);

		if ( conn >= 0 && FD_ISSET(conn, &fds) )
		{
			/*
			 *	serve the request
			 */
			int		rv;

			glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] incoming request", getpid());

			if ( !services[srv].on_request_hnd )
			{
				kick_client = KICK_HANDLER;
			} else {
				req_cnt++;
				first_request = 0;
				to = set_request_to;
				rv = services[srv].on_request_hnd(conn,to.tv_sec>=0 ? &to : NULL,clnt_data);
				if ( (rv == ENOTCONN) || (rv == ECONNREFUSED) ) {
					if (services[srv].on_disconnect_hnd
							&& (rv = services[srv].on_disconnect_hnd(conn,NULL,clnt_data)))
					{
						glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] disconnect handler: %s, terminating",getpid(),strerror(rv));
						exit(1);
					}
					close(conn);
					conn = -1;
					srv = -1;
					glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] Connection closed", getpid());
				}
				else if (rv > 0) {
					/*	non-fatal error -> close connection and contiue
					 * XXX: likely to leak resources but can we call on_disconnect_hnd() on error? 
					 */
					close(conn);
					conn = -1;
					srv = -1;
					glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] %s, connection closed",getpid(),strerror(rv));
					continue;
				} else if (rv == -EINPROGRESS) {
					/*	background operation -> parent forked -> kill slave */
					glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] terminating parent",getpid());
					exit(0);
				} else if ( rv < 0 ) {
					/*	unknown error -> clasified as FATAL -> kill slave
					 */
					glite_common_log(set_log_category, LOG_PRIORITY_INFO, "[%d] %s, terminating",getpid(),strerror(-rv));
					exit(1);
				}
				else {
					glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] request done", getpid());
					gettimeofday(&client_done, NULL);
				}

				if (!check_timeout(new_client_duration,client_start,now)) continue;

			}
		} else {
			if (conn >= 0 && check_timeout(set_idle_to, client_done, now)) 
				kick_client = KICK_IDLE;
		}

		if ( !die && (conn < 0 || !first_request) && FD_ISSET(sock, &fds) && req_cnt < set_slave_reqs_max )
		{
			/* Prefer slaves with no connection, then kick idle clients,
			 * active ones last. Wait less if we have serviced a request in the meantime.
			 * Tuned for HZ=100 timer. */
			if ( conn >= 0 ) usleep( kick_client || FD_ISSET(conn, &fds) ? 11000 : 21000);
			if ( do_recvmsg(sock, &newconn, &seq, &newsrv) ) switch ( errno )
			{
			case EINTR: /* XXX: signals are blocked */
		 	case EAGAIN:
				continue;
			default: 
				glite_common_log(set_log_category, LOG_PRIORITY_ERROR, "[%d] recvmsg(): %s", getpid(), strerror(errno));
				exit(1);
			}
			kick_client = KICK_LOAD;
		}

		if (req_cnt >= set_slave_reqs_max && !first_request) kick_client = KICK_COUNT;

		if ( kick_client && conn >= 0 )
		{
			if ( services[srv].on_disconnect_hnd )
				services[srv].on_disconnect_hnd(conn, NULL, clnt_data);
			close(conn);
			conn = -1;
			srv = -1;
			glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] Connection closed, %s", getpid(), kicks[kick_client]);
		}

		if ( newconn >= 0 )
		{
			int	ret;

			conn = newconn;
			srv = newsrv;
			gettimeofday(&client_start, NULL);

			switch ( send(sock, &seq, sizeof(seq), 0) )
			{
			case -1:
				glite_common_log(set_log_category, LOG_PRIORITY_ERROR, "send()");
				exit(1);
				
			case sizeof(seq):
				break;
				
			default: 
				glite_common_log(set_log_category, LOG_PRIORITY_DEBUG, "[%d] send(): incomplete message", getpid());
				exit(1);
			}
	
			req_cnt++;
			glite_common_log(set_log_category, 
				LOG_PRIORITY_DEBUG, 
				"[%d] serving %s connection %lu", getpid(),
				services[srv].id ? services[srv].id : "", seq);
	
			connflags = fcntl(conn, F_GETFL, 0);
			if ( fcntl(conn, F_SETFL, connflags | O_NONBLOCK) < 0 )
			{
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "[%d] can't set O_NONBLOCK mode (%s), closing.", getpid(), strerror(errno));
				close(conn);
				conn = srv = -1;
				continue;
			}

			to = set_connect_to;
			if (   services[srv].on_new_conn_hnd
				&& (ret = services[srv].on_new_conn_hnd(conn, to.tv_sec >= 0 ? &to : NULL, clnt_data)) )
			{
				glite_common_log(set_log_category, LOG_PRIORITY_WARN, "[%d] Connection not established, err = %d.", getpid(),ret);
				close(conn);
				conn = srv = -1;
				if (ret < 0) exit(1);
				continue;
			}
			gettimeofday(&client_done, NULL);
			first_request = 1;
		}
	}

	if ( die )
	{
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
			"[%d] Terminating on signal %d", getpid(), die);
	}

	if (conn >= 0  && services[srv].on_disconnect_hnd )
		services[srv].on_disconnect_hnd(conn, NULL, clnt_data);

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO,
		"[%d] Terminating after %d requests", getpid(), req_cnt);


	exit(0);
}

static void catchsig(int sig)
{
	die = sig;
}

static void catch_chld(int sig)
{
	child_died = 1;
}

static int check_timeout(struct timeval timeout, struct timeval before, struct timeval after)
{
	return (timeout.tv_usec <= after.tv_usec - before.tv_usec) ? 
			(timeout.tv_sec <= after.tv_sec - before.tv_sec) :
			(timeout.tv_sec < after.tv_sec - before.tv_sec);
}

#define MSG_BUFSIZ	30

/*
 * send socket sock through socket to_sock
 */
static int do_sendmsg(int to_sock, int sock, unsigned long clnt_dispatched, int srv)
{
	struct msghdr		msg;
	struct cmsghdr	   *cmsg;
	struct iovec		sendiov;
	int					myfds,							/* file descriptors to pass. */
					   *fdptr;
	char				buf[CMSG_SPACE(sizeof myfds)];	/* ancillary data buffer */
	char				sendbuf[MSG_BUFSIZ];			/* to store unsigned int + \0 */


	memset(sendbuf, 0, sizeof(sendbuf));
	snprintf(sendbuf, sizeof(sendbuf), "%u %lu", srv, clnt_dispatched);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &sendiov;
	msg.msg_iovlen = 1;
	sendiov.iov_base = sendbuf;
	sendiov.iov_len = sizeof(sendbuf);

	memset(buf, 0, sizeof(buf));
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
static int do_recvmsg(int from_sock, int *sock, unsigned long *clnt_accepted,int *srv)
{
	struct msghdr		msg;
	struct cmsghdr	   *cmsg;
	struct iovec		recviov;
	int					myfds;							/* file descriptors to pass. */
	char				buf[CMSG_SPACE(sizeof(myfds))];	/* ancillary data buffer */
	char				recvbuf[MSG_BUFSIZ];


	memset(&msg, 0, sizeof(msg));
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
	sscanf(recvbuf, "%u %lu", srv, clnt_accepted);

	return 0;
}

static void glite_srvbones_set_slaves_ct(int n)
{
	set_slaves_ct = (n == -1)? SLAVES_COUNT: n;
}

static void glite_srvbones_set_slave_overload(int n)
{
	set_slave_overload = (n == -1)? SLAVE_OVERLOAD: n;
}

static void glite_srvbones_set_slave_conns_max(int n)
{
	set_slave_reqs_max = (n == -1)? SLAVE_REQS_MAX: n;
}

static void set_timeout(struct timeval *to, struct timeval *val)
{
	if (val) {
	/* XXX: why not, negative timeouts don't make any sense, IMHO */
		assert(val->tv_sec >= 0);
		*to = *val;
	}
	else to->tv_sec = -1;
}

static void glite_srvbones_set_log_category(char *log_category)
{
	free(set_log_category);
	set_log_category = strdup(log_category);
}
