#ident "$Header$"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/param.h>

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
	struct event_store_list *next;			// LL of event_store's
	struct event_store_list *jobid_next;	   /* double LL of rotated stores - forward */
	struct event_store_list *jobid_prev;	   /* double LL of rotated stores - backward */
};


static struct event_store_list *store_list;
static pthread_rwlock_t store_list_lock = PTHREAD_RWLOCK_INITIALIZER;


/* ----------------
 * helper functions
 * ----------------
 */
static
char *
astrcat(const char *s1, const char *s2)
{
	char *s = malloc(strlen(s1) + strlen(s2) + 1);
	if(s == NULL)
		return NULL;
	*s = 0;
	strcat(s, s1);
	strcat(s, s2);
	return s;
}


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
int
fname2index(const char *filename)
{
	char *p = rindex(filename, '.');
	char *s;

	if(p == NULL)
		return 0;

	for(s = p+1; *s != 0; s++) {
		if(*s < '0' || *s > '9') {
			return 0;
		}
	}

	return atoi(p+1);
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
  pthread_rwlock_destroy(&es->commit_lock);
  pthread_rwlock_destroy(&es->offset_lock);
  free(es);

  return(0);
}


static
struct event_store *
event_store_create(char *job_id_s, const char *filename)
{
  struct event_store *es;
  IL_EVENT_ID_T job_id;

  es = malloc(sizeof(*es));
  if(es == NULL) {
    set_error(IL_NOMEM, ENOMEM, "event_store_create: error allocating room for structure");
    return(NULL);
  }

  memset(es, 0, sizeof(*es));

  job_id = NULL;
  if(strcmp(job_id_s, "default") && IL_EVENT_ID_PARSE(job_id_s, &job_id)) {
    set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "event_store_create: error parsing id");
    free(es);
    return(NULL);
  }

  es->job_id_s = strdup(job_id_s);
  es->event_file_name = filename ? strdup(filename) : jobid2eventfile(job_id);
  es->control_file_name = filename ? astrcat(filename, ".ctl") : jobid2controlfile(job_id);
  es->rotate_index = filename ? fname2index(filename) : 0;
  IL_EVENT_ID_FREE(job_id);

  il_log(LOG_DEBUG, "  creating event store for id %s, filename %s\n", job_id_s, es->event_file_name);

  if(pthread_rwlock_init(&es->commit_lock, NULL))
          abort();
  if(pthread_rwlock_init(&es->offset_lock, NULL))
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

  if(pthread_rwlock_rdlock(&es->commit_lock))
    abort();

  return(0);
}


static
int
event_store_lock(struct event_store *es)
{
  assert(es != NULL);

  if(pthread_rwlock_wrlock(&es->commit_lock))
    abort();

  return(0);
}


