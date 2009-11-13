#ident "$Header$"

/*
 * Real time monitor.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <assert.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <glite/security/glite_gss.h>
#ifdef WITH_LBU_DB
#include <glite/lbu/trio.h>
#include <glite/lbu/db.h>
#endif
#include <glite/lb/context.h>
#ifndef WITH_OLD_LB
#include <glite/lb/connpool.h>
#endif
#include <glite/lb/notification.h>
#include <glite/lb/consumer.h>


// default number of the threads/sockets
#define RTM_THREADS 5
// requested notification life in seconds
#define RTM_NOTIF_TTL 86400
// consider end of the notification life sooner
#define RTM_NOTIF_TTL_TO_DEAD 2
// poll timeout in seconds
#define RTM_NOTIF_READ_TIMEOUT 5
// recheck LB server after error in seconds
#define RTM_ERROR_REPEAT_RATE 120
// initial read loop time (can be infinity)
#define RTM_NOTIF_LOOP_MAX_TIME 1800
// idle "quit" poll
#define RTM_IDLE_POLL_TIME 0.5
// purge & summary jobs poll time
#define RTM_SUMMARY_POLL_TIME 600
// preventive suicide against memleaks and ugly things (12 h)
#define RTM_SUICIDE_TIME 43200

#define RTM_SUMMARY_JOBS 100

#define RTM_DB_TABLE_JOBS "jobs"
#define RTM_DB_TABLE_LBS "lb20"
#define DBPAR(N) ("$" (N))
#define DBAMP "\""

// debug message level: insane, debug, progress, warning, error
#define INS 4
#define DBG 3
#define INF 2
#define WRN 1
#define ERR 0
#define DEBUG_LEVEL_MASK 7
#define DEBUG_GUARD_MASK 8

// internal quit codes
#define RTM_QUIT_RUN 0
#define RTM_QUIT_CLEANUP 1
#define RTM_QUIT_PRESERVE 2
#define RTM_QUIT_RELOAD 3

// exit codes
#define RTM_EXIT_OK 0
#define RTM_EXIT_RELOAD 1
#define RTM_EXIT_ERROR 2

#define RTM_NOTIF_TYPE_STATUS 1
#define RTM_NOTIF_TYPE_JDL 2
#define RTM_NOTIF_TYPE_OLD 3
#define RTM_NOTIF_TYPE_DONE 4

#ifdef RTM_NO_COLORS
#define RTM_TTY_RED ""
#define RTM_TTY_GREEN ""
#define RTM_TTY_RST ""
#else
#define RTM_TTY_RED "\e[1;31m"
#define RTM_TTY_GREEN "\e[1;32m"
#define RTM_TTY_RST "\e[0;39m"
#endif

#ifndef LINE_MAX
#define LINE_MAX 1023
#endif

#define RTM_FILE_NOTIFS "/var/tmp/notifs.txt"
#define WLCG_FILENAME_TEMPLATE "/tmp/wlcg_%02d_XXXXXX"
#define WLCG_COMMAND_MESSAGE "/opt/lcg/bin/msg-publish -c /opt/lcg/etc/msg-publish.conf org.wlcg.usage.jobStatus %s"
#define WLCG_BINARY "/opt/lcg/bin/msg-publish"
#define WLCG_CONFIG "/opt/lcg/etc/msg/msg-publish.conf.wlcg"
#define WLCG_TOPIC "org.wlcg.usage.jobStatus"


#ifdef WITH_OLD_LB
#define glite_jobid_t edg_wlc_JobId
#define glite_jobid_create edg_wlc_JobIdCreate
#define glite_jobid_recreate edg_wlc_JobIdRecreate
#define glite_jobid_dup edg_wlc_JobIdDup
#define glite_jobid_free edg_wlc_JobIdFree
#define glite_jobid_parse edg_wlc_JobIdParse
#define glite_jobid_unparse edg_wlc_JobIdUnparse
#define glite_jobid_getServer edg_wlc_JobIdGetServer
#define glite_jobid_getServerParts edg_wlc_JobIdGetServerParts
#define glite_jobid_getUnique edg_wlc_JobIdGetUnique
#endif
#ifndef GLITE_JOBID_DEFAULT_PORT
#define GLITE_JOBID_DEFAULT_PORT GLITE_WMSC_JOBID_DEFAULT_PORT
#define edg_wll_NotifNew(CTX, CONDS, FLAGS, SOCK, LADDR, ID, VALID) edg_wll_NotifNew((CTX), (CONDS), (SOCK), (LADDR), (ID), (VALID))
#define edg_wll_JDLField(STAT, NAME) NULL
#endif

// TODO: ipv6? :-)

typedef struct {
	edg_wll_NotifId id;  // notification context (after bootstrap/rebind)
	char *id_str;        // notification id string
	int type;            // for distinguish various notifications on one LB
	char *server;        // LB server hostname
	unsigned int port;   // LB server port
	time_t valid;        // maximal validity of the notification
	time_t refresh;      // when try to refresh (before expiration),
	                     // used for retry time after error too
	double last_update;  // last change from the server
	int active;          // helper (compare LB servers and notifications,
	                     // if to save to the persistent storage)
	int error;           // errors counter
} notif_t;

typedef struct {
	int id;
	pthread_t thread;
	notif_t *notifs;
	int nservers;
	time_t first_refresh;
	char time_s[100];
	char *dash_filename;
	int dash_fd;
#ifdef WITH_LBU_DB
	glite_lbu_DBContext dbctx;
	glite_lbu_Statement insertcmd, updatecmd, updatecmd_vo;
	int dbcaps;
#endif
} thread_t;

typedef struct {
	char *local_address;
	int nthreads;
	char *config_file;
	char *notif_file;
	int debug;
	int guard;
	int daemonize;
	char *pidfile;
	int dive;
	char *dbcs;  // DB connection string
	char *cert, *key;
	int ttl;     // requested time to live (validity) of the notifications
	int cleanup;        // if to clean up notifications on LB servers
	int wlcg;           // dashboard messaging
	int wlcg_no_remove; // don't remove temporary files (for debugging)
	char *wlcg_binary;  // path msg-publish binary
	char *wlcg_config;  // msg config file
	char *wlcg_topic;   // msg topic
	int wlcg_flush;     // send message for eachnotification
	int silly;          // old LB 3.1 mode

	int nservers;
	notif_t *notifs;
} config_t;

typedef struct {
	notif_t *notifs;
	int n, maxn;
	pthread_mutex_t lock;
	double last_check;
	int was_summary; // flag for debugging
#ifdef WITH_LBU_DB
	glite_lbu_DBContext dbctx;
#endif
} db_t;


static const char rcsid[] = "@(#)$Id$";

static int rtm2syslog[] = {
	LOG_ERR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG,
	LOG_DEBUG,
};

static const struct option opts[] = {
	{ "wlcg-binary", required_argument, 	NULL,   0},
	{ "wlcg-config", required_argument, 	NULL,   0},
	{ "wlcg-topic", required_argument, 	NULL,   0},
	{ "wlcg-flush", no_argument,	 	NULL,   0},
	{ "help", 	no_argument, 		NULL,	'h'},
	{ "version",	no_argument,		NULL,	'v'},
	{ "threads",	required_argument,	NULL,	's'},
	{ "debug",	required_argument,	NULL,	'd'},
	{ "daemonize",	no_argument,		NULL,	'D'},
	{ "pidfile",	required_argument,	NULL,	'i'},
	{ "ttl",	required_argument,	NULL,	't'},
	{ "history",	required_argument,	NULL,	'H'},
	{ "config",	required_argument,	NULL,	'c'},
	{ "notifs",	required_argument,	NULL,	'n'},
	{ "port",	required_argument,	NULL,	'p'},
	{ "pg",		required_argument,	NULL,	'm'},
 	{ "cert",	required_argument,	NULL,	'C'},
 	{ "key",	required_argument,	NULL,	'K'},
	{ "wlcg",	no_argument,		NULL,	'w'},
	{ "old",	no_argument,		NULL,	'o'},
	{ "cleanup",	no_argument,		NULL,	'l'},
	{ NULL, 	no_argument, 		NULL, 	0}
};

static const char *opts_line = "hvs:d:Di:t:H:c:n:p:m:C:K:wo";

config_t config = {
	local_address: NULL,
	nthreads: RTM_THREADS,
	config_file: NULL,
	notif_file: NULL,
	debug: DBG,
	guard: 1,
	dive: 10800,
	dbcs: NULL,
	cert: NULL,
	key: NULL,
	ttl: RTM_NOTIF_TTL,
	cleanup: 0,
	wlcg: 0,
	silly: 0,

	nservers: 0,
	notifs: NULL,
};
db_t db = {
	notifs: NULL,
	n: 0,
	maxn: 0,
	lock: PTHREAD_MUTEX_INITIALIZER,
#ifdef WITH_LBU_DB
	dbctx: NULL
#endif
};
thread_t *threads = NULL;
volatile sig_atomic_t quit = RTM_QUIT_RUN;

static int listen_port = 0;

#define lprintf(T, LEVEL, FMT, ARGS...) \
	if ((LEVEL) <= config.debug) lprintf_func((T), (LEVEL), (FMT), ##ARGS)
#define lprintf_ctx(T, LEVEL, CTX, FMT, ARGS...) \
	if ((LEVEL) <= config.debug) lprintf_ctx_func((T), (CTX), (LEVEL), (FMT), ##ARGS)
#define lprintf_dbctx(T, LEVEL, FMT, ARGS...) \
	if ((LEVEL) <= config.debug) lprintf_dbctx_func((T), (LEVEL), (FMT), ##ARGS)

#ifdef WITH_OLD_LB
int edg_wll_gss_initialize() {
	if (globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE) != GLOBUS_SUCCESS) return EINVAL;
	return 0;
}
#endif

void lvprintf_func(thread_t *t, const char *description, int level, const char *fmt, va_list ap) {
	char prefix[10];
	char *msg, *line;
	
	if (t) snprintf(prefix, sizeof prefix, "[%02d]", t->id);
	else memcpy(prefix, "[main]", 8);
	vasprintf(&msg, fmt, ap);
	if (description) asprintf(&line, "%s %s, %s\n", prefix, msg, description);
	else asprintf(&line, "%s %s\n", prefix, msg);
	free(msg);

	if (level <= WRN && !config.daemonize) fprintf(stderr, RTM_TTY_RED);
	if (config.daemonize) {
		openlog(NULL, LOG_PID | LOG_CONS, LOG_DAEMON);
		syslog(rtm2syslog[level], "%s", line);
		closelog();
	} else {
		fputs(line, stderr);
	}
	if (level <= WRN && !config.daemonize) fprintf(stderr, RTM_TTY_RST);

	free(line);
}


void lprintf_func(thread_t *t, int level, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	lvprintf_func(t, NULL, level, fmt, ap);
	va_end(ap);
}


void lprintf_ctx_func(thread_t *t, edg_wll_Context ctx, int level, const char *fmt, ...) {
	va_list ap;
	char *errText, *errDesc, *s;

	va_start(ap, fmt);
	edg_wll_Error(ctx, &errText, &errDesc);
	asprintf(&s, "%s: %s", errText, errDesc);
	lvprintf_func(t, s, level, fmt, ap);
	free(errText);
	free(errDesc);
	free(s);
	va_end(ap);
}


#ifdef WITH_LBU_DB
void lprintf_dbctx_func(thread_t *t, int level, const char *fmt, ...) {
	va_list ap;
	char *errText = NULL, *errDesc = NULL, *s = NULL;
	glite_lbu_DBContext dbctx = t ? t->dbctx : db.dbctx;

	va_start(ap, fmt);
	if (dbctx) {
		glite_lbu_DBError(dbctx, &errText, &errDesc);
		asprintf(&s, "%s: %s", errText, errDesc);
	}
	lvprintf_func(t, s, level, fmt, ap);
	free(errText);
	free(errDesc);
	free(s);
	va_end(ap);
}
#endif

#ifndef WITH_LBU_DB
time_t glite_lbu_StrToTime(const char *str) {
	struct tm       tm;

	memset(&tm,0,sizeof(tm));
	putenv("TZ=UTC"); tzset();
	sscanf(str,"%4d-%02d-%02d %02d:%02d:%02d",
	        &tm.tm_year,&tm.tm_mon,&tm.tm_mday,
        	&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
}

double glite_lbu_StrToTimestamp(const char *str) {
	struct tm	tm;
	double	sec;

	memset(&tm,0,sizeof(tm));
	putenv("TZ=UTC"); tzset();
	sscanf(str,"%4d-%02d-%02d %02d:%02d:%lf",
		&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
		&tm.tm_hour,&tm.tm_min,&sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;
	tm.tm_sec = sec;

	return (sec - tm.tm_sec) + mktime(&tm);
}
#endif


// hacky time->string conversion
char *time2str(thread_t *t, time_t time) {
	struct tm tm;

	if ((int)time <= 0) memcpy(t->time_s, "-", sizeof("-"));
	else {
		localtime_r(&time, &tm);
		strftime(t->time_s, sizeof(t->time_s), "%F %T", &tm);
	}
	return t->time_s;
}


double rtm_gettimeofday() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}


void rtm_time2str(time_t t, char **str) {
	struct tm	*tm;

	if (t) {
		tm = gmtime(&t);
		asprintf(str,"%4d-%02d-%02d %02d:%02d:%02d",tm->tm_year+1900,tm->tm_mon+1,
			tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	} else
		*str = strdup("-");
}


void rtm_timestamp2str(double t, char **str) {
	time_t tsec = t;
	struct tm *tm = gmtime(&tsec);

	if (t) {
		t = t - tsec + tm->tm_sec;
		asprintf(str,"%4d-%02d-%02d %02d:%02d:%02.09f",tm->tm_year+1900,tm->tm_mon+1,
	        	 tm->tm_mday,tm->tm_hour,tm->tm_min,t);
	} else
		*str = strdup("-");
}


int rtm_str2notiftype(const char *str) {
	if (strcasecmp(str, "STATUS") == 0) return RTM_NOTIF_TYPE_STATUS;
	if (strcasecmp(str, "DONE") == 0) return RTM_NOTIF_TYPE_DONE;
	if (strcasecmp(str, "JDL") == 0) return RTM_NOTIF_TYPE_JDL;
	if (strcasecmp(str, "OLD") == 0) return RTM_NOTIF_TYPE_OLD;
	return -1;
}


const char *rtm_notiftype2str(int type) {
	switch (type) {
	case RTM_NOTIF_TYPE_STATUS: return "STATUS";
	case RTM_NOTIF_TYPE_DONE: return "DONE";
	case RTM_NOTIF_TYPE_JDL: return "JDL";
	case RTM_NOTIF_TYPE_OLD: return "OLD";
	default: return NULL;
	}
}


void wlcg_timeval2str(struct timeval *t, char **str) {
	struct tm	*tm;

	tm = gmtime(&t->tv_sec);
	asprintf(str,"%4d-%02d-%02dT%02d:%02d:%02dZ",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
}


int wlcg_store_message(thread_t *t, __attribute((unused))notif_t *notif, edg_wll_JobStat *stat) {
	unsigned int port;
	int status = 0;
	char *jobid_str = NULL, *state_str = NULL, *vo = NULL, *lbhost = NULL;
	char *wlcg_last_update_time_str = NULL, *wlcg_state_start_time_str = NULL;

	jobid_str = stat->jobId ? glite_jobid_unparse(stat->jobId) : strdup("Unknown");
	glite_jobid_getServerParts(stat->jobId, &lbhost, &port);
	state_str = edg_wll_StatToString(stat->state);
	vo = edg_wll_JDLField(stat,"VirtualOrganisation") ? : strdup("Unknown");

	if (!t->dash_filename || !t->dash_fd) {
		free(t->dash_filename);
		asprintf(&t->dash_filename, WLCG_FILENAME_TEMPLATE, t->id);
		if ((t->dash_fd = mkstemp(t->dash_filename)) == -1) {
			status = errno;
			lprintf(t, ERR, "can't create temporary file '%s': %s", t->dash_filename, strerror(status));
			free(t->dash_filename);
			t->dash_filename = NULL;
			goto quit;
		}
	}

	wlcg_timeval2str(&stat->lastUpdateTime, &wlcg_last_update_time_str);
	wlcg_timeval2str(&stat->stateEnterTime, &wlcg_state_start_time_str);

	dprintf(t->dash_fd, "jobId: %s\n\
stateName: %s\n\
ownerDN: %s\n\
voname: %s\n\
bkHost: %s:%d\n\
networkHost: %s\n\
lastUpdateTime: %s\n\
stateStartTime: %s\n\
exitCode: %d\n\
DoneCode: %d\n\
destSite: %s\n\
condorId: %s\n\
StatusReason: %s\n\
EOT\n", jobid_str, state_str, stat->owner, vo, lbhost, port, stat->network_server ? : "unknown", wlcg_last_update_time_str, wlcg_state_start_time_str, stat->exit_code, stat->done_code, stat->destination ? : "NULLByPublisher", stat->condorId ? : "0", stat->reason && stat->reason[strspn(stat->reason, " \t\n\r")] != '\0' ? stat->reason : "UNAVAILABLE By Publisher");

	free(wlcg_last_update_time_str);
	free(wlcg_state_start_time_str);
quit:
	free(jobid_str);
	free(lbhost);
	free(state_str);
	free(vo);
	return status;
}


int wlcg_send_message(thread_t *t) {
	int status = 0;
	char *command;

	// WLCG message
	if (t->dash_fd) {
		close(t->dash_fd);
		asprintf(&command, "'%s' -c '%s' '%s' '%s'", config.wlcg_binary, config.wlcg_config, config.wlcg_topic, t->dash_filename);
		lprintf(t, DBG, "calling %s", command);
		switch (vfork()) {
		case 0:
			if (execlp("/bin/sh", "/bin/sh", "-c", command, NULL) == -1) {
				lprintf(t, ERR, "can't exec '%s':%s", command, strerror(errno));
			}
			_exit(1);
			break;
		case -1:
			lprintf(t, ERR, "can't fork: %s", strerror(errno));
			break;
		default:
			break;
		}
		wait(&status);
		free(command);
		if (WIFEXITED(status)) {
			status = WEXITSTATUS(status);
			if (status) {
				lprintf(t, WRN, "%s exited with %d", config.wlcg_binary, status);
			} else {
				lprintf(t, INF, "%s exited successfully", config.wlcg_binary);
				if (!config.wlcg_no_remove) remove(t->dash_filename);
			}
		} else {
			lprintf(t, ERR, "%s not exited normally", config.wlcg_binary);
			status = -1;
		}
		free(t->dash_filename);
		t->dash_filename = NULL;
		t->dash_fd = 0;
	}

	return status;
}


void notif_free(notif_t *notif) {
	edg_wll_NotifIdFree(notif->id);
	free(notif->id_str);
	free(notif->server);
	memset(notif, 0, sizeof(notif_t));
}


void notif_invalidate(notif_t *notif) {
	edg_wll_NotifIdFree(notif->id);
	free(notif->id_str);
	notif->id = NULL;
	notif->id_str = NULL;
	notif->error = 0;
}


int notif_copy(notif_t *dest, notif_t *src) {
	if (!src || !dest) return EINVAL;
	memset(dest, 0, sizeof(notif_t));
	if (src->id) dest->id = edg_wll_NotifIdDup(src->id);
	if (src->id_str) dest->id_str = strdup(src->id_str);
	dest->type = src->type;
	if (src->server) dest->server = strdup(src->server);
	dest->port = src->port;
	dest->valid = src->valid;
	dest->refresh = src->refresh;
	dest->last_update = src->last_update;
	dest->active = src->active;
	dest->error = src->error;
	return 0;
}


#ifdef WITH_LBU_DB
static int db_init(thread_t *t, glite_lbu_DBContext *dbctx) {
	int err, dbcaps;

	if (config.dbcs) {
		if ((err = glite_lbu_InitDBContext(dbctx, GLITE_LBU_DB_BACKEND_PSQL)) != 0) {
			lprintf_dbctx(t, ERR, "can't initialize DB context");
			return err;
		}
		while ((err = glite_lbu_DBConnect(*dbctx, config.dbcs)) != 0 && !quit) {
			lprintf_dbctx(t, ERR, "can't connect to '%s'", config.dbcs);
			lprintf(t, INF, "still trying...");
			sleep(5);
		}
		if (err == 0) {
			if ((dbcaps = glite_lbu_DBQueryCaps(*dbctx)) == -1) {
				lprintf_dbctx(t, ERR, "can't get database capabilities");
				dbcaps = 0;
			}
			lprintf(t, INF, "DB connected, cs: %s, capabilities: %d", config.dbcs, dbcaps);
			if (t == NULL && (dbcaps & GLITE_LBU_DB_CAP_PREPARED) == 0) {
				lprintf(NULL, WRN, "postgresql server doesn't support SQL prepared commands, recommended version >= 8.2");
			}
			if (t) t->dbcaps = dbcaps;
			return 0;
		} else {
			glite_lbu_FreeDBContext(*dbctx);
			return err;
		}
	} else {
		lprintf(t, DBG, "no DB configured (--pg option)");
		return -1;
	}
}


static void db_free(__attribute((unused))thread_t *t, glite_lbu_DBContext dbctx) {
	if (dbctx) {
		glite_lbu_DBClose(dbctx);
		glite_lbu_FreeDBContext(dbctx);
	}
}
#endif


static notif_t *db_add_notif(char *notifid, int type, time_t valid, time_t refresh, double last_update, char *server, int port, int active) {
	void *tmp;
	notif_t *notif;

	if (db.n >= db.maxn) {
		db.maxn = db.n + 20;
		if ((tmp = realloc(db.notifs, db.maxn * sizeof(notif_t))) == NULL) return NULL;
		db.notifs = (notif_t *)tmp;
		memset(db.notifs + db.n, 0, (db.maxn - db.n) * sizeof(notif_t));
	}
	notif = db.notifs + db.n;
	notif->id_str = notifid;
	notif->type = type;
	notif->valid = valid;
	notif->refresh = refresh;
	notif->last_update = last_update;
	notif->server = server;
	notif->port = port;
	notif->active = active;
	db.n++;

	return notif;
}


static int db_save_notifs_file(thread_t *t) {
	FILE *f;
	char *filename = NULL;
	int retval = 1;
	notif_t *notif;
	int i;
	char *valid_str = NULL, *refresh_str = NULL, *last_update_str = NULL;

	asprintf(&filename, "%s-new", config.notif_file);
	if ((f = fopen(filename, "wt")) == NULL) {
		lprintf(t, ERR, "can't write '%s': %s", filename, strerror(errno));
		goto quit;
	}

	for (i = 0; i < db.n; i++) {
		notif = db.notifs + i;
		if (!notif->active) {
			lprintf(t, DBG, "not saving inactive notif %s (%s), server %s:%d", notif->id_str, rtm_notiftype2str(notif->type), notif->server, notif->port);
			continue;
		}
		if (notif->id_str) {
			rtm_time2str(notif->valid, &valid_str);
			rtm_time2str(notif->refresh, &refresh_str);
			rtm_timestamp2str(notif->last_update, &last_update_str);

			fprintf(f, "%s\t%s\t%s\t%s\t%s\n", notif->id_str, rtm_notiftype2str(notif->type), valid_str, refresh_str, last_update_str);

			free(valid_str); valid_str = NULL;
			free(refresh_str); refresh_str = NULL;
			free(last_update_str); last_update_str = NULL;
		}
	}
	fclose(f);
	if (rename(filename, config.notif_file) != 0) {
		lprintf(t, ERR, "can't move new notification file '%s' to '%s': %s", filename, config.notif_file, strerror(errno));
		goto quit;
	}
	retval = 0;
quit:
	free(filename);
	free(valid_str);
	free(refresh_str);
	free(last_update_str);
	return 0;
}


#if defined(WITH_RTM_SQL_STORAGE) && defined(WITH_LBU_DB)
static int db_save_notifs_sql(thread_t *t) {
	int retval = 1;
	notif_t *notif;
	int i;
	char *sql = NULL, *valid_str = NULL, *refresh_str = NULL, *last_update_str = NULL;
	const char *type_str;

	for (i = 0; i < db.n; i++) {
		notif = db.notifs + i;
/*
		if (!notif->active) {
			lprintf(t, INS, "not saving inactive notif %s (%s:%d)", notif->id_str, notif->server, notif->port);
			continue;
		}
*/
		type_str = rtm_notiftype2str(notif->type);
		if (notif->id_str) {
			if (notif->valid) glite_lbu_TimeToDB(db.dbctx, notif->valid, &valid_str);
			else valid_str = strdup("NULL");
			if (notif->refresh) glite_lbu_TimeToDB(db.dbctx, notif->refresh, &refresh_str);
			else refresh_str = strdup("NULL");
			if (notif->last_update) glite_lbu_TimestampToDB(db.dbctx, notif->last_update, &last_update_str);
			else last_update_str = strdup("NULL");
			trio_asprintf(&sql, "UPDATE notifs SET notifid='%|Ss', valid=%s, refresh=%s, last_update=%s WHERE lb='%|Ss' AND port=%d AND notiftype='%|Ss'", notif->id_str, valid_str, refresh_str, last_update_str, notif->server, notif->port, type_str);
			switch (glite_lbu_ExecSQL(db.dbctx, sql, NULL)) {
			case 0:
				// not found - insert
				// can be handy when using file as input of LBs
				free(sql);
				trio_asprintf(&sql, "INSERT INTO notifs (lb, port, notifid, notiftype, valid, refresh, last_update) VALUES ('%|Ss', %d, '%|Ss', '%|Ss', %s, %s, %s)", notif->server, notif->port, notif->id_str, type_str, valid_str, refresh_str, last_update_str);
				switch (glite_lbu_ExecSQL(db.dbctx, sql, NULL)) {
				case -1:
					lprintf_dbctx(t, ERR, "notif '%s' (%s) insert failed", notif->id_str, type_str);
					goto quit;
				case 0:
					lprintf(t, ERR, "notif '%s' (%s) not inserted for unknown reason", type_str);
					break;
				default:
					lprintf(t, INS, "notif '%s' (%s) inserted", notif->id_str, type_str);
					break;
				}
				break;
			case -1:
				lprintf_dbctx(t, ERR, "notif '%s' (%s) update failed", notif->id_str, type_str);
				goto quit;
			default:
				lprintf(t, INS, "notif '%s' updated", notif->id_str);
				break;
			}
		} else {
			trio_asprintf(&sql, "UPDATE notifs SET notifid=NULL, valid=NULL, refresh=NULL, last_update=NULL WHERE lb='%|Ss' AND port=%d AND notiftype='%|Ss'", notif->server, notif->port, type_str);
			switch (glite_lbu_ExecSQL(db.dbctx, sql, NULL)) {
			case 0:
				lprintf(t, INS, "cleared %s notif for %s:%d not found, ok", type_str, notif->server, notif->port);
				break;
			case -1:
				lprintf_dbctx(t, ERR, "clearing notif %s for %s:%d failed", type_str, notif->server, notif->port);
				goto quit;
			default:
				lprintf(t, INS, "cleared notif %s for %s:%d", type_str, notif->server, notif->port);
				break;
			}
		}
		free(sql); sql = NULL;
		free(valid_str); valid_str = NULL;
		free(refresh_str); refresh_str = NULL;
		free(last_update_str); last_update_str = NULL;
	}
	retval = 0;
