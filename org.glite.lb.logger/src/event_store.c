#ident "$Header$"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "glite/lb/consumer.h"
#include "glite/lb/events_parse.h"

#include "interlogd.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

static char *file_prefix = NULL;


struct event_store_list {
	struct event_store *es;
	struct event_store_list *next;
};


static struct event_store_list *store_list;
static pthread_rwlock_t store_list_lock = PTHREAD_RWLOCK_INITIALIZER;


/* ----------------
 * helper functions
 * ----------------
 */
static
char *
jobid2eventfile(IL_EVENT_ID_T job_id)
{
  char *buffer;
  char *hash;

  if(job_id) {
    hash = IL_EVENT_GET_UNIQUE(job_id);
    asprintf(&buffer, "%s.%s", file_prefix, hash);
    free(hash);
  } else 
    asprintf(&buffer, "%s.default", file_prefix);
    
  return(buffer);
}


static
char *
jobid2controlfile(IL_EVENT_ID_T job_id)
{
  char buffer[256];
  char *hash;

  if(job_id) {
    hash = IL_EVENT_GET_UNIQUE(job_id);
    snprintf(buffer, 256, "%s.%s.ctl", file_prefix, hash);
    free(hash);
  } else 
    snprintf(buffer, 256, "%s.default.ctl", file_prefix);
    
  return(strdup(buffer));
}


static
char *
read_event_string(FILE *file)
{
  char *buffer, *p, *n;
  int  len, c;

  buffer=malloc(1024);
  if(buffer == NULL) {
    set_error(IL_NOMEM, ENOMEM, "read_event_string: no room for event");
    return(NULL);
  }
  p = buffer;
  len = 1024;

  while((c=fgetc(file)) != EOF) {
    
    /* we have to have free room for one byte */
    /* if(len - (p - buffer) < 1) */
    if(p - buffer >= len) {
      n = realloc(buffer, len + 8192);
      if(n == NULL) {
	free(buffer);
	set_error(IL_NOMEM, ENOMEM, "read_event_string: no room for event");
	return(NULL);
      }
      p = p - buffer + n;
      buffer = n;
      len += 8192;
    }

    if(c == EVENT_SEPARATOR) {
      *p++ = 0;
      break;
    } else
      *p++ = (char) c; 
  }

  if(c != EVENT_SEPARATOR) {
    free(buffer);
    return(NULL);
  }

  return(buffer);
}



/* ------------------------------
 * event_store 'member' functions
 * ------------------------------
 */
static
int
event_store_free(struct event_store *es)
{
  assert(es != NULL);

  if(es->job_id_s) free(es->job_id_s);
  if(es->event_file_name) free(es->event_file_name);
  if(es->control_file_name) free(es->control_file_name);
  pthread_rwlock_destroy(&es->use_lock);
  pthread_rwlock_destroy(&es->update_lock);
  free(es);

  return(0);
}


static
struct event_store *
event_store_create(char *job_id_s)
{
  struct event_store *es;
  IL_EVENT_ID_T job_id;

  es = malloc(sizeof(*es));
  if(es == NULL) {
    set_error(IL_NOMEM, ENOMEM, "event_store_create: error allocating room for structure");
    return(NULL);
  }

  memset(es, 0, sizeof(*es));

  il_log(LOG_DEBUG, "  creating event store for id %s\n", job_id_s);

  job_id = NULL;
  if(strcmp(job_id_s, "default") && IL_EVENT_ID_PARSE(job_id_s, &job_id)) {
    set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "event_store_create: error parsing id");
    free(es);
    return(NULL);
  }

  es->job_id_s = strdup(job_id_s);
  es->event_file_name = jobid2eventfile(job_id);
  es->control_file_name = jobid2controlfile(job_id);
  IL_EVENT_ID_FREE(job_id);

  if(pthread_rwlock_init(&es->update_lock, NULL)) 
          abort();
  if(pthread_rwlock_init(&es->use_lock, NULL)) 
	  abort();

  return(es);
}


static
int
event_store_lock_ro(struct event_store *es)
{
  assert(es != NULL);

  if(pthread_rwlock_rdlock(&es->update_lock)) 
    abort();

  return(0);
}