static
int
event_store_unlock(struct event_store *es)
{
  assert(es != NULL);

  if(pthread_rwlock_unlock(&es->commit_lock))
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
 * event_store_qurantine()
 *   - rename damaged event store file
 *   - essentially does the same actions as cleanup, but the event store
 *     does not have to be empty
 * returns 0 on success, -1 on error
 */
static
int
event_store_quarantine(struct event_store *es)
{
	// TODO enable cleanup of quarantined event_store struct
	// TODO	handle file rotation

	int num;
	char newname[MAXPATHLEN+1];

	/* find available quarantine name */
	/* we give it at most 1024 tries */
	for(num = 0; num < 1024; num++) {
		struct stat st;

		snprintf(newname, MAXPATHLEN, "%s.quarantine.%d", es->event_file_name, num);
		newname[MAXPATHLEN] = 0;
		if(stat(newname, &st) < 0) {
			if(errno == ENOENT) {
				/* file not found */
				break;
			} else {
				/* some other error with name, probably permanent */
				set_error(IL_SYS, errno, "event_store_qurantine: error looking for quarantine filename");
				return(-1);

			}
		} else {
			/* the filename is used already */
		}
	}
	if(num >= 1024) {
		/* new name not found */
		/* XXX - is there more suitable error? */
		set_error(IL_SYS, ENOSPC, "event_store_quarantine: exhausted number of retries looking for quarantine filename");
		return(-1);
	}

	/* actually rename the file */
	il_log(LOG_DEBUG, "    renaming damaged event file from %s to %s\n",
	       es->event_file_name, newname);
	if(rename(es->event_file_name, newname) < 0) {
		set_error(IL_SYS, errno, "event_store_quarantine: error renaming event file");
		return(-1);
	}

	/* clear the counters */
	es->last_committed_ls = 0;
	es->last_committed_bs = 0;
	es->offset = 0;

	/* increase cleanup count, this will invalidate all commits from previous generation */
	es->generation++;

	return(0);
}


/*
 * event_store_rotate_file()
 * returns 0 on success, -1 on error
 */
static
int
event_store_rotate_file(struct event_store *es)
{
	int num;
	char newname[MAXPATHLEN+1];

	/* do not rotate already rotated files */
	if(es->rotate_index > 0)
		return 0;

	/* find available name */
	/* we give it at most 1024 tries */
	for(num = 0; num < 1024; num++) {
		struct stat st;

		snprintf(newname, MAXPATHLEN, "%s.%d", es->event_file_name, num);
		newname[MAXPATHLEN] = 0;
		if(stat(newname, &st) < 0) {
			if(errno == ENOENT) {
				/* file not found */
				break;
			} else {
				/* some other error with name, probably permanent */
				set_error(IL_SYS, errno, "event_store_rotate_file: error looking for available filename");
				return(-1);

			}
		} else {
			/* the filename is used already */
		}
	}
	if(num >= 1024) {
		/* new name not found */
		/* XXX - is there more suitable error? */
		set_error(IL_SYS, ENOSPC, "event_store_quarantine: exhausted number of retries looking for quarantine filename");
		return(-1);
	}

	/* actually rename the file */
	il_log(LOG_DEBUG, "    renaming too large event file from %s to %s\n",
	       es->event_file_name, newname);
	if(rename(es->event_file_name, newname) < 0) {
		set_error(IL_SYS, errno, "event_store_rotate_file: error renaming event file");
		return(-1);
	}

	/* change names in event_store */
	es->event_file_name = strdup(newname);
	es->control_file_name = astrcat(newname, ".ctl");

	return(0);
}


/*
 * event_store_recover_jobid()
 *  - recover all event stores for given jobid
 */
static
int
event_store_recover_jobid(struct event_store *es)
{
	// es is locked for use already
	struct event_store_list *p = es->le;

	do {
		event_store_recover(p->es);
		if(p != es->le ) {
			event_store_release(p->es);
		}

		if(pthread_rwlock_rdlock(&store_list_lock))
			abort();
		p = p->jobid_next;
		if(p != es->le) {
			if(pthread_rwlock_rdlock(&p->es->use_lock))
				abort();
		}
		if(pthread_rwlock_unlock(&store_list_lock))
			abort();


	} while(p != es->le);

	return 0;
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
  struct event_queue *eq_l = NULL, *eq_b = NULL;
  struct server_msg *msg;
  char *event_s;
  int fd, ret;
  long last;
  FILE *ef;
  struct flock efl;
  char err_msg[128];
  struct stat stbuf;

  assert(es != NULL);

#if defined(IL_NOTIFICATIONS)
  /* destination queue has to be found for each message separately */
#else
  /* find bookkeeping server queue */
  eq_b = queue_list_get(es->job_id_s);
  if(eq_b == NULL)
    return(-1);
#endif

#if !defined(IL_NOTIFICATIONS)
  /* get log server queue */
  eq_l = queue_list_get(NULL);
#endif

  /* lock the event_store and offset locks */
  event_store_lock(es);
  if(pthread_rwlock_wrlock(&es->offset_lock))
	  abort();

  il_log(LOG_DEBUG, "  reading events from %s\n", es->event_file_name);

  /* open event file */
  ef = fopen(es->event_file_name, "r");
  if(ef == NULL) {
	  snprintf(err_msg, sizeof(err_msg),
		   "event_store_recover: error opening event file %s",
		   es->event_file_name);
	  set_error(IL_SYS, errno, err_msg);
	  event_store_unlock(es);
	  if(pthread_rwlock_unlock(&es->offset_lock))
		  abort();
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
	  if(pthread_rwlock_unlock(&es->offset_lock))
		  abort();
	  fclose(ef);
	  return(-1);
  }

  /* check the file modification time and size to avoid unnecessary operations */
  memset(&stbuf, 0, sizeof(stbuf));
  if(fstat(fd, &stbuf) < 0) {
	  il_log(LOG_ERR, "    could not stat event file %s: %s\n", es->event_file_name, strerror(errno));
	  fclose(ef);
	  event_store_unlock(es);
	  if(pthread_rwlock_unlock(&es->offset_lock))
		  abort();
	  return -1;
  } else {
	  if((es->offset == stbuf.st_size) && (es->last_modified == stbuf.st_mtime)) {
		  il_log(LOG_DEBUG, "  event file not modified since last visit, skipping\n");
		  fclose(ef);
		  event_store_unlock(es);
		  if(pthread_rwlock_unlock(&es->offset_lock))
			  abort();
		  return(0);
	  }
  }

  /* check the file size, rename it if it is bigger than max_store_size */
  if(max_store_size > 0 && stbuf.st_size > max_store_size) {
	  event_store_rotate_file(es);
  }

  while(1) { /* try, try, try */

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
			  /* I took the liberty to optimize this,
			     since LS is not used. */
			  /* last = (es->last_committed_bs <
			     es->last_committed_ls) ? es->last_committed_bs :
			     es->last_committed_ls; */
			  last = es->last_committed_bs;
	  }

	  il_log(LOG_DEBUG, "    setting starting file position to  %ld\n", last);
	  il_log(LOG_DEBUG, "    bytes sent to logging server: %d\n", es->last_committed_ls);
	  il_log(LOG_DEBUG, "    bytes sent to bookkeeping server: %d\n", es->last_committed_bs);

	  if(last > 0) {
		  int c;

		  /* skip all committed or already enqueued events */
		  /* be careful - check, if the offset really points to the
		     beginning of event string */
		  if(fseek(ef, last-1, SEEK_SET) < 0) {
			  set_error(IL_SYS, errno, "event_store_recover: error setting position for read");
			  event_store_unlock(es);
			  fclose(ef);
			  if(pthread_rwlock_unlock(&es->offset_lock))
				  abort();
			  return(-1);
		  }
		  /* the last enqueued event MUST end with EVENT_SEPARATOR,
		     even if the offset points at EOF */
		  if((c=fgetc(ef)) != EVENT_SEPARATOR) {
			  /* Houston, we have got a problem */
			  il_log(LOG_WARNING,
				 "    file position %ld does not point at the beginning of event string, backing off!\n",
				 last);
			  /* now, where were we? */
			  if(es->offset) {
				  /* next try will be with
				     last_commited_bs */
				  es->offset = 0;
			  } else {
				  /* this is really weird... back off completely */
				  es->last_committed_ls = es->last_committed_bs = 0;
			  }
		  } else {
			  /* OK, break out of the loop */
			  break;
		  }
	  } else {
		  /* this breaks out of the loop, we are starting at
		   * the beginning of file
		   */
		  if(fseek(ef, 0, SEEK_SET) < 0) {
			  set_error(IL_SYS, errno, "event_store_recover: error setting position for read");
			  event_store_unlock(es);
			  fclose(ef);
			  if(pthread_rwlock_unlock(&es->offset_lock))
				  abort();
			  return(-1);
		  }
		  break;
	  }
  }

  /* now we have:
   *   - event file opened at position 'last'
   *   - offset and last_committed_* potentially reset to zero
   */

  /* release lock on commits, offset remains locked;
   * other threads are allowed to send/remove events, but not insert
   */
  event_store_unlock(es);

  /* enqueue all remaining events */
  ret = 1;
  msg = NULL;
  while((event_s=read_event_string(ef)) != NULL) {
    long last_ls, last_bs;

    /* last holds the starting position of event_s in file */
    il_log(LOG_DEBUG, "    reading event at %ld\n", last);

    last_ls = es->last_committed_ls;
    last_bs = es->last_committed_bs;

    /* break from now on means there was some error */
    ret = -1;

    /* create message for server */
    {
	    il_octet_string_t e;

	    e.data = event_s;
	    e.len = strlen(event_s);
	    msg = server_msg_create(&e, last);
	    free(event_s);
    }
    if(msg == NULL) {
	    il_log(LOG_ALERT, "    event file corrupted! I will try to move it to quarantine (ie. rename it).\n");
	    /* actually do not bother if quarantine succeeded or not - we could not do more */
	    event_store_quarantine(es);
	    fclose(ef);
	    if(pthread_rwlock_unlock(&es->offset_lock))
		    abort();
	    return(-1);
    }
    msg->es = es;
    msg->generation = es->generation;

    /* first enqueue to the LS */
    if(!bs_only && (last >= last_ls)) {

      il_log(LOG_DEBUG, "      queuing event at %ld to logging server\n", last);

#if !defined(IL_NOTIFICATIONS)
      if(enqueue_msg(eq_l, msg) < 0)
	break;
#endif
      }

#ifdef IL_NOTIFICATIONS
    eq_b = queue_list_get(msg->dest);
#endif

    /* now enqueue to the BS, if neccessary */
    if((eq_b != eq_l) &&
       (last >= last_bs)) {

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

  es->offset = last;
  es->last_modified = stbuf.st_mtime;
  il_log(LOG_DEBUG, "  event store offset set to %ld\n", last);

  if(msg)
    server_msg_free(msg);

  fclose(ef);
  il_log(LOG_DEBUG, "  finished reading events with %d\n", ret);

  if(pthread_rwlock_unlock(&es->offset_lock))
	  abort();

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

  /* Commented out due to the fact that offset as received on socket
   * has little to do with the real event file at the moment. The
   * event will be read from file, socket now serves only to notify
   * about possible event file change.
   */
  ret = event_store_recover_jobid(es);
  ret = (ret < 0) ? ret : 0;
  return(ret);

#if 0
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
     * 5) use_lock is in place, so no cleanup possible
      * => no one is messing with us right now */
    event_store_lock_ro(es);
    if(ret < 0)
      ret = -1;
    else
	    if(es->offset <= offset) {
		    /* Apparently there is something wrong - we are receiving an event
		     * which is beyond the end of file. Someone must have removed the file
		     * when we were not looking. The question is - what should we do with the event?
		     * We have to send it, as this is the only one occasion when we see it.
		     * However, we must not allow the es->offset to be set using this event,
		     * as it would point after the end of file. Sort this out in event_store_next().
		     */
		    ret = 1;
	    } else if(es->offset > offset) {
		    /* we have seen at least this event */
		    ret = 0;
	    }
  }
  event_store_unlock(es);
  return(ret);
#endif
}