quit:
	free(sql);
	free(valid_str);
	free(refresh_str);
	free(last_update_str);
	return 0;
}
#endif


static int db_save_notifs(thread_t *t) {
#if 0
	int i;

	for (i = 0; i < db.n; i++) {
		notif_t *notif = db.notifs + i;
		lprintf(NULL, DBG, "save: %s (%s), server: %s:%d, active: %d", notif->id_str, rtm_notiftype2str(notif->type), notif->server, notif->port, notif->active);
	}
#endif

#if defined(WITH_RTM_SQL_STORAGE) && defined(WITH_LBU_DB)
	if (!db.dbctx) return db_save_notifs_file(t);
	else return db_save_notifs_sql(t);
#else
	return db_save_notifs_file(t);
#endif
}


static notif_t *db_search_notif(notif_t *notifs, int n, const char *notifid) {
	int i;

	for (i = 0; i < n && (!notifs[i].id_str || strcmp(notifs[i].id_str, notifid) != 0); i++);
	return i == n ? NULL : notifs + i;
}


static notif_t *db_search_notif_by_server(notif_t *notifs, int n, const char *server, unsigned int port, int type) {
	int i;

	for (i = 0; i < n; i++) {
		if (strcmp(notifs[i].server, server) == 0 && notifs[i].port == port && notifs[i].type == type) break;
	}

	return i == n ? NULL : notifs + i;
}