static
int
event_store_lock(struct event_store *es)
{
  assert(es != NULL);

  if(pthread_rwlock_wrlock(&es->update_lock)) 
    abort();

  return(0);
}


static
int
event_store_unlock(struct event_store *es)
{
  assert(es != NULL);

  if(pthread_rwlock_unlock(&es->update_lock)) 
    abort();
  return(0);
}


static
int
event_store_read_ctl(struct event_store *es)
{
  FILE *ctl_file;

  assert(es != NULL);

  event_store_lock(es);
  if((ctl_file = fopen(es->control_file_name, "r")) == NULL) {
    /* no control file, new event file */
    es->last_committed_ls = 0;
    es->last_committed_bs = 0;
  } else {
    /* read last seen and last committed counts */
    fscanf(ctl_file, "%*s\n%ld\n%ld\n",
	   &es->last_committed_ls,
	   &es->last_committed_bs);
    fclose(ctl_file);
  }
  event_store_unlock(es);

  return(0);
}


static
int
event_store_write_ctl(struct event_store *es)
{
  FILE   *ctl;

  assert(es != NULL);

  ctl = fopen(es->control_file_name, "w");
  if(ctl == NULL) {
    set_error(IL_SYS, errno, "event_store_write_ctl: error opening control file");
    return(-1);
  }

  if(fprintf(ctl, "%s\n%ld\n%ld\n", 
	     es->job_id_s, 
	     es->last_committed_ls,
	     es->last_committed_bs) < 0) {
    set_error(IL_SYS, errno, "event_store_write_ctl: error writing control record");
    return(-1);
  }

  if(fclose(ctl) < 0) {
    set_error(IL_SYS, errno, "event_store_write_ctl: error closing control file");
    return(-1);
  }

  return(0);
}


/*
 * event_store_recover()
 *   - recover after restart or catch up when events missing in IPC
 *   - if offset > 0, read everything behind it
 *   - if offset == 0, read everything behind min(last_committed_bs, last_committed_es)
 */
int
event_store_recover(struct event_store *es)
{
  struct event_queue *eq_l, *eq_b;
  struct server_msg *msg;
  char *event_s;
  int fd, ret;
  long last;
  FILE *ef;
  struct flock efl;
  char err_msg[128];

  assert(es != NULL);
  
#if defined(IL_NOTIFICATIONS)
  eq_b = queue_list_get(es->dest);
#else
  /* find bookkepping server queue */
  eq_b = queue_list_get(es->job_id_s);
#endif
  if(eq_b == NULL) 
    return(-1);

#if !defined(IL_NOTIFICATIONS)
  /* get log server queue */
  eq_l = queue_list_get(NULL);
#endif

  event_store_lock(es);

  il_log(LOG_DEBUG, "  reading events from %s\n", es->event_file_name);

  /* open event file */
  ef = fopen(es->event_file_name, "r");
  if(ef == NULL) {
	  snprintf(err_msg, sizeof(err_msg), 
		   "event_store_recover: error opening event file %s",
		   es->event_file_name);
	  set_error(IL_SYS, errno, err_msg);
	  event_store_unlock(es);
	  return(-1);
  }

  /* lock the file for reading (we should not read while dglogd is writing) */
  fd = fileno(ef);
  efl.l_type = F_RDLCK;
  efl.l_whence = SEEK_SET;
  efl.l_start = 0;
  efl.l_len = 0;
  if(fcntl(fd, F_SETLKW, &efl) < 0) {
	  snprintf(err_msg, sizeof(err_msg), 
		   "event_store_recover: error locking event file %s",
		   es->event_file_name);
	  set_error(IL_SYS, errno, err_msg);
	  event_store_unlock(es);
	  fclose(ef);
	  return(-1);
  }

  /* get the position in file to be sought */
  if(es->offset)
    last = es->offset;
  else {
#if !defined(IL_NOTIFICATIONS)
    if(eq_b == eq_l) 
      last = es->last_committed_ls;
    else
#endif
      /* last = min(ls, bs) */
      last = (es->last_committed_bs < es->last_committed_ls) ? es->last_committed_bs : es->last_committed_ls;
  }

  il_log(LOG_DEBUG, "    setting starting file position to  %ld\n", last);
  il_log(LOG_DEBUG, "    bytes sent to logging server: %d\n", es->last_committed_ls);
  il_log(LOG_DEBUG, "    bytes sent to bookkeeping server: %d\n", es->last_committed_bs);

  /* skip all committed or already enqueued events */
  if(fseek(ef, last, SEEK_SET) < 0) {
    set_error(IL_SYS, errno, "event_store_recover: error setting position for read");
    event_store_unlock(es);
    fclose(ef);
    return(-1);
  }

  /* enqueue all remaining events */
  ret = 1;
  msg = NULL;
  while((event_s=read_event_string(ef)) != NULL) {
	
    /* last holds the starting position of event_s in file */
    il_log(LOG_DEBUG, "    reading event at %ld\n", last);

    /* break from now on means there was some error */
    ret = -1;

    /* create message for server */
    msg = server_msg_create(event_s);
    free(event_s);
    if(msg == NULL) {
      break;
    }
    msg->es = es;

    /* first enqueue to the LS */
    if(!bs_only && (last >= es->last_committed_ls)) {
      
      il_log(LOG_DEBUG, "      queueing event at %ld to logging server\n", last);

#if !defined(IL_NOTIFICATIONS)
      if(enqueue_msg(eq_l, msg) < 0)
	break;
#endif
      }

    /* now enqueue to the BS, if neccessary */
    if((eq_b != eq_l) && 
       (last >= es->last_committed_bs)) {
      
      il_log(LOG_DEBUG, "      queueing event at %ld to bookkeeping server\n", last);
      
      if(enqueue_msg(eq_b, msg) < 0)
	break;
    }
    server_msg_free(msg);
    msg = NULL;

    /* now last is also the offset behind the last successfully queued event */
    last = ftell(ef);

    /* ret == 0 means EOF or incomplete event found */
    ret = 0;

  } /* while */

  /* due to this little assignment we had to lock the event_store for writing */
  es->offset = last;
  il_log(LOG_DEBUG, "  event store offset set to %ld\n", last);

  if(msg) 
    server_msg_free(msg);

  fclose(ef);
  il_log(LOG_DEBUG, "  finished reading events with %d\n", ret);

  event_store_unlock(es);
  return(ret);
}


