#ifndef INTERLOGGER_P_H
#define INTERLOGGER_P_H

#ident "$Header$"

#include "il_error.h"
#include "glite/security/glite_gss.h"
#include "glite/lb/il_msg.h"

#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#define INTERLOGD_HANDLE_CMD
#define INTERLOGD_FLUSH
#define INTERLOGD_EMS

#define DEFAULT_USER "michal"
#define DEFAULT_LOG_SERVER "localhost"
#define DEFAULT_TIMEOUT 60

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#endif

#if defined(IL_NOTIFICATIONS)

#include "glite/lb/notifid.h"

#undef INTERLOGD_HANDLE_CMD
#undef INTERLOGD_FLUSH
#undef INTERLOGD_EMS
#define IL_EVENT_ID_T          edg_wll_NotifId
#define IL_EVENT_GET_UNIQUE(a) edg_wll_NotifIdGetUnique((a))
#define IL_EVENT_ID_FREE(a)    edg_wll_NotifIdFree((a))
#define IL_EVENT_ID_PARSE(a,b) edg_wll_NotifIdParse((a),(b))

#else

#define INTERLOGD_HANDLE_CMD
#define INTERLOGD_FLUSH
#define INTERLOGD_EMS
#define IL_EVENT_ID_T          edg_wlc_JobId
#define IL_EVENT_GET_UNIQUE(a) edg_wlc_JobIdGetUnique((a))
#define IL_EVENT_ID_FREE(a)    edg_wlc_JobIdFree((a))
#define IL_EVENT_ID_PARSE(a,b) edg_wlc_JobIdParse((a),(b))

#endif


#define EVENT_SEPARATOR '\n'

// #define TIMEOUT      5
extern int TIMEOUT;
#define INPUT_TIMEOUT (60)
#define EXIT_TIMEOUT (1*60)

typedef struct cred_handle {
	edg_wll_GssCred creds;
	int counter;
} cred_handle_t;
extern cred_handle_t *cred_handle;

extern pthread_mutex_t cred_handle_lock;
extern pthread_key_t cred_handle_key;
extern char *cert_file;
extern char *key_file;
extern char *CAcert_dir;
extern int bs_only;
extern int killflg;
extern int lazy_close;
extern int default_close_timeout;
extern size_t max_store_size;
extern int parallel;
#ifdef LB_PERF
extern int nosend, nosync, norecover, noparse;
#ifdef PERF_EVENTS_INLINE
extern char *event_source;
#endif
#endif

/* shared data for thread communication */
#ifdef INTERLOGD_FLUSH
extern pthread_mutex_t flush_lock;
extern pthread_cond_t flush_cond;
#endif

typedef struct {
	/* il_octet_string_t */
	int       len;
	char     *data;
	/* http message specific */
	enum { IL_HTTP_OTHER,
	       IL_HTTP_GET,
	       IL_HTTP_POST,
	       IL_HTTP_REPLY
	} msg_type;
	int       reply_code;
	char      *reply_string;
	size_t    content_length;
	char     *host;
} il_http_message_t;

/* this struct can be passed instead of il_octet_string as parameter */
typedef union {
	il_octet_string_t bin_msg;
	il_http_message_t http_msg;
} il_message_t;


struct event_store {
	char     *event_file_name;         /* file with events from local logger */
	char     *control_file_name;       /* file with control information */
	char     *job_id_s;                /* string form of the job id */
	long      last_committed_bs;       /* offset behind event that was last committed by BS */
	long      last_committed_ls;       /*  -"-                                           LS */
	long      offset;                  /* expected file position of next event */
	time_t    last_modified;           /* time of the last file modification */
	int       generation;              /* cleanup counter, scopes the offset */
	long long		  rotate_index;			   /* rotation counter */
	struct 	event_store_list *le;	   /* points back to the list */
	pthread_rwlock_t commit_lock;      /* lock to prevent simultaneous updates to last_committed_* */
	pthread_rwlock_t offset_lock;      /* lock to prevent simultaneous updates offset */
	pthread_rwlock_t use_lock;         /* lock to prevent struct deallocation */
#if defined(IL_NOTIFICATIONS)
	char     *dest;                    /* host:port destination */
#endif
};


struct server_msg {
	char                   *job_id_s;       /* necessary for commit */
	long                    offset;         /* just for printing more information to debug */
	char                   *msg;
	int                     len;
	int                     ev_len;
	struct event_store     *es;             /* cache for corresponding event store */
	int                     generation;     /* event store genereation */
	long                    receipt_to;     /* receiver (long local-logger id - LLLID) of delivery confirmation (for priority messages) */
#if defined(IL_NOTIFICATIONS)
	char                   *dest_name;
	int                     dest_port;
	char                   *dest;
#endif
	time_t                  expires;        /* time (in seconds from epoch) the message expires */
};