static int db_store_change(__attribute((unused))thread_t *t, notif_t *notif, __attribute((unused))int index, edg_wll_JobStat *stat) {
	char *jobid_str = NULL, *state_str = NULL, *sql = NULL, *sql2 = NULL, *state_entered_str = NULL, *rtm_timestamp_str = NULL, *lbhost = NULL, *unique_str = NULL, *regtime_str = NULL,*vo = NULL;
	unsigned int port;

	jobid_str = stat->jobId ? glite_jobid_unparse(stat->jobId) : strdup("unknown");
	glite_jobid_getServerParts(stat->jobId, &lbhost, &port);
	unique_str = glite_jobid_getUnique(stat->jobId);
	state_str = edg_wll_StatToString(stat->state);
	vo = edg_wll_JDLField(stat,"VirtualOrganisation");
	printf(RTM_TTY_GREEN "notifid: %s (%s), jobid: %s, state: %s, vo: %s, last time: %lf" RTM_TTY_RST "\n", notif->id_str, rtm_notiftype2str(notif->type), jobid_str, state_str, vo, notif->last_update);

#ifdef WITH_LBU_DB
	if (config.dbcs && t->dbctx) {
		double state_entered, rtm_timestamp;
		char *ce, *queue, *colon, *sql_part;
		const char *rb, *ui, *state, *active, *state_changed, *lb;
		time_t registered;

		ce = stat->destination ? : "unknown";
		queue = strchr(ce, '/');
		if (queue) *queue++='\0';
		else queue = "unknown";
		colon = strchr(ce, ':');
		if (colon) colon[0] = '\0';
		rb = stat->network_server ? : "unknown";
		ui = stat->ui_host ? : "unknown";
		state = state_str ? : "unknown";
		state_entered = stat->stateEnterTime.tv_sec + stat->stateEnterTime.tv_usec / 1000000.0;
		rtm_timestamp = rtm_gettimeofday();
		registered = stat->stateEnterTimes[1 + EDG_WLL_JOB_SUBMITTED];
		lb = lbhost;
		active = "true";
		state_changed = "true";

		if ((t->dbcaps & GLITE_LBU_DB_CAP_PREPARED) == 0) {

		glite_lbu_TimestampToDB(t->dbctx, state_entered, &state_entered_str);
		glite_lbu_TimestampToDB(t->dbctx, rtm_timestamp, &rtm_timestamp_str);
		glite_lbu_TimeToDB(t->dbctx, registered, &regtime_str);

		if (vo) trio_asprintf(&sql_part, ", vo='%|Ss' ", vo);
		else sql_part = strdup("");
		trio_asprintf(&sql, "UPDATE " RTM_DB_TABLE_JOBS " SET ce='%|Ss', queue='%|Ss', rb='%|Ss', ui='%|Ss', state='%|Ss', state_entered=%s, rtm_timestamp=%s, active=%s, state_changed=%s, registered=%s%sWHERE jobid='%|Ss' AND lb='%|Ss'", ce, queue, rb, ui, state, state_entered_str, rtm_timestamp_str, active, state_changed, regtime_str, sql_part, unique_str, lb);
		free(sql_part);
		lprintf(t, INS, "update: %s", sql);
		switch (glite_lbu_ExecSQL(t->dbctx, sql, NULL)) {
		case -1:
			lprintf_dbctx(t, ERR, "can't get jobs");
			goto quit;
		case 0:
			trio_asprintf(&sql2, "INSERT INTO " RTM_DB_TABLE_JOBS " "
				"(ce, queue, rb, ui, state, state_entered, rtm_timestamp, jobid, lb, active, state_changed, registered, vo) VALUES "
				"('%|Ss', '%|Ss', '%|Ss', '%|Ss', '%|Ss', %s, %s, '%|Ss', '%|Ss', %s, %s, %s, '%|Ss')", ce, queue, rb, ui, state, state_entered_str, rtm_timestamp_str, unique_str, lb, active, state_changed, regtime_str, vo ? : "unknown");
			lprintf(t, INS, "insert: %s", sql2);
			if (glite_lbu_ExecSQL(t->dbctx, sql2, NULL) == -1) {
				lprintf_dbctx(t, ERR, "can't insert job");
				goto quit;
			}
			break;
		default:
			break;
		}

		} else { // prepared commands
			int ret;

			if (vo) {
				ret = glite_lbu_ExecPreparedStmt(t->updatecmd_vo, 13,
					GLITE_LBU_DB_TYPE_VARCHAR, ce,
					GLITE_LBU_DB_TYPE_VARCHAR, queue,
					GLITE_LBU_DB_TYPE_VARCHAR, rb,
					GLITE_LBU_DB_TYPE_VARCHAR, ui,
					GLITE_LBU_DB_TYPE_VARCHAR, state,
					GLITE_LBU_DB_TYPE_TIMESTAMP, state_entered,
					GLITE_LBU_DB_TYPE_TIMESTAMP, rtm_timestamp,
					GLITE_LBU_DB_TYPE_BOOLEAN, 1, // active
					GLITE_LBU_DB_TYPE_BOOLEAN, 1, // state_changed
					GLITE_LBU_DB_TYPE_TIMESTAMP, (double)registered,
					GLITE_LBU_DB_TYPE_VARCHAR, vo, // VO

					GLITE_LBU_DB_TYPE_VARCHAR, unique_str, // jobid
					GLITE_LBU_DB_TYPE_VARCHAR, lb // L&B server
				);
			} else {
				ret = glite_lbu_ExecPreparedStmt(t->updatecmd, 12,
					GLITE_LBU_DB_TYPE_VARCHAR, ce,
					GLITE_LBU_DB_TYPE_VARCHAR, queue,
					GLITE_LBU_DB_TYPE_VARCHAR, rb,
					GLITE_LBU_DB_TYPE_VARCHAR, ui,
					GLITE_LBU_DB_TYPE_VARCHAR, state,
					GLITE_LBU_DB_TYPE_TIMESTAMP, state_entered,
					GLITE_LBU_DB_TYPE_TIMESTAMP, rtm_timestamp,
					GLITE_LBU_DB_TYPE_BOOLEAN, 1, // active
					GLITE_LBU_DB_TYPE_BOOLEAN, 1, // state_changed
					GLITE_LBU_DB_TYPE_TIMESTAMP, (double)registered,

					GLITE_LBU_DB_TYPE_VARCHAR, unique_str, // jobid
					GLITE_LBU_DB_TYPE_VARCHAR, lb // L&B server
				);
			}

			switch (ret) {
			case -1:
				lprintf_dbctx(t, ERR, "can't update " RTM_DB_TABLE_JOBS " table");
				goto quit;
			case 0:
				if (glite_lbu_ExecPreparedStmt(t->insertcmd, 13,
					GLITE_LBU_DB_TYPE_VARCHAR, ce,
					GLITE_LBU_DB_TYPE_VARCHAR, queue,
					GLITE_LBU_DB_TYPE_VARCHAR, rb,
					GLITE_LBU_DB_TYPE_VARCHAR, ui,
					GLITE_LBU_DB_TYPE_VARCHAR, state,
					GLITE_LBU_DB_TYPE_TIMESTAMP, state_entered,
					GLITE_LBU_DB_TYPE_TIMESTAMP, rtm_timestamp,
					GLITE_LBU_DB_TYPE_VARCHAR, unique_str, // jobid
					GLITE_LBU_DB_TYPE_VARCHAR, lb, // L&B server
					GLITE_LBU_DB_TYPE_BOOLEAN, 1, // active
					GLITE_LBU_DB_TYPE_BOOLEAN, 1, // state_changed
					GLITE_LBU_DB_TYPE_TIMESTAMP, (double)registered,
					GLITE_LBU_DB_TYPE_VARCHAR, vo ? : "unknown" // VO
				) == -1) {
					lprintf_dbctx(t, ERR, "can't insert to " RTM_DB_TABLE_JOBS " table");
					goto quit;
				}
				break;
			default:
				break;
			}
		} // prepare commands

	}
#endif

	// store message
	if (config.wlcg) {
		if (wlcg_store_message(t, notif, stat) != 0) goto quit;
		if (config.wlcg_flush) wlcg_send_message(t);
	}

quit:
	free(jobid_str);
	free(state_str);
	free(sql);
	free(sql2);
	free(state_entered_str);
	free(rtm_timestamp_str);
	free(lbhost);
	free(unique_str);
	free(regtime_str);
	free(vo);

	return 0;
}