/*
 * event_store_sync()
 *   - check the position of event and fill holes from file
 *   - return 1 if the event is new,
 *            0 if it was seen before,
 *           -1 if there was an error
 */
int
event_store_sync(struct event_store *es, long offset)
{
  int ret;

  assert(es != NULL);

  event_store_lock_ro(es);
  if(es->offset == offset) 
    /* we are up to date */
    ret = 1;
  else if(es->offset > offset)
    /* we have already seen this event */
    ret = 0;
  else {
    /* es->offset < offset, i.e. we have missed some events */
    event_store_unlock(es);
    ret = event_store_recover(es);
    /* XXX possible room for intervention by another thread - is there 
     * any other thread messing with us? 
     * 1) After recover() es->offset is set at the end of file. 
     * 2) es->offset is set only by recover() and next().
     * 3) Additional recover can not do much harm.
     * 4) And next() is only called by the same thread as sync().
     * => no one is messing with us right now */
    event_store_lock_ro(es);
    if(ret < 0)
      ret = -1;
    else 
      /* somehow we suppose that now es->offset >= offset */
      /* in fact it must be es->offset > offset, anything else would be weird */
      ret = (es->offset > offset) ? 0 : 1;
  }
  event_store_unlock(es);
  return(ret);
}


int
event_store_next(struct event_store *es, int len)
{
  assert(es != NULL);
  
  event_store_lock(es);
  es->offset += len;
  event_store_unlock(es);

  return(0);
}


/* 
 * event_store_commit()
 *
 */
int
event_store_commit(struct event_store *es, int len, int ls)
{
  assert(es != NULL);

  event_store_lock(es);

  if(ls)
    es->last_committed_ls += len;
  else {
    es->last_committed_bs += len;
    if (bs_only) es->last_committed_ls += len;
  }

  if(event_store_write_ctl(es) < 0) {
    event_store_unlock(es);
    return(-1);
  }

  event_store_unlock(es);


  return(0);
}


/*
 * event_store_clean()
 *  - remove the event files (event and ctl), if they are not needed anymore
 *  - returns 0 if event_store is in use, 1 if it was removed and -1 on error
 *
 * Q: How do we know that we can safely remove the files?
 * A: When all events from file have been committed both by LS and BS.
 */