int
event_store_next(struct event_store *es, long offset, int len)
{
  assert(es != NULL);

  /* Commented out due to the fact that offset as received on socket
   * has little to do with real event file at the moment. es->offset
   * handling is left solely to the event_store_recover().
   */

#if 0
  event_store_lock(es);
  /* Whoa, be careful now. The es->offset points right after the last enqueued event,
   * but it may not be the offset of the event WE have just enqueued, because:!
   *  1) someone could have removed the event file behind our back
   *  2) the file could have been recover()ed and more events read
   * In either case the offset should not be moved.
   */
  if(es->offset == offset) {
	  es->offset += len;
  }
  event_store_unlock(es);
#endif

  return(0);
}


/*
 * event_store_commit()
 *
 */
int
event_store_commit(struct event_store *es, int len, int ls, int generation)
{
  assert(es != NULL);

  /* do not move counters if event store with this message was cleaned up
   * (this can happen only when moving to quarantine)
   */
  /* XXX - assume int access is atomic */
  if(generation != es->generation)
	  return 0;

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

  if(fd = pthread_rwlock_wrlock(&es->offset_lock)) {
	  fprintf(stderr, "Fatal locking error: %s\n", strerror(fd));
	  abort();
  }

  /* the file can only be removed when all the events were succesfully sent
     (ie. committed both by LS and BS */
  /* That also implies that the event queues are 'empty' at the moment. */
  ef = fopen(es->event_file_name, "r+");
  if(ef == NULL) {
    /* if we can not open the event store, it is an error and the struct should be removed */
    /* XXX - is it true? */
    event_store_unlock(es);
    if(pthread_rwlock_unlock(&es->offset_lock))
	    abort();
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
    if(pthread_rwlock_unlock(&es->offset_lock))
	    abort();
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
    if(pthread_rwlock_unlock(&es->offset_lock))
	    abort();
    fclose(ef);
    return(-1);
  }

  last = ftell(ef);
  il_log(LOG_DEBUG, "    total bytes in file: %d\n", last);

  if(es->last_committed_ls < last) {
    fclose(ef);
    event_store_unlock(es);
    if(pthread_rwlock_unlock(&es->offset_lock))
	    abort();
    il_log(LOG_DEBUG, "    events still waiting in queue, cleanup aborted\n");
    return(0);
  } else if( es->last_committed_ls > last) {
	  il_log(LOG_WARNING, "  warning: event file seems to shrink!\n");
	  /* XXX - in that case we can not continue because there may be
	     some undelivered events referring to that event store */
	  fclose(ef);
	  event_store_unlock(es);
	  if(pthread_rwlock_unlock(&es->offset_lock))
		  abort();
	  return(0);
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

  /* increasing the generation count is rather pointless here, because there
     are no messages waiting in the queue that would be invalidated */
  /* es->generation++ */

  /* unlock the event_store even if it is going to be removed */
  event_store_unlock(es);
  if(pthread_rwlock_unlock(&es->offset_lock))
	  abort();

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
event_store_find(char *job_id_s, const char *filename)
{
  struct event_store_list *q, *p, *d;
  struct event_store *es;

  if(pthread_rwlock_wrlock(&store_list_lock)) {
	  abort();
  }

  es = NULL;

  d = NULL;
  p = store_list;

  while(p) {
    if(strcmp(p->es->job_id_s, job_id_s) == 0) {
		es = p->es;
	    d = p;
    	// if filename was given, compare it as well
    	if(filename == NULL || strcmp(p->es->event_file_name, filename) != 0) {
    		if(pthread_rwlock_rdlock(&es->use_lock))
    			abort();
    		if(pthread_rwlock_unlock(&store_list_lock))
    			abort();
    		return(es);
    	}
	}
    p = p->next;
  }

  // event store for given jobid and filename was not found, create one
  es = event_store_create(job_id_s, filename);
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
  p->es = es;
  p->jobid_next = p;
  p->jobid_prev = p;
  es->le = p;

  if(filename != NULL && d != NULL) {
	  // there is another event store for this jobid;
	  // 	d points to the last event store for this jobid in LL
	  // find proper place to insert new event store
	  if(p->es->rotate_index == 0) {
		  // insert behind d in LL
		  p->next = d->next;
		  d->next = p;
		  // insert behind d in jobid LL
		  p->jobid_next = d->jobid_next;
		  p->jobid_prev = d;
		  d->jobid_next->jobid_prev = p;
		  d->jobid_next = p;
	  } else {
		  struct event_store_list *r;
		  q = NULL;
		  for(r = d->jobid_next; r != d->jobid_next; r = r->jobid_next) {
			  if(p->es->rotate_index < r->es->rotate_index)
				  break;
			  if(r->es->rotate_index > 0)
				  q = r;
		  }
		  // q has the last lesser non-zero index than p
		  if(q == NULL) {
			  p->next = store_list;
			  store_list = p;
			  // insert behind d
			  p->jobid_next = d->jobid_next;
			  p->jobid_prev = d;
			  d->jobid_next->jobid_prev = p;
			  d->jobid_next = p;
		  } else {
			  p->next = q->next;
			  q->next = p;
			  // insert behind q
			  p->jobid_next = q->jobid_next;
			  p->jobid_prev = q;
			  q->jobid_next->jobid_prev = p;
			  q->jobid_next = p;
		  }
	  }
  } else {
	  // insert at the beginning
	  p->next = store_list;
	  store_list = p;
  }

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
	il_log(LOG_DEBUG, "  released lock on %s (%s)\n", es->job_id_s, es->event_file_name);
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

#endif

	il_log(LOG_INFO, "  attaching to event file: %s\n", filename);

	if(strstr(filename, "quarantine") != NULL) {
		il_log(LOG_INFO, "  file name belongs to quarantine, not touching that.\n");
		return(0);
	}

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
	edg_wll_InitContext(&context);
	ret=edg_wll_ParseNotifEvent(context, event_s, &notif_event);
	edg_wll_FreeContext(context);
	if(ret) {
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
	/*  XXX: what was that good for?
	if(notif_event->notification.dest_host &&
	   (strlen(notif_event->notification.dest_host) > 0)) {
		asprintf(&dest_name, "%s:%d", notif_event->notification.dest_host, notif_event->notification.dest_port);
	}
	*/

#else
	job_id_s = edg_wll_GetJobId(event_s);
#endif
	il_log(LOG_DEBUG, "  event id: '%s'\n", job_id_s);
	if(job_id_s == NULL) {
		il_log(LOG_NOTICE, "  skipping file, could not parse event\n");
		ret = 0;
		goto out;
	}

	es = event_store_find(job_id_s, filename);

	if(es == NULL) {
		ret = -1;
		goto out;
	}

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

    /* one more pass - this time remove stale .ctl files */
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

      /* find all control files */
      if((s=strstr(entry->d_name, ".ctl")) != NULL &&
	 s[4] == '\0') {
	      char *ef;
	      struct stat st;

	      /* is there corresponding event file? */
	      ef = malloc(strlen(dir) + strlen(entry->d_name) + 2);
	      if(ef == NULL) {
		      free(dir);
		      set_error(IL_NOMEM, ENOMEM, "event_store_init: no room for event file name");
		      return(-1);
	      }

	      s[0] = 0;
	      *ef = '\0';
	      strcat(ef, dir);
	      strcat(ef, "/");
	      strcat(ef, entry->d_name);
	      s[0] = '.';

	      if(stat(ef, &st) == 0) {
		      /* something is there */
		      /* XXX - it could be something else than event file, but do not bother now */
	      } else {
		      /* could not stat file, remove ctl */
		      strcat(ef, s);
		      il_log(LOG_DEBUG, "  removing stale file %s\n", ef);
		      if(unlink(ef))
			      il_log(LOG_ERR, "  could not remove file %s: %s\n", ef, strerror(errno));

	      }
	      free(ef);

      }
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
		  /* remove this event store from LL */
		  (*prev) = slnext;
		  /* remove this event store from jobid's LL */
		  if(sl->jobid_next != sl) {
			  sl->jobid_prev->jobid_next = sl->jobid_next;
			  sl->jobid_next->jobid_prev = sl->jobid_prev;
		  }
		  event_store_free(sl->es);
		  free(sl);
		  break;

	  case -1:
		  il_log(LOG_ERR, "  error removing event store %s (file %s):\n    %s\n",
			 sl->es->job_id_s, sl->es->event_file_name, error_get_msg());
		  /* event_store_release(sl->es); */
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