static int db_summary_getjobids(__attribute((unused))db_t *db, __attribute((unused))int maxn, __attribute((unused))char **jobids, int *n) {
/*
	switch (db->was_summary) {
	case 0:
		*n = 3;
		jobids[0] = strdup("https://skurut68-2.cesnet.cz:9000/FJldtiAR2EHC12C3Zz8WjQ");
		jobids[1] = strdup("https://skurut68-2.cesnet.cz:9000/AWTCWrUCr3uUh6cuRFaENQ");
		jobids[2] = strdup("https://skurut68-1.cesnet.cz:9000/o73CG2wrNdEQ909mG0Ac1g");
		break;
	case 1:
		*n = 1;
		jobids[0] = strdup("https://skurut68-2.cesnet.cz:9000/-46Qa2ag4gLsA_Ki-3bSLw");

		break;
	default: *n = 0; break;
	}
	db->was_summary = (db->was_summary + 1) % 3;
	return 0;
*/
	*n = 0;
	return 0;
}


static int db_summary_setinfo(__attribute((unused))db_t *db, edg_wll_JobStat *stat) {
	char *jobidstr;

	jobidstr = stat->jobId ? glite_jobid_unparse(stat->jobId) : NULL;
	printf(RTM_TTY_GREEN "summary: jobid='%s'" RTM_TTY_RST "\n", jobidstr);
	free(jobidstr);
	return 0;
}


int rtm_summary(edg_wll_Context ctx, db_t *db) {
	char *jobids[RTM_SUMMARY_JOBS];
	edg_wll_QueryRec lbquery[RTM_SUMMARY_JOBS + 1], *qr;
	const edg_wll_QueryRec *lbqueryext[2];
	edg_wll_JobStat *jobstates = NULL;
	int err = 0, ijob = 0, njobs = 0, iquery = 0, k, server_changed = 0;
	glite_jobid_t jid = NULL;
	char *server = NULL, *new_server = NULL;
	unsigned int port = 0, new_port = 0;

	lprintf(NULL, INS, "Summary");

	lbqueryext[0] = lbquery;
	lbqueryext[1] = NULL;
	memset(lbquery, 0, sizeof(lbquery));

	do {
		if (server) {

		if ((iquery >= RTM_SUMMARY_JOBS || server_changed || !njobs) && iquery) {
			if ((err = edg_wll_QueryJobsExt(ctx, lbqueryext, 0, NULL, &jobstates)) != 0) {
				lprintf_ctx(NULL, ERR, ctx, "query to '%s:%u' failed: %s", server, port, strerror(err));
				// report error jobids and skip the job (do nothing)
				// TODO
			}
			for (k = 0; k < iquery; k++) glite_jobid_free(lbquery[k].value.j);

			if (err == 0) {
				for (k = 0; jobstates[k].state != EDG_WLL_JOB_UNDEF; k++) {
					if ((err = db_summary_setinfo(db, jobstates + k)) != 0) lprintf(NULL, ERR, "Can't store %d. summary info for %s:%u", k, server, port);
					edg_wll_FreeStatus(jobstates + k);
				}
				free(jobstates);
				lprintf(NULL, DBG, "query to '%s:%u' succeed", server, port);
			}

			iquery = 0;
			memset(lbquery, 0, sizeof(lbquery));
			if (!njobs) break; // not needed, just spare summary select

			server_changed = 0;
		} else {
			lprintf(NULL, DBG, "summary pushed %d. %s\n", iquery, jobids[ijob]);
			qr = lbquery + iquery;
			iquery++;
			qr->attr = EDG_WLL_QUERY_ATTR_JOBID;
			qr->op = EDG_WLL_QUERY_OP_EQUAL;
			glite_jobid_parse(jobids[ijob], &qr->value.j);
			free(jobids[ijob]); jobids[ijob] = NULL;
			ijob++;
		}

		} // server

		if (ijob >= njobs) {
			ijob = 0;
			memset(jobids, 0, sizeof(jobids));
			njobs = 0;
			if ((err = db_summary_getjobids(db, RTM_SUMMARY_JOBS, jobids, &njobs)) != 0) {
				lprintf(NULL, ERR, "Can't get jobs for the summary");
				return err;
			}
			lprintf(NULL, DBG, "summary for %d jobs", njobs);
			if (!njobs) {
				if (iquery) continue; // do the last query
				else break;
			}
		}

		if ((err = glite_jobid_parse(jobids[ijob], &jid)) != 0) {
			lprintf(NULL, ERR, "Can't parse jobid '%s': %s", jobids[ijob], strerror(err));
			// report error jobid and skip the job
			// TODO
			glite_jobid_free(jid); jid = NULL;
			free(jobids[ijob]); jobids[ijob] = NULL;
			ijob++;
			continue;
		}
		free(new_server);
		glite_jobid_getServerParts(jid, &new_server, &new_port);
		glite_jobid_free(jid); jid = NULL;

		// first or different LB server
		if (new_server && (!server || strcmp(server, new_server) != 0 || port != new_port)) {
			if (server) server_changed = 1;

			free(server);
			server = new_server;
			port = new_port;

			new_server = NULL;
			new_port = 0;

			edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, port);
			lprintf(NULL, INF, "summary LB server '%s:%u'", server, port);
		}
	} while (njobs || iquery);

	free(server);
	free(new_server);

	return err;
}


/*
 * Updates error counter and retry times on the notification.
 *
 * On errors it lineary increases delay. Minimum delay is
 * RTM_ERROR_REPEAT_RATE, maximum is half of the configured
 * bootstrap time.
 *
 * \param t		thread context
 * \param notif		updated notification
 * \param[IN] index	notification order (for debug printing)
 * \param is_error[IN]	error state (to reset or increment error counter)
 *
 */
static int rtm_update_error_state(thread_t *t, notif_t *notif, int index, int is_error) {
	int old_error, max_count;

	old_error = notif->error;
	if (is_error) {
		if (!notif->error++) notif->refresh = time(NULL);
		max_count = config.dive / RTM_ERROR_REPEAT_RATE / 2;
		if (max_count <= 0) max_count = 1;
		notif->refresh += (notif->error <= max_count ? notif->error : max_count) * RTM_ERROR_REPEAT_RATE;
		lprintf(t, DBG, "planned to retry at %s", time2str(t, notif->refresh));
	} else {
		notif->error = 0;
	}
	if (old_error != notif->error) {
		lprintf(t, DBG, "error count of %d. server %s:%d changed from %d to %d", index, notif->server, notif->port, old_error, notif->error);
	}

	return 0;
}


/**
 * Updates notifications in persistent storage. Used to send WLCG messages too.
 *
 * \param t	thread context
 * \param[IN]	new_notif updating notification, NULL = no change in shared memory
 * \param[IN]	store     0=light (just shared memory), 1=save (flush, really store)
 * \retval 0 if OK
 */
int rtm_update_notif(thread_t *t, notif_t *new_notif, int store) {
	notif_t *notif;
	int retval = 1;

	pthread_mutex_lock(&db.lock);

	if (new_notif) {	
		if ((notif = db_search_notif_by_server(db.notifs, db.n, new_notif->server, new_notif->port, new_notif->type)) == NULL) {
			if (db_add_notif(strdup(new_notif->id_str), new_notif->type, new_notif->valid, new_notif->refresh, new_notif->last_update, strdup(new_notif->server), new_notif->port, 1) == NULL) {
				lprintf(t, ERR, "can't realloc");
				goto quit;
			}
		} else {
			notif_free(notif);
			notif_copy(notif, new_notif);
		}
	}

	wlcg_send_message(t);

	if (store) {
		if (db_save_notifs(t) != 0) goto quit;
	}
	retval = 0;
	
quit:
	pthread_mutex_unlock(&db.lock);
	return retval;
}


int rtm_drop_notif(thread_t *t, char *notifid, int store) {
	notif_t *notif;
	int retval = 1;

	pthread_mutex_lock(&db.lock);
	if ((notif = db_search_notif(db.notifs, db.n, notifid)) != NULL) {
		notif_invalidate(notif);
		if (store)
			if (db_save_notifs(t) != 0) goto quit;
	}
	retval = 0;
quit:
	pthread_mutex_unlock(&db.lock);
	return retval;
}