static 
int
event_store_clean(struct event_store *es)
{
  long last;
  int fd;
  FILE *ef;
  struct flock efl;

  assert(es != NULL);

  /* prevent sender threads from updating */
  event_store_lock(es);
  
  il_log(LOG_DEBUG, "  trying to cleanup event store %s\n", es->job_id_s);
  il_log(LOG_DEBUG, "    bytes sent to logging server: %d\n", es->last_committed_ls);
  il_log(LOG_DEBUG, "    bytes sent to bookkeeping server: %d\n", es->last_committed_bs);

  /* preliminary check to avoid opening event file */
  /* if the positions differ, some events still have to be sent */
  if(es->last_committed_ls != es->last_committed_bs) {
    event_store_unlock(es);
    il_log(LOG_DEBUG, "  not all events sent, cleanup aborted\n");
    return(0);
  }

  /* the file can only be removed when all the events were succesfully sent 
     (ie. committed both by LS and BS */
  /* That also implies that the event queues are 'empty' at the moment. */
  ef = fopen(es->event_file_name, "r+");
  if(ef == NULL) {
    /* if we can not open the event store, it is an error and the struct should be removed */
    /* XXX - is it true? */
    event_store_unlock(es);
    il_log(LOG_ERR,  "  event_store_clean: error opening event file: %s\n", strerror(errno));
    return(1);
  }
  
  fd = fileno(ef);
  
  /* prevent local-logger from writing into event file */
  efl.l_type = F_WRLCK;
  efl.l_whence = SEEK_SET;
  efl.l_start = 0;
  efl.l_len = 0;
  if(fcntl(fd, F_SETLK, &efl) < 0) {
    il_log(LOG_DEBUG, "    could not lock event file, cleanup aborted\n");
    fclose(ef);
    event_store_unlock(es);
    if(errno != EACCES &&
       errno != EAGAIN) {
      set_error(IL_SYS, errno, "event_store_clean: error locking event file");
      return(-1);
    }
    return(0);
  }
  
  /* now the file should not contain partially written event, so it is safe
     to get offset behind last event by seeking the end of file */
  if(fseek(ef, 0, SEEK_END) < 0) {
    set_error(IL_SYS, errno, "event_store_clean: error seeking the end of file");
    event_store_unlock(es);
    fclose(ef);
    return(-1);
  }
  
  last = ftell(ef);
  il_log(LOG_DEBUG, "    total bytes in file: %d\n", last);

  if(es->last_committed_ls < last) {
    fclose(ef);
    event_store_unlock(es);
    il_log(LOG_DEBUG, "    events still waiting in queue, cleanup aborted\n");
    return(0);
  } else if( es->last_committed_ls > last) {
	  il_log(LOG_WARNING, "  warning: event file seems to shrink!\n");
  }
  
  /* now we are sure that all events were sent and the event queues are empty */
  il_log(LOG_INFO, "    removing event file %s\n", es->event_file_name);
  
  /* remove the event file */
  unlink(es->event_file_name);
  unlink(es->control_file_name);
  
  /* clear the counters */
  es->last_committed_ls = 0;
  es->last_committed_bs = 0;
  es->offset = 0;

  /* unlock the event_store even if it is going to be removed */
  event_store_unlock(es);

  /* close the event file (that unlocks it as well) */
  fclose(ef);

  /* indicate that it is safe to remove this event_store */
  return(1);
}



/* --------------------------------
 * event store management functions
 * --------------------------------
 */
struct event_store *
event_store_find(char *job_id_s)
{
  struct event_store_list *q, *p;
  struct event_store *es;

  if(pthread_rwlock_wrlock(&store_list_lock)) {
	  abort();
  }

  es = NULL;
  
  q = NULL;
  p = store_list;
  
  while(p) {
    if(strcmp(p->es->job_id_s, job_id_s) == 0) {
      es = p->es;
      if(pthread_rwlock_rdlock(&es->use_lock))
	      abort();
      if(pthread_rwlock_unlock(&store_list_lock)) 
	      abort();
      return(es);
    }

    q = p;
    p = p->next;
  }

  es = event_store_create(job_id_s);
  if(es == NULL) {
	  if(pthread_rwlock_unlock(&store_list_lock)) 
		  abort();
	  return(NULL);
  }

  p = malloc(sizeof(*p));
  if(p == NULL) {
    set_error(IL_NOMEM, ENOMEM, "event_store_find: no room for new event store");
      if(pthread_rwlock_unlock(&store_list_lock)) 
	      abort();
    return(NULL);
  }
  
  p->next = store_list;
  store_list = p;
    
  p->es = es;

  if(pthread_rwlock_rdlock(&es->use_lock))
	  abort();

  if(pthread_rwlock_unlock(&store_list_lock)) 
	  abort();

  return(es);
}