struct event_queue {
	edg_wll_GssConnection   gss;            /* GSS connection */
	char                   *dest_name;
	int                     dest_port;
	int                     timeout;        /* queue timeout */
	struct event_queue_msg *tail;           /* last message in the queue */
	struct event_queue_msg *head;           /* first message in the queue */
#if defined(INTERLOGD_EMS)
	struct event_queue_msg *tail_ems;       /* last priority message in the queue (or NULL) */
	struct event_queue_msg *mark_this;      /* mark message for removal */
	struct event_queue_msg *mark_prev;      /* predecessor of the marked message */
#endif
	pthread_t               thread_id;      /* id of associated thread */
	pthread_rwlock_t        update_lock;    /* mutex for queue updates */
	pthread_mutex_t         cond_lock;      /* mutex for condition variable */
	pthread_cond_t          ready_cond;     /* condition variable for message arrival */
#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
	int                     flushing;
	int                     flush_result;   /* result of flush operation */
	pthread_cond_t          flush_cond;     /* condition variable for flush operation */
#endif
	/* statistics */
	int                     times_empty;    /* number of times the queue was emptied */
	int                     max_len;        /* max queue length */
	int                     cur_len;        /* current length */
};


/* credential destructor */
void cred_handle_destroy(void *);

/* server msg methods */
struct server_msg *server_msg_create(il_octet_string_t *, long);
struct server_msg *server_msg_copy(struct server_msg *);
int server_msg_init(struct server_msg *, il_octet_string_t *);
#if defined(INTERLOGD_EMS)
int server_msg_is_priority(struct server_msg *);
#endif
int server_msg_free(struct server_msg *);

/* general event queue methods */
struct event_queue *event_queue_create(char *);
int event_queue_free(struct event_queue *);
int event_queue_empty(struct event_queue *);
int event_queue_insert(struct event_queue *, struct server_msg *);
int event_queue_get(struct event_queue *, struct server_msg **);
int event_queue_remove(struct event_queue *);
int event_queue_enqueue(struct event_queue *, char *);
/* helper */
int enqueue_msg(struct event_queue *, struct server_msg *);
int event_queue_move_events(struct event_queue *, struct event_queue *, int (*)(struct server_msg *, void *), void *);

/* protocol event queue methods */
int event_queue_connect(struct event_queue *);
int event_queue_send(struct event_queue *);
int event_queue_close(struct event_queue *);
int send_confirmation(long, int);

/* thread event queue methods */
int event_queue_create_thread(struct event_queue *);
int event_queue_lock(struct event_queue *);
int event_queue_unlock(struct event_queue *);
int event_queue_lock_ro(struct event_queue *);
int event_queue_signal(struct event_queue *);
int event_queue_wait(struct event_queue *, int);
int event_queue_sleep(struct event_queue *);
int event_queue_wakeup(struct event_queue *);
int event_queue_cond_lock(struct event_queue *);
int event_queue_cond_unlock(struct event_queue *);

/* input queue */
int input_queue_attach();
void input_queue_detach();
int input_queue_get(il_octet_string_t **, long *, int);

/* queue management functions */
int queue_list_init(char *);
struct event_queue *queue_list_get(char *);
struct event_queue *queue_list_first();
struct event_queue *queue_list_next();
int queue_list_is_log(struct event_queue *);

#if defined(IL_NOTIFICATIONS)
struct event_queue *notifid_map_get_dest(const char *);
int notifid_map_set_dest(const char *, struct event_queue *);
time_t notifid_map_get_expiration(const char *);
int notifid_map_set_expiration(const char *, time_t);
#endif

/* event store functions */
int event_store_init(char *);
int event_store_cleanup();
int event_store_recover_all(void);
struct event_store *event_store_find(char *, const char *);
int event_store_sync(struct event_store *, long);
int event_store_next(struct event_store *, long, int);
int event_store_commit(struct event_store *, int, int, int);
int event_store_recover(struct event_store *);
int event_store_release(struct event_store *);
/* int event_store_remove(struct event_store *); */

#if defined(IL_WS)
/* http functions */
int parse_header(const char *, il_http_message_t *);
int receive_http(void *, int (*)(void *, char *, const int), il_http_message_t *);
#endif

/* master main loop */
int loop();

/* recover thread */
void *recover_thread(void*);

#endif