int load_notifs_file() {
	FILE *f;
	char *results[5];
	notif_t *new_notif;
	int err;
	char *notifidstr;
	time_t valid, refresh;
	double last_update;
	edg_wll_NotifId id;
	int type;
	int retval = 1;

	if ((f = fopen(config.notif_file, "rt")) == NULL) {
		lprintf(NULL, WRN, "WARNING: can't open notification file '%s'", config.notif_file);
		return 0;
	}

	results[0] = malloc(5 * 512);
	results[1] = results[0] + 512;
	results[2] = results[0] + 1024;
	results[3] = results[0] + 1536;
	results[4] = results[0] + 2048;
	while ((err = fscanf(f, "%511[^\t]\t%511[^\t]\t%511[^\t]\t%511[^\t]\t%511[^\t\r\n]\n", results[0], results[1], results[2], results[3], results[4])) == 5) {
		notifidstr = results[0];
		if ((type = rtm_str2notiftype(results[1])) == -1) {
			lprintf(NULL, ERR, "unknown notification type '%s' in '%s'", results[1], notifidstr);
			continue;
		}

		valid = 0;
		if (results[2] && strcasecmp(results[2], "-") != 0) {
			valid = glite_lbu_StrToTime(results[2]);
		}

		refresh = 0;
		if (results[3] && strcasecmp(results[3], "-") != 0) {
			refresh = glite_lbu_StrToTime(results[2]);
		}

		last_update = 0;
		if (results[4] && strcasecmp(results[4], "-") != 0) {
			last_update = glite_lbu_StrToTimestamp(results[4]);
		}

		if ((new_notif = db_add_notif(strdup(notifidstr), type, valid, refresh, last_update, NULL, 0, 0)) == NULL) {
			lprintf(NULL, ERR, "can't alloc");
			goto quit;
		}
		if (edg_wll_NotifIdParse(notifidstr, &id) != 0) {
			lprintf(NULL, WRN, "can't parse notification ID '%s'", notifidstr);
			notif_free(new_notif);
			db.n--;
			continue;
		}
		edg_wll_NotifIdGetServerParts(id, &new_notif->server, &new_notif->port);
		edg_wll_NotifIdFree(id);
	}
	if (err == EOF) retval = 0;
	else lprintf(NULL, ERR, "can't parse notification file '%s'", config.notif_file);
quit:
	fclose(f);
	free(results[0]);
	return retval;
}


#if defined(WITH_RTM_SQL_STORAGE) && defined(WITH_LBU_DB)
int load_notifs_sql() {
	notif_t *new_notif;
	int err;
	char *notifidstr;
	time_t valid, refresh;
	double last_update;
	edg_wll_NotifId id;
	int type;
	int retval = 1;
	glite_lbu_Statement stmt = NULL;
	char *results[5];

	if (glite_lbu_ExecSQL(db.dbctx, "SELECT notifid, notiftype, valid, refresh, last_update FROM notifs WHERE notifid IS NOT NULL", &stmt) == -1) {
		lprintf_dbctx(NULL, ERR, "fetching notification failed");
		goto quit;
	}
	while ((err = glite_lbu_FetchRow(stmt, 5, NULL, results)) > 0) {
		notifidstr = results[0];
		results[0] = NULL;
		if ((type = rtm_str2notiftype(results[1])) == -1) {
			lprintf(NULL, ERR, "unknown notification type '%s' in '%s'", results[1], notifidstr);
			free(results[1]);
			free(results[2]);
			free(results[3]);
			free(results[4]);
			continue;
		}
		free(results[1]);

		valid = 0;
		if (results[2] && results[2][0]) {
			valid = glite_lbu_DBToTime(db.dbctx, results[2]);
		}
		free(results[2]);

		refresh = 0;
		if (results[3] && results[3][0]) {
			refresh = glite_lbu_DBToTime(db.dbctx, results[3]);
		}
		free(results[3]);

		last_update = 0;
		if (results[4] && results[4][0]) {
			last_update = glite_lbu_DBToTimestamp(db.dbctx, results[4]);
		}
		free(results[4]);

		if ((new_notif = db_add_notif(notifidstr, type, valid, refresh, last_update, NULL, 0, 0)) == NULL) {
			free(notifidstr);
			lprintf(NULL, ERR, "can't alloc");
			goto quit;
		}
		if (edg_wll_NotifIdParse(notifidstr, &id) != 0) {
			lprintf(NULL, WRN, "can't parse notification IDs '%s'", notifidstr);
			notif_free(new_notif);
			db.n--;
			continue;
		}
		edg_wll_NotifIdGetServerParts(id, &new_notif->server, &new_notif->port);
		edg_wll_NotifIdFree(id);
	}
	if (err == 0) retval = 0;
	else lprintf_dbctx(NULL, ERR, "fetching failed");
quit:
	if (stmt) glite_lbu_FreeStmt(&stmt);
	return retval;
}
#endif


int load_notifs() {
	int ret;

	pthread_mutex_lock(&db.lock);

#if defined(WITH_RTM_SQL_STORAGE) && defined(WITH_LBU_DB)
	if (!db.dbctx) ret = load_notifs_file();
	else ret = load_notifs_sql();
#else
	ret = load_notifs_file();
#endif

	pthread_mutex_unlock(&db.lock);

	return ret;
}


void db_free_notifs() {
	int i;

	for (i = 0; i < db.n; i++) notif_free(db.notifs + i);
	free(db.notifs);
	db.notifs = NULL;
	db.n = db.maxn = 0;
}


void *notify_thread(void *thread_data) {
	struct sockaddr_in addr;
	int i, j, err;
	time_t now, bootstrap;
	edg_wll_NotifId notifid;
	struct timeval to;
	edg_wll_JobStat jobstat, *jobstates;
	notif_t *notif, *notif_jdl;
	edg_wll_QueryRec *conditions[3] = { NULL, NULL, NULL }, condition[2], condition2[2];
	int sock = -1, updated = 0, received = 0;
	thread_t *t = (thread_t *)thread_data;
	edg_wll_Context ctx = NULL;
	int flags = 0;

	const int	one = 1;

	lprintf(t, DBG, "thread started");

	if (!t->nservers) goto exit;

	// LB
	if (edg_wll_InitContext(&ctx) != 0) {
		lprintf(t, ERR, "can't init LB context: %s", strerror(errno));
		goto exit;
	}
	if (config.cert) edg_wll_SetParam(ctx, EDG_WLL_PARAM_X509_CERT, config.cert);
	if (config.key) edg_wll_SetParam(ctx, EDG_WLL_PARAM_X509_KEY, config.key);

	// socket
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		lprintf(t, ERR, "can't create socket: %s", strerror(errno));
		goto exit;
	}
	lprintf(t, DBG, "socket created: %d", sock);

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	if (listen_port) addr.sin_port = htons(listen_port + t->id);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (const struct sockaddr*)&addr, sizeof addr) != 0) {
		lprintf(t, ERR, "can't bind socket: %s, port = %d", strerror(errno), listen_port ? listen_port + t->id : -1);
		goto exit;
	}
	if (listen(sock, 10) != 0) {
		lprintf(t, ERR, "can't listen on socket: %s", strerror(errno));
		goto exit;
	}

#ifdef WITH_LBU_DB
	if (db_init(t, &t->dbctx) == 0)
		if ((t->dbcaps & GLITE_LBU_DB_CAP_PREPARED) != 0) {
			if (glite_lbu_PrepareStmt(t->dbctx, "INSERT INTO " DBAMP RTM_DB_TABLE_JOBS DBAMP " "
			    "(ce, queue, rb, ui, state, state_entered, rtm_timestamp, jobid, lb, active, state_changed, registered, vo)"
			    " VALUES "
			    "($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)",
			&t->insertcmd) != 0 || glite_lbu_PrepareStmt(t->dbctx, "UPDATE " DBAMP RTM_DB_TABLE_JOBS DBAMP " "
			    "SET ce=$1, queue=$2, rb=$3, ui=$4, state=$5, state_entered=$6, rtm_timestamp=$7, active=$8, state_changed=$9, registered=$10 WHERE jobid=$11 AND lb=$12", 
			&t->updatecmd) != 0 || glite_lbu_PrepareStmt(t->dbctx, "UPDATE " DBAMP RTM_DB_TABLE_JOBS DBAMP " "
			    "SET ce=$1, queue=$2, rb=$3, ui=$4, state=$5, state_entered=$6, rtm_timestamp=$7, active=$8, state_changed=$9, registered=$10, vo=$11 WHERE jobid=$12 AND lb=$13", 
			&t->updatecmd_vo) != 0) {
				lprintf_dbctx(t, ERR, "can't create prepare commands");
				lprintf(t, DBG, "insertcmd=%p, updatecmd=%p, updatecmd_vo=%p", t->insertcmd, t->updatecmd, t->updatecmd_vo);
				quit = RTM_QUIT_PRESERVE;
			}
		}