int
event_store_release(struct event_store *es)
{
	assert(es != NULL);

	if(pthread_rwlock_unlock(&es->use_lock))
		abort();
	il_log(LOG_DEBUG, "  released lock on %s\n", es->job_id_s);
	return(0);
}


static
int
event_store_from_file(char *filename)
{
	struct event_store *es;
	FILE *event_file;
	char *event_s, *job_id_s = NULL;
	int ret;
#if defined(IL_NOTIFICATIONS)
	edg_wll_Event *notif_event;
	edg_wll_Context context;
	char *dest_name = NULL;

	edg_wll_InitContext(&context);
#endif
	
	il_log(LOG_INFO, "  attaching to event file: %s\n", filename);
	
	event_file = fopen(filename, "r");
	if(event_file == NULL) {
		set_error(IL_SYS, errno, "event_store_from_file: error opening event file");
		return(-1);
	}
	event_s = read_event_string(event_file);
	fclose(event_file);
	if(event_s == NULL) 
		return(0);
	
#if defined(IL_NOTIFICATIONS)
	if((ret=edg_wll_ParseNotifEvent(context, event_s, &notif_event))) {
		set_error(IL_LBAPI, ret, "event_store_from_file: could not parse event");
		ret = -1;
		goto out;
	}
	if(notif_event->notification.notifId == NULL) {
		set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, 
			  "event_store_from_file: parse error - no notif id");
		ret = -1;
		goto out;
	}
	if((job_id_s = edg_wll_NotifIdUnparse(notif_event->notification.notifId)) == NULL) {
		set_error(IL_SYS, ENOMEM, "event_store_from_file: could not copy id");
		ret = -1;
		goto out;
	}
	if(notif_event->notification.dest_host && 
	   (strlen(notif_event->notification.dest_host) > 0)) {
		asprintf(&dest_name, "%s:%d", notif_event->notification.dest_host, notif_event->notification.dest_port);
	}
	
#else
	job_id_s = edg_wll_GetJobId(event_s);
#endif
	il_log(LOG_DEBUG, "  event id: '%s'\n", job_id_s);
	if(job_id_s == NULL) {
		il_log(LOG_NOTICE, "  skipping file, could not parse event\n");
		ret = 0;
		goto out;
	}
	
	es=event_store_find(job_id_s);
	
	if(es == NULL) {
		ret = -1;
		goto out;
	}

#if defined(IL_NOTIFICATIONS)
	es->dest = dest_name;
#endif

	if((es->last_committed_ls == 0) &&
	   (es->last_committed_bs == 0) &&
	   (es->offset == 0)) {
		ret = event_store_read_ctl(es);
	} else 
		ret = 0;
	
	event_store_release(es);

out:
#if defined(IL_NOTIFICATIONS)
	if(notif_event) {
		edg_wll_FreeEvent(notif_event);
		free(notif_event);
	}
#endif
	if(event_s) free(event_s); 
	if(job_id_s) free(job_id_s);
	return(ret);
}


int
event_store_init(char *prefix)
{
  if(file_prefix == NULL) {
    file_prefix = strdup(prefix);
    store_list = NULL;
  }

  /* read directory and get a list of event files */
  {
    int len;

    char *p, *dir;
    DIR *event_dir;
    struct dirent *entry;


    /* get directory name */
    p = strrchr(file_prefix, '/');
    if(p == NULL) {
      dir = strdup(".");
      p = "";
      len = 0;
    } else {
      *p = '\0';
      dir = strdup(file_prefix);
      *p++ = '/';
      len = strlen(p);
    }

    event_dir = opendir(dir);
    if(event_dir == NULL) {
      free(dir);
      set_error(IL_SYS, errno, "event_store_init: error opening event directory");
      return(-1);
    }
    
    while((entry=readdir(event_dir))) {
      char *s;

      /* skip all files that do not match prefix */
      if(strncmp(entry->d_name, p, len) != 0) 
	continue;

      /* skip all control files */
      if((s=strstr(entry->d_name, ".ctl")) != NULL &&
	 s[4] == '\0')
	continue;

      s = malloc(strlen(dir) + strlen(entry->d_name) + 2);
      if(s == NULL) {
	free(dir);
	set_error(IL_NOMEM, ENOMEM, "event_store_init: no room for file name");
	return(-1);
      }

      *s = '\0';
      strcat(s, dir);
      strcat(s, "/");
      strcat(s, entry->d_name);

      if(event_store_from_file(s) < 0) {
	free(dir);
	free(s);
	closedir(event_dir);
	return(-1);
      }

      free(s);
    }
    closedir(event_dir);
    free(dir);
  }

  return(0);
}


int
event_store_recover_all()
{
  struct event_store_list *sl;


  if(pthread_rwlock_rdlock(&store_list_lock)) 
	  abort();

  /* recover all event stores */
  sl = store_list;
  while(sl != NULL) {

	  /* recover this event store */
	  /* no need to lock use_lock in event_store, the store_list_lock is in place */
	  if(event_store_recover(sl->es) < 0) {
		  il_log(LOG_ERR, "  error recovering event store %s:\n    %s\n", sl->es->event_file_name, error_get_msg());
		  clear_error();
	  }
	  sl = sl->next;
  }
  
  if(pthread_rwlock_unlock(&store_list_lock)) 
	  abort();

  return(0);
}


#if 0 
int
event_store_remove(struct event_store *es)
{
  struct event_store_list *p, **q;

  assert(es != NULL);

  switch(event_store_clean(es)) {
  case 0:
    il_log(LOG_DEBUG, "  event store not removed, still used\n");
    return(0);
    
  case 1:
    if(pthread_rwlock_wrlock(&store_list_lock) < 0) {
      set_error(IL_SYS, errno, "  event_store_remove: error locking event store list");
      return(-1);
    }

    p = store_list;
    q = &store_list;

    while(p) {
      if(p->es == es) {
	(*q) = p->next;
	event_store_free(es);
	free(p);
	break;
      }
      q = &(p->next);
      p = p->next;
    }

    if(pthread_rwlock_unlock(&store_list_lock) < 0) {
      set_error(IL_SYS, errno, "  event_store_remove: error unlocking event store list");
      return(-1);
    }
    return(1);

  default:
    return(-1);
  }
  /* not reached */
  return(0);
}
#endif

int
event_store_cleanup()
{
  struct event_store_list *sl;
  struct event_store_list *slnext;
  struct event_store_list **prev;

  /* try to remove event files */

  if(pthread_rwlock_wrlock(&store_list_lock)) 
	  abort();

  sl = store_list;
  prev = &store_list;

  while(sl != NULL) {
	  int ret;

	  slnext = sl->next;
	  
	  /* one event store at time */
	  ret = pthread_rwlock_trywrlock(&sl->es->use_lock);
	  if(ret == EBUSY) {
		  il_log(LOG_DEBUG, "  event_store %s is in use by another thread\n", 
			 sl->es->job_id_s);
		  sl = slnext;
		  continue;
	  } else if (ret < 0)
	    abort();

	  switch(event_store_clean(sl->es)) {
		  
	  case 1:
		  /* remove this event store */
		  (*prev) = slnext;
		  event_store_free(sl->es);
		  free(sl);
		  break;
		  
	  case -1:
		  il_log(LOG_ERR, "  error removing event store %s (file %s):\n    %s\n", 
			 sl->es->job_id_s, sl->es->event_file_name, error_get_msg());
		  event_store_release(sl->es);
		  clear_error();
		  /* go on to the next */
		  
	  default:
		  event_store_release(sl->es);
		  prev = &(sl->next);
		  break;
	  }
	  
	  sl = slnext;
  }
  
  if(pthread_rwlock_unlock(&store_list_lock)) 
	  abort();
  
  return(0);
}