#endif

	//
	// notifications loop:
	//   - refresh/create with bootstrap
	//   - receive & store changes
	//
	while (!quit) {
		now = time(NULL);
		t->first_refresh = now + RTM_NOTIF_LOOP_MAX_TIME;
		for (i = 0; i < t->nservers; i++) {
			notif = t->notifs + i;
			if (!notif->active) {
				lprintf(t, INS, "inactive %d. notification '%s' (%s)", i, notif->id_str, rtm_notiftype2str(notif->type));
				continue;
			}
			// skip invalid LBs if not planned yet
			if (notif->error) {
				if (notif->refresh > now) {
					lprintf(t, INS, "not planned to retry previously failed %d. notification '%s' (%s), plan %s", i, notif->id_str, rtm_notiftype2str(notif->type), time2str(t, notif->refresh));
					if (t->first_refresh > notif->refresh) t->first_refresh = notif->refresh;
					continue;
				}
				lprintf(t, DBG, "retry previously failed %d. notification '%s' (%s)", i, notif->id_str, rtm_notiftype2str(notif->type));
			}
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER, notif->server);
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER_PORT, notif->port);
			now = time(NULL);
			if (!notif->valid || notif->valid - RTM_NOTIF_TTL_TO_DEAD <= now || !notif->id_str) {
			// new notification
				lprintf(t, DBG, "host %s:%d, valid %s, notifstr '%s', notifid %p", notif->server, notif->port, time2str(t, notif->valid), notif->id_str, notif->id);

				// crazy inter-notif interactions
				switch (notif->type) {
				case RTM_NOTIF_TYPE_STATUS:
					// STATUS must wait for existing JDL notification
					notif_jdl = db_search_notif_by_server(t->notifs, t->nservers, notif->server, notif->port, RTM_NOTIF_TYPE_JDL);
					if (!notif_jdl || !notif_jdl->valid || notif_jdl->valid - RTM_NOTIF_TTL_TO_DEAD <= now || !notif_jdl->id_str) {
						lprintf(t, DBG, "not created %d. notification for %s:%d (%s), waiting for %d. (JDL)", i, notif->server, notif->port, rtm_notiftype2str(notif->type), i + RTM_NOTIF_TYPE_JDL - RTM_NOTIF_TYPE_STATUS);
						// next retry of STATUS stright before the JDL
						if (notif_jdl) {
							notif->refresh = notif_jdl->refresh;
							if (t->first_refresh > notif->refresh) t->first_refresh = notif->refresh;
						}
						continue;
					}
					break;
				default:
					break;
				}
				bootstrap = notif->valid > RTM_NOTIF_TTL_TO_DEAD ? notif->valid - RTM_NOTIF_TTL_TO_DEAD : 0;
				if (config.dive > 0 && now - bootstrap > config.dive) {
					bootstrap = now - config.dive;
					lprintf(t, INS, "dive from %s:%d cut to %s (max. dive %d)", notif->server, notif->port, time2str(t, bootstrap), config.dive);
				}
				// explicitly drop old (failed) notification, if any
				if (notif->id_str) {
					if (notif->id) {
						if (edg_wll_NotifDrop(ctx, notif->id)) lprintf_ctx(t, WRN, ctx, "dropping %d. notification '%s' (%s) failed", i, notif->id_str, rtm_notiftype2str(notif->type));
					}
					// remove from the persistent storage now,
					// invalidate && update 
					rtm_drop_notif(t, notif->id_str, 1);
					// free the notification in the current thread
					notif_invalidate(notif);
					now = time(NULL);
				}
				// create the new notification
				notif->valid = now + config.ttl;

				memset(conditions, 0, sizeof(conditions));
				memset(condition, 0, sizeof(condition));
				memset(condition2, 0, sizeof(condition2));
				flags = 0;
				switch(notif->type) {
#ifndef WITH_OLD_LB
				case RTM_NOTIF_TYPE_STATUS:
					conditions[0] = condition;
					condition[0].attr = EDG_WLL_QUERY_ATTR_STATUS;
					condition[0].op = EDG_WLL_QUERY_OP_CHANGED;
					break;
				case RTM_NOTIF_TYPE_JDL:
					conditions[0] = condition;
					conditions[1] = condition2;
					condition[0].attr = EDG_WLL_QUERY_ATTR_STATUS;
					condition[0].op = EDG_WLL_QUERY_OP_EQUAL;
					condition[0].value.i = EDG_WLL_JOB_WAITING;
					condition2[0].attr = EDG_WLL_QUERY_ATTR_JDL_ATTR;
					condition2[0].op = EDG_WLL_QUERY_OP_CHANGED;
					flags = EDG_WLL_STAT_CLASSADS;
					break;
#endif
				case RTM_NOTIF_TYPE_OLD:
					flags = EDG_WLL_STAT_CLASSADS;
					break;
				case RTM_NOTIF_TYPE_DONE:
					conditions[0] = condition;
					condition[0].attr = EDG_WLL_QUERY_ATTR_STATUS;
					condition[0].op = EDG_WLL_QUERY_OP_EQUAL;
					condition[0].value.i = EDG_WLL_JOB_DONE;
					flags = EDG_WLL_STAT_CHILDREN;
					break;
				default:
					assert(notif->type != notif->type); // unknown type
					break;
				}
				if (edg_wll_NotifNew(ctx, (edg_wll_QueryRec const * const *) conditions, flags, sock, config.local_address, &notif->id, &notif->valid)) {
					memset(condition,0,sizeof condition);
					lprintf_ctx(t, ERR, ctx, "can't create notification on %s:%d", notif->server, notif->port);
					notif->valid = 0;
					notif->id = NULL;
					rtm_update_error_state(t, notif, i, 1);
					if (t->first_refresh > notif->refresh) t->first_refresh = notif->refresh;
					continue;
				} 
				notif->id_str = edg_wll_NotifIdUnparse(notif->id);
				lprintf(t, INF, "created %d. notification '%s' (%s), valid: %s", i, notif->id_str, rtm_notiftype2str(notif->type), time2str(t, notif->valid));

				// bootstrap
				memset(condition, 0, sizeof(condition));
				flags = 0;
				switch (notif->type) {
				case RTM_NOTIF_TYPE_STATUS:
					condition[0].attr = EDG_WLL_QUERY_ATTR_LASTUPDATETIME;
					condition[0].op = EDG_WLL_QUERY_OP_WITHIN;
					condition[0].value.t.tv_sec = bootstrap;
					condition[0].value2.t.tv_sec = now;
					flags = EDG_WLL_STAT_CLASSADS;
					break;
				case RTM_NOTIF_TYPE_OLD:
					break;
				case RTM_NOTIF_TYPE_JDL:
					break;
				case RTM_NOTIF_TYPE_DONE:
					break;
				default:
					assert(notif->type != notif->type); // unknown type
					break;
				}

				if (condition[0].attr) {

				lprintf(t, INF, "bootstrap %s:%d (%d), time %s..%d(now)", notif->server, notif->port, i, time2str(t, bootstrap), now);
				edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, notif->server);
				edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, notif->port);
				if ((err = edg_wll_QueryJobs(ctx, condition, flags, NULL, &jobstates)) != 0 && err != ENOENT) {
					lprintf_ctx(t, ERR, ctx, "can't bootstrap jobs on %s:%d, time %s..%d(now)", notif->server, notif->port, time2str(t, bootstrap), now);
					//
					// destroy the notification after failed bootstrap
					//
					// This error means there is something nasty on the remote LB server.
					// It could lost some messages between recreating notification,
					// so destroy this notification now.
					//
					if (edg_wll_NotifDrop(ctx, notif->id)) {
						lprintf_ctx(t, WRN, ctx, "dropping %d. notification '%s' (%s) after failed bootstrap failed", i, notif->id_str, rtm_notiftype2str(notif->type));
					} else {
						lprintf(t, INF, "dropped %d. notification '%s' (%s) after failed bootstrap", i, notif->id_str, rtm_notiftype2str(notif->type));
					}
					// free the notification instance in the current thread
					// (not propagated to the persistent storage yet)
					edg_wll_NotifIdFree(notif->id);
					notif->id = NULL;
					free(notif->id_str);
					notif->id_str = NULL;
					notif->valid = 0;
					rtm_update_error_state(t, notif, i, 1);
					if (t->first_refresh > notif->refresh) t->first_refresh = notif->refresh;
					continue;
				} else {
					for (j = 0; jobstates[j].state != EDG_WLL_JOB_UNDEF; j++) {
						notif->last_update = jobstates[j].lastUpdateTime.tv_sec + jobstates[j].lastUpdateTime.tv_usec / 1000000.0;
						db_store_change(t, notif, i, jobstates + j);
						edg_wll_FreeStatus(jobstates + j);
					}
					free(jobstates);
					lprintf(t, INF, "bootstrap %s:%d (%d), found %d jobs", notif->server, notif->port, i, j);
					rtm_update_error_state(t, notif, i, 0);
					updated = 1;
				}

				} else {
					rtm_update_error_state(t, notif, i, 0);
					updated = 1;
				}
			} else if (!notif->id) {
			// rebind existing still valid notification
				if (edg_wll_NotifIdParse(notif->id_str, &notif->id)) {
					lprintf_ctx(t, WRN, ctx, "can't parse %d. notification '%s' (%s)", i, notif->id_str, rtm_notiftype2str(notif->type));
					notif->valid = 0;
					notif->id = NULL;
					i--;
					continue;
				}
				notif->valid = now + config.ttl;
				if (edg_wll_NotifBind(ctx, notif->id, sock, config.local_address, &notif->valid)) {
					lprintf_ctx(t, WRN, ctx, "can't rebind %d. notification '%s' (%s)", i, notif->id_str, rtm_notiftype2str(notif->type));
					notif->valid = 0;
					edg_wll_NotifIdFree(notif->id);
					notif->id = NULL;
					i--;
					continue;
				}
				lprintf(t, INF, "bound %d. notification '%s' (%s)", i, notif->id_str, rtm_notiftype2str(notif->type));
				rtm_update_error_state(t, notif, i, 0);
				// no bootstrap here, reliable delivery will send changes
				updated = 1;
			} else if (!notif->refresh || notif->refresh <= now) {
			// refresh notification
				time_t valid;

				valid = now + config.ttl;
				if (edg_wll_NotifRefresh(ctx, notif->id, &valid)) {
					lprintf_ctx(t, WRN, ctx, "can't refresh %d. notification '%s' (%s), will try up to %s...", i, notif->id_str, rtm_notiftype2str(notif->type), time2str(t, notif->valid - RTM_NOTIF_TTL_TO_DEAD));
					// refresh failed, just move the refresh time...
					updated = 1;
				} else {
					notif->valid = valid;
					lprintf(t, INF, "refreshed %d. notification '%s' (%s), valid: %s", i, notif->id_str, rtm_notiftype2str(notif->type), time2str(t, notif->valid));
					rtm_update_error_state(t, notif, i, 0);
					updated = 1;
				}
			} else {
				lprintf(t, INS, "no change in %d. notification '%s' (%s)", i, notif->id_str, rtm_notiftype2str(notif->type));
			}

			if (updated) {
				assert(notif->valid);
				// create or refresh OK, bootstrap if needed OK, store the new notification
				updated = 0;
				notif->refresh = notif->valid ? (now + ((notif->valid - now) >> 1)) : 0;
				// quicker refresh (or recreate) if needed
				now = time(NULL);
				if (notif->valid && now >= notif->refresh) {
					lprintf(t, WRN, "operation not in time, refreshing/recreating the notification '%s' (%s) now", notif->id_str, rtm_notiftype2str(notif->type));
					i--;
					continue;
				}
 				rtm_update_notif(t, notif, 1);
			}

			// compute time of the next event from the new refresh on notification
	 		if (t->first_refresh > notif->refresh) t->first_refresh = notif->refresh;
		}

		// receive
		//
		// cycle here locally around NotifReceive, we know about next
		// refresh time
		//
		lprintf(t, DBG, "waiting for the notifications up to %s...", t->first_refresh ? time2str(t, t->first_refresh) : "0 (no wait)");
		while (t->first_refresh > now && !quit) {
			to.tv_sec = t->first_refresh - now;
			if (to.tv_sec > RTM_NOTIF_READ_TIMEOUT) to.tv_sec = RTM_NOTIF_READ_TIMEOUT;
			to.tv_usec = 0;
			memset(&jobstat, 0, sizeof(jobstat));
			notifid = NULL;
			err = edg_wll_NotifReceive(ctx, sock, &to, &jobstat, &notifid);
			lprintf(t, INS, "received, err=%d%s", err, err == ETIMEDOUT ? " (timeout)":"");
			if (err != 0) {
				if (err != ETIMEDOUT) {
					lprintf_ctx(t, ERR, ctx, "can't receive notifications");
					// don't cycle too quick...
					sleep(1);
				}
				// lazily refresh persistent storage here, only after timeouts
				if (received) {
					lprintf(t, DBG, "storing notification times");
 	 				rtm_update_notif(t, NULL, 1);
					received = 0;
				}
			} else {
				char *jobidstr, *notifidstr;
				double last_update;

				if (notifid) {
					jobidstr = jobstat.jobId ? glite_jobid_unparse(jobstat.jobId) : NULL;
					notifidstr = notifid ? edg_wll_NotifIdUnparse(notifid) : NULL;
					for (i = 0; i < t->nservers && (!t->notifs[i].id_str || strcmp(notifidstr, t->notifs[i].id_str) != 0); i++);
					if (i == t->nservers) {
						lprintf(t, ERR, "received notify '%s' not found", notifidstr);
					} else {
						received = 1;
						notif = t->notifs + i;
						//
						// last changed time from the arrived notification
						//
						last_update = jobstat.lastUpdateTime.tv_sec + jobstat.lastUpdateTime.tv_usec / 1000000.0;
						if (last_update > notif->last_update) notif->last_update = last_update;
						db_store_change(t, notif, i, &jobstat);
 	 					rtm_update_notif(t, notif, 0);
					}
					free(jobidstr);
					free(notifidstr);
				}
			}
			if (jobstat.state != EDG_WLL_JOB_UNDEF) edg_wll_FreeStatus(&jobstat);
			if (notifid) edg_wll_NotifIdFree(notifid);

			now = time(NULL);
		} // receive
	} // main loop

exit:	
	if (sock != -1) close(sock);
//	for (i = 0; conditions[i]; i++) free(conditions[i]);
	if (t->nservers && quit != RTM_QUIT_PRESERVE && quit != RTM_QUIT_RELOAD) {
		for (i = 0; i < t->nservers; i++) {
			if (t->notifs[i].id) {
				char *notifidstr;

				notifidstr = edg_wll_NotifIdUnparse(t->notifs[i].id);
				edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER, t->notifs[i].server);
				edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER_PORT, t->notifs[i].port);
				if (edg_wll_NotifDrop(ctx, t->notifs[i].id)) {
					lprintf_ctx(t, WRN, ctx, "can't drop %s (%s)", notifidstr, rtm_notiftype2str(t->notifs[i].type));
				} else {
					lprintf(t, INF, "notification %s (%s) dropped", notifidstr, rtm_notiftype2str(t->notifs[i].type));
				}
				rtm_drop_notif(t, t->notifs[i].id_str, 0);
				free(notifidstr);
			}
		}
		rtm_update_notif(t, NULL, 1);
	}
#ifdef WITH_LBU_DB
	if (t->insertcmd) glite_lbu_FreeStmt(&t->insertcmd);
	if (t->updatecmd) glite_lbu_FreeStmt(&t->updatecmd);
	if (t->updatecmd_vo) glite_lbu_FreeStmt(&t->updatecmd_vo);
	db_free(t, t->dbctx);
#endif
	if (ctx) edg_wll_FreeContext(ctx);
	lprintf(t, DBG, "thread ended");
	pthread_exit(NULL);
	return NULL;
}


int reconcile_config_db() {
	int i, j, n, type, typestart, typeend;
	notif_t *a, *b;
	edg_wll_Context ctx = NULL;
	edg_wll_NotifId notifid;

	if (!config.cleanup) {
		if (config.silly) {
			typestart = RTM_NOTIF_TYPE_OLD;
			typeend = RTM_NOTIF_TYPE_OLD;
		} else {
			typestart = RTM_NOTIF_TYPE_STATUS;
			typeend = RTM_NOTIF_TYPE_JDL;
		}
		n = db.n;
		for (i = 0; i < config.nservers; i++) {
			a = config.notifs + i;
			for (type = typestart; type <= typeend; type++)
			{
				b = db_search_notif_by_server(db.notifs, n, a->server, a->port, type);
				if (!b) b = db_add_notif(NULL, type, 0, 0, 0, strdup(a->server), a->port, 1);
				else lprintf(NULL, INF, "found previous notification '%s' (%s)", b->id_str, rtm_notiftype2str(b->type));
				b->active = 1;
			}
		}
	}

	if (edg_wll_InitContext(&ctx) != 0) {
		lprintf(NULL, ERR, "can't init LB context: %s", strerror(errno));
		return 1;
	}
	if (config.cert) edg_wll_SetParam(ctx, EDG_WLL_PARAM_X509_CERT, config.cert);
	if (config.key) edg_wll_SetParam(ctx, EDG_WLL_PARAM_X509_KEY, config.key);
	for (j = 0; j < db.n; j++) {
		if (!db.notifs[j].active) {
			if (db.notifs[j].id_str) {
				lprintf(NULL, INF, "dropping previous notification '%s' (%s)", db.notifs[j].id_str, rtm_notiftype2str(db.notifs[j].type));
				if (edg_wll_NotifIdParse(db.notifs[j].id_str, &notifid)) {
					lprintf(NULL, WRN, "can't parse notification ID '%s'", db.notifs[j].id_str);
					continue;
				}
				edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER, db.notifs[j].server);
				edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER_PORT, db.notifs[j].port);
				if (edg_wll_NotifDrop(ctx, notifid) != 0) {
					lprintf_ctx(NULL, WRN, ctx, "can't drop %s (%s)", db.notifs[j].id_str, rtm_notiftype2str(db.notifs[j].type));
				}
				edg_wll_NotifIdFree(notifid);
				notif_invalidate(db.notifs + j);
			}
		}
	}
	edg_wll_FreeContext(ctx);

	return db_save_notifs(NULL);
}


void usage(const char *prog) {
	fprintf(stderr, "Usage: %s [options]\n"
		"	-h, --help         display this help\n"
		"	-v, --version      display version\n"
		"	-d, --debug LEVEL  debug level (0=error,1=warn,2=info,3=debug,4=insane,\n"
		"	                   +8=not fork)\n"
		"	-D, --daemonize    daemonize\n"
		"	-i, --pidfile      the file with process ID\n"
		"	-s, --threads N    number of slave threads\n"
		"	-t, --ttl TIME     time to live (validity) of the notifications\n"
		"	                   in seconds (%d)\n"
		"	-H, --history      historic dive in seconds (<=0 is unlimited)\n"
		"	-c, --config       config file name (list of LB servers), precedence before " RTM_DB_TABLE_LBS " table\n"
		"	-n, --notifs       file for persistent information about active\n"
		"	                   notifications\n"
#ifdef WITH_LBU_DB
		"	-m, --pg           db connection string (user/pwd@server:dbname)\n"
#endif
		"	-C, --cert         X509 certificate file\n"
		"	-K, --key          X509 key file\n"
		"	-o, --old          \"silly\" mode for old L&B 3.1 servers\n"
		"	-l, --cleanup      clean up the notifications and exit\n"
		"	-w, --wlcg         enable messaging for dashboard\n"
		"	--wlcg-binary      full path to msg-publish binary\n"
		"	--wlcg-topic       topic for msg-publish\n"
		"	--wlcg-config      config file for msg-publish\n"
		"	--wlcg-flush       send message on each notification\n"
		, prog, RTM_NOTIF_TTL);
	fprintf(stderr, "\n");
	fprintf(stderr, "List of L&B servers: first it's read the config file if specified (-c option). When config file is not used and connection to database is specified, it's tried DB table " RTM_DB_TABLE_LBS ".\n");
	fprintf(stderr, "\n");
}


int config_preload(int argn, char *argv[]) {
	int opt, intval, index;
	char *err, *s;

	while ((opt = getopt_long(argn, argv, opts_line, opts, &index)) != EOF) {
		switch (opt) {
		case 'h':
		case '?':
			usage(argv[0]);
			return 1;
		case 'v':
			fprintf(stderr, "%s: %s\n", argv[0], rcsid);
			return 1;
		case 'd':
			intval = strtol(optarg, &err, 10);
			if (err && err[0]) {
				lprintf(NULL, ERR, "debug level number required");
				return 2;
			}
			config.debug = (intval & DEBUG_LEVEL_MASK);
			config.guard = !(intval & DEBUG_GUARD_MASK);
			break;
		case 'D':
			config.daemonize = 1;
			break;
		case 'i':
			config.pidfile = strdup(optarg);
			break;
		case 's':
			intval = strtol(optarg, &err, 10);
			if (err && err[0]) {
				lprintf(NULL, ERR, "number of threads required");
				return 2;
			}
			config.nthreads = intval;
			break;
		case 't':
			intval = strtol(optarg, &err, 10);
			if (err && err[0]) {
				lprintf(NULL, ERR, "requested validity in seconds required");
				return 2;
			}
			config.ttl = intval;
			break;
		case 'H':
			intval = strtol(optarg, &err, 10);
			if (err && err[0]) {
				lprintf(NULL, ERR, "historic dive in seconds required");
				return 2;
			}
			config.dive = intval;
			break;
		case 'c':
			free(config.config_file);
			config.config_file = strdup(optarg);
			break;
		case 'n':
			free(config.notif_file);
			config.notif_file = strdup(optarg);
			break;
		case 'p':
			listen_port = atoi(optarg);
			break;
		case 'm':
			free(config.dbcs);
			config.dbcs = strdup(optarg);
			break;
		case 'C':
			free(config.cert);
			config.cert = strdup(optarg);
			break;
		case 'K':
			free(config.key);
			config.key = strdup(optarg);
			break;
		case 'l':
			config.cleanup = 1;
			break;
		case 'w':
			config.wlcg = 1;
			break;
		case 'o':
			config.silly = 1;
			break;
		case 0:
			switch(index) {
			case 0:
				config.wlcg_binary = strdup(optarg);
				break;
			case 1:
				config.wlcg_config = strdup(optarg);
				break;
			case 2:
				config.wlcg_topic = strdup(optarg);
				break;
			case 3:
				config.wlcg_flush = 1;
				break;
			default:
				lprintf(NULL, ERR, "crazy option, index %d", index);
				break;
			}
			break;
		}
	}
	if (!config.notif_file) config.notif_file = strdup(RTM_FILE_NOTIFS);
	if (config.wlcg) {
		if (!config.wlcg_binary) config.wlcg_binary = strdup(WLCG_BINARY);
		if (!config.wlcg_config) config.wlcg_config = strdup(WLCG_CONFIG);
		if (!config.wlcg_topic) config.wlcg_topic = strdup(WLCG_TOPIC);
	}
#ifdef WITH_OLD_LB
	if (!config.silly) {
		lprintf(NULL, WRN, "compiled with older LB library, switching on silly mode");
		config.silly = 1;
	}
#endif

	if ((s = getenv("GLITE_LB_HARVESTER_NO_REMOVE")) != NULL) {
		if (s[0] != '0' && strcasecmp(s, "false") != 0) config.wlcg_no_remove = 1;
	}

	if (INF <= config.debug) {
		lprintf(NULL, INF, "threads: %d", config.nthreads);
		lprintf(NULL, INF, "historic dive: %d", config.dive);
		if (config.dbcs) {
			lprintf(NULL, INF, "database storage: '%s'", config.dbcs);
		} else {
			lprintf(NULL, INF, "file storage: '%s'", config.notif_file);
		}
		lprintf(NULL, INF, "WLCG messaging: %s%s", config.wlcg ? "enabled" : "disabled", config.wlcg_no_remove ? " (not removing tmp files)" : "");
		lprintf(NULL, INF, "debug level: %d", config.debug);
		lprintf(NULL, INF, "daemonize: %s", config.daemonize ? "enabled" : "disabled");
		lprintf(NULL, INF, "fork guard: %s", config.guard ? "enabled" : "disabled");
		lprintf(NULL, INF, "silly compatibility mode: %s", config.silly ? "enabled" : "disabled");
	}

	return 0;
}


int config_load() {
	char line[LINE_MAX], *port, *s;
	FILE *f;
	void *tmp;
	int i, n;
#ifdef WITH_LBU_DB
	char *results[2];
	char *result = NULL;
	glite_lbu_Statement stmt = NULL;
	int err = 0;
#endif

	if (config.config_file) {
		if ((f = fopen(config.config_file, "rt")) == NULL) {
			lprintf(NULL, ERR, "can't open config file '%s': %s", config.config_file, strerror(errno));
			return 1;
		}

		n = 10;
		while (fgets(line, sizeof(line), f) != NULL) {
			if ((s = strpbrk(line, "\n\r")) != NULL) s[0] = '\0';
			if (line[0] == '\0' || line[0] == '#') continue;
			if (config.nservers >= n || !config.notifs) {
				n = 2 * n;
				if ((tmp = (notif_t *)realloc(config.notifs, n * sizeof(notif_t))) == NULL) {
					lprintf(NULL, ERR, "insufficient memory");
					return 1;
				}
				config.notifs = tmp;
				memset(config.notifs + config.nservers, 0, (n - config.nservers) * sizeof(notif_t));
			}
			if ((port = strrchr(line, ':')) != NULL) { port[0] = '\0'; port++; }
			config.notifs[config.nservers].server = strdup(line);
			config.notifs[config.nservers++].port = (port && port[0]) ? atoi(port) : GLITE_JOBID_DEFAULT_PORT;
		}

		fclose(f);
	} else
#ifdef WITH_LBU_DB
	if (db.dbctx) {
		if ((err = glite_lbu_ExecSQL(db.dbctx, "SELECT COUNT(*) FROM " RTM_DB_TABLE_LBS, &stmt)) < 0 ||
		    (err = glite_lbu_FetchRow(stmt, 1, NULL, &result)) < 0) {
			goto err;
		}
		if (err == 0) {
			lprintf(NULL, ERR, "can't count LB servers");
			goto err;
		}
		n = atoi(result);
		free(result);
		glite_lbu_FreeStmt(&stmt);

		config.notifs = calloc(n, sizeof(notif_t));
		config.nservers = 0;
		if ((err = glite_lbu_ExecSQL(db.dbctx, "SELECT DISTINCT lb, port FROM " RTM_DB_TABLE_LBS, &stmt)) < 0) {
			goto err;
		}
		while (config.nservers < n && (err = glite_lbu_FetchRow(stmt, 2, NULL, results)) > 0) {
			config.notifs[config.nservers].server = strdup(results[0]);
			config.notifs[config.nservers++].port = atoi(results[1]);
			free(results[0]);
			free(results[1]);
		}
		if (err < 0) goto err;
		glite_lbu_FreeStmt(&stmt);
	}
#endif

	if (INF <= config.debug) {
		lprintf(NULL, INF, "servers: %d", config.nservers);
		for (i = 0; i < config.nservers; i++) lprintf(NULL, INF, "  %s:%d", config.notifs[i].server, config.notifs[i].port);
	}

	return 0;
#ifdef WITH_LBU_DB
err:
	if (err) lprintf_dbctx(NULL, ERR, "can't get LB servers");
	if (stmt) glite_lbu_FreeStmt(&stmt);
	if (result) free(result);
#endif
	return 1;
}


void config_free() {
	int i;

	for (i = 0; i < config.nservers; i++) free(config.notifs[i].server);
	free(config.config_file);
	free(config.notif_file);
	free(config.pidfile);
	free(config.dbcs);
	free(config.notifs);
	free(config.cert);
	free(config.key);
	free(config.wlcg_binary);
	free(config.wlcg_config);
	free(config.wlcg_topic);
}


// on keyboard cleanup notification, on termination signal break with
// notification preserved
void handle_signal(int num) {
	lprintf(NULL, INF, "received signal %d", num);
	switch (num) {
	case SIGINT:
	case SIGTERM:
	default:
		quit = RTM_QUIT_PRESERVE;
		break;
	}
}


int main(int argn, char *argv[]) {
	struct sigaction sa;
	sigset_t sset;
	int i, j, k, gran, mod, nnotifs;
	double t1, t2, last_summary = 0, start_time;
	thread_t *t;
	struct stat pstat;
	pid_t watched;
	int status;
	edg_wll_Context ctx = NULL;
	int retval = RTM_EXIT_ERROR;
	int cert_mtime = 0;

	// load basic configurations
	switch (config_preload(argn, argv)) {
	case 0:
		break;
	case 1:
		retval = RTM_EXIT_OK;
		goto quit_guard0;
		break;
	default:
		retval = RTM_EXIT_ERROR;
		goto quit_guard0;
	}

	// daemonize
	if (config.pidfile) {
		FILE *f;
		char s[256];

		if ((f = fopen(config.pidfile, "rt"))) {
			if (fscanf(f, "%255[^\n\r]", s) == 1) {
				if (kill(atoi(s),0)) {
					lprintf(NULL, WRN, "stale pidfile, pid = %s, pidfile '%s'", s, config.pidfile);
					rewind(f);
				}
				else {
					lprintf(NULL, ERR, "another instance running, pid = %s, pidfile '%s'", s, config.pidfile);
					fclose(f);
					goto quit_guard0;
				}
			} else {
				lprintf(NULL, ERR, "another instance possibly running, can't read pidfile '%s': %s", config.pidfile, strerror(errno));
				fclose(f);
				goto quit_guard0;
			}
		} else if (errno != ENOENT) {
			lprintf(NULL, ERR, "error opening pidfile '%s': %s", config.pidfile, strerror(errno));
			goto quit_guard0;
		}
	}
	if (config.daemonize) {
		if (daemon(0, 0) == -1) {
			lprintf(NULL, ERR, "can't daemonize: %s", strerror(errno));
			goto quit_guard0;
		}
	}

	// disable signals to the guardian
	sigemptyset(&sset);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	pthread_sigmask(SIG_BLOCK, &sset, NULL);

	if (!config.guard) {
	// not guard
		if (config.pidfile) {
			FILE *f;

			if ((f = fopen(config.pidfile, "wt")) == NULL) {
				lprintf(NULL, ERR, "can't create pidfile '%s': %s", config.pidfile, strerror(errno));
				goto quit_guard0;
			}
			fprintf(f, "%d", getpid());
			fclose(f);
		}
	} else
	// guard
	while ((watched = fork()) != 0) {
		if (watched == -1) {
			lprintf(NULL, ERR, "fork() failed: %s", strerror(errno));
			goto quit_guard;
		}
		if (config.pidfile) {
			FILE *f;

			if ((f = fopen(config.pidfile, "wt")) == NULL) {
				lprintf(NULL, ERR, "can't create pidfile '%s': %s", config.pidfile, strerror(errno));
				goto quit_guard0;
			}
			fprintf(f, "%d", watched);
			fclose(f);
		}
		if (waitpid(watched, &status, 0) == -1) {
			lprintf(NULL, ERR, "waitpid() failed: %s", strerror(errno));
			// orpaned child will restart later anyway,
			// better to end the child process just now
			kill(watched, SIGTERM);
			goto quit_guard;
		}
		if (WIFSIGNALED(status)) {
			switch (WTERMSIG(status)) {
			case SIGSEGV:
			case SIGILL:
			case SIGABRT:
#ifdef SIGBUS
			case SIGBUS:
#endif
				lprintf(NULL, ERR, "caught signal %d from process %d, resurrecting...", WTERMSIG(status), watched);
				// slow down the core generator ;-)
				// disabled signals and ended child in pidfile, live with it
				sleep(2);
				break;
			default:
				lprintf(NULL, WRN, "ended with signal %d", WTERMSIG(status));
				goto quit_guard;
			}
		} else if (WIFEXITED(status)) {
			retval = WEXITSTATUS(status);
			switch(retval) {
			case RTM_EXIT_OK:
				lprintf(NULL, INF, "exit with status %d, OK", retval);
				goto quit_guard;
			case RTM_EXIT_RELOAD:
				lprintf(NULL, INF, "exit with status %d, reloading", retval);
				break;
			default:
				lprintf(NULL, WRN, "exit with status %d, error", retval);
				goto quit_guard;
			}
		} else {
			lprintf(NULL, ERR, "unknown child status");
			goto quit_guard;
		}
	}

	// threads && Globus
	if (edg_wll_gss_initialize()) {
		lprintf(NULL, ERR, "can't initialize GSS");
		goto quit_guard;
	}

#ifndef WITH_OLD_LB
	// connection pool manually (just for tuning memory leaks)
	if (!edg_wll_initConnections()) {
		lprintf(NULL, ERR, "can't initialize LB connections");
		goto quit_guard;
	}
#endif

#ifdef WITH_LBU_DB
	// database
	switch(db_init(NULL, &db.dbctx)) {
	case 0:
		break;
	case -1:
		// no db
		break;
	default:
		// error
		goto quit;
	}
#endif

	// load configurations
	if (config_load()) goto quit;

	// load previous notifications
	if (load_notifs()) goto quit;
	// compare lb servers from configuration and notifications,
	// or clean up and exit if specified
	if (reconcile_config_db()) goto quit;
	if (config.cleanup) {
		retval = RTM_EXIT_OK;
		goto quit;
	}

	// signal handler
	sa.sa_handler = handle_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESETHAND;
	if (sigaction(SIGABRT, &sa, NULL) == -1
	    || sigaction(SIGTERM, &sa, NULL) == -1
	    || sigaction(SIGINT, &sa, NULL) == -1) {
		lprintf(NULL, ERR, "can't handle signal: %s", strerror(errno));
		goto quit;
	}
	// enable signals in main
	pthread_sigmask(SIG_UNBLOCK, &sset, NULL);

	// distribution LB servers between threads
	nnotifs = config.silly ? 1 : 2;
	threads = (thread_t *)calloc(config.nthreads, sizeof(thread_t));
	assert(db.n % nnotifs == 0); // each server RTM_NOTIF_TYPE_LAST notification types
	gran = (db.n / nnotifs) / config.nthreads, mod = (db.n / nnotifs) % config.nthreads;
	i = 0;
	j = 0;
	do {
		assert(j < config.nthreads);
		t = threads + j;
		t->nservers = nnotifs * ((j < mod) ? gran + 1 : gran);
		lprintf(NULL, DBG, "%d thread: %d notifications", j, t->nservers);
		if (t->nservers) {
			t->notifs = (notif_t *)calloc(t->nservers, sizeof(notif_t));
			for (k = 0; k < t->nservers; k++) {
				notif_copy(t->notifs + k, db.notifs + i);
				i++;
			}
		}
		j++;
	} while (i < db.n);
	// launch the threads
	for (j = 0; j < config.nthreads; j++) {
		t = threads + j;
		t->id = j;
		if (pthread_create(&threads[j].thread, NULL, notify_thread, t) != 0) {
			lprintf(NULL, ERR, "[main] can't create %d. thread: %s\n", j, strerror(errno));
			goto quit;
		}
	}

	edg_wll_InitContext(&ctx);
	if (config.cert) edg_wll_SetParam(ctx, EDG_WLL_PARAM_X509_CERT, config.cert);
	if (config.key) edg_wll_SetParam(ctx, EDG_WLL_PARAM_X509_KEY, config.key);
	last_summary = 0;
	start_time = rtm_gettimeofday();
	while (!quit) {
		t1 = rtm_gettimeofday();
		if (t1 - last_summary > RTM_SUMMARY_POLL_TIME) {
			last_summary = t1;
			rtm_summary(ctx, &db);
		}
		if (config.guard) {
			if (t1 - start_time > RTM_SUICIDE_TIME) {
				quit = RTM_QUIT_RELOAD;
				lprintf(NULL, INF, "preventive suicide");
				break;
			}
			if (config.cert) {
				if (stat(config.cert, &pstat) == 0) {
					if (!cert_mtime) cert_mtime = pstat.st_mtime;
					if (cert_mtime < pstat.st_mtime) {
						lprintf(NULL, INF, "certificate '%s' changed, reloading", config.cert);
						quit = RTM_QUIT_RELOAD;
						break;
					}
				} else {
					lprintf(NULL, ERR, "can't check certificate file '%s'", config.cert, strerror(errno));
				}
			}
		}
		t2 = rtm_gettimeofday();
		if (t2 - t1 < RTM_IDLE_POLL_TIME) usleep((RTM_IDLE_POLL_TIME + t1 - t2) * 1000000);
	}
	retval = quit == RTM_QUIT_RELOAD ? RTM_EXIT_RELOAD : RTM_EXIT_OK;
quit:
	// cleanup on error
	if (!quit) quit = RTM_QUIT_CLEANUP;
	if (threads) {
		for (i = 0; i < config.nthreads; i++) {
			t = threads + i;
			if (t->thread) pthread_join(t->thread, NULL);
			for (j = 0; j < t->nservers; j++) notif_free(t->notifs + j);
			free(t->notifs);
		}
		free(threads);
	}

	if (config.pidfile && !config.guard) {
		if (remove(config.pidfile) == -1) lprintf(NULL, WRN, "can't remove pidfile '%s': %s", config.pidfile, strerror(errno));
	}

#ifdef WITH_LBU_DB
	db_free(NULL, db.dbctx);
#endif
	edg_wll_FreeContext(ctx);
	db_free_notifs();
	config_free();
#ifndef WITH_OLD_LB
	edg_wll_poolFree();
#endif

	return retval;

quit_guard:
	if (config.pidfile) {
		if (remove(config.pidfile) == -1) lprintf(NULL, WRN, "can't remove pidfile '%s': %s", config.pidfile, strerror(errno));
	}
quit_guard0:
	config_free();
	return retval;
}
