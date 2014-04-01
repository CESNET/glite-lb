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
jobid2eventfile(const char *job_id_s)
{
  char *buffer;

  if(job_id_s) {
    asprintf(&buffer, "%s.%s", file_prefix, job_id_s);
  } else 
    asprintf(&buffer, "%s.default", file_prefix);
    
  return(buffer);
}


static
char *
jobid2controlfile(char *job_id_s)
{
  char *buffer;
  char *hash;

  if(job_id_s) {
    asprintf(&buffer, "%s.%s.ctl", file_prefix, job_id_s);
  } else 
    asprintf(&buffer, "%s.default.ctl", file_prefix);
    
  return(buffer);
}

static
int
file_reader(void *user_data, char *buffer, const int len)
{
	size_t ret = 0;
	
	if(len > 0) {
		ret = fread(buffer, 1, len, (FILE*)user_data);
		if(ret == 0 && ferror((FILE*)user_data)) {
			return -1;
		} 
	}
	return ret;
}


static
int
read_event_string(FILE *file, il_http_message_t *msg)
{
	int  len, ret;
	int fd = fileno(file);
	long start;

	/* remember the start position */
	start = ftell(file);
	ret = receive_http(file, file_reader, msg);
	if(ret < 0) return ret;
	/* seek at the end of message in case the reader read ahead */
	len = fseek(file, start + msg->len, SEEK_SET);
	len = fgetc(file);
	if(len != '\n') {
		il_log(LOG_ERR, "error reading event from file, missing terminator character at %d, found %c(%d))\n", 
		       start+msg->len, len, len);
		if(msg->data) { free(msg->data); msg->data = NULL; }
		if(msg->host) { free(msg->host); msg->host = NULL; }
		return EINVAL;
	}
	return ret;
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
  free(es);

  return(0);
}


static
struct event_store *
event_store_create(char *job_id_s)
{
  struct event_store *es;

  es = malloc(sizeof(*es));
  if(es == NULL) {
    set_error(IL_NOMEM, ENOMEM, "event_store_create: error allocating room for structure");
    return(NULL);
  }

  memset(es, 0, sizeof(*es));

  il_log(LOG_DEBUG, "  creating event store for id %s\n", job_id_s);

  es->job_id_s = strdup(job_id_s);
  es->event_file_name = jobid2eventfile(job_id_s);
  es->control_file_name = jobid2controlfile(job_id_s);

  if(pthread_rwlock_init(&es->commit_lock, NULL)) 
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
	int num;
	char newname[MAXPATHLEN+1];

	/* find available qurantine name */
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
				set_error(IL_SYS, errno, "event_store_qurantine: error looking for qurantine filename");
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
  struct event_queue *eq_l = NULL, *eq_b = NULL;
  struct server_msg *msg;
  il_http_message_t hmsg;
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
  /* find bookkepping server queue */
  eq_b = queue_list_get(es->job_id_s);
  if(eq_b == NULL) 
    return(-1);
#endif

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

  /* check the file modification time and size to avoid unnecessary operations */
  memset(&stbuf, 0, sizeof(stbuf));
  if(fstat(fd, &stbuf) < 0) {
	  il_log(LOG_ERR, "    could not stat event file %s: %s\n", es->event_file_name, strerror(errno));
	  fclose(ef);
	  event_store_unlock(es);
	  return -1;
  } else {
	  if((es->offset == stbuf.st_size) && (es->last_modified == stbuf.st_mtime)) {
		  il_log(LOG_DEBUG, "  event file not modified since last visit, skipping\n");
		  fclose(ef);
		  event_store_unlock(es);
		  return(0);
	  }
  }

  while(1) { /* try, try, try */

	  /* get the position in file to be sought */
	  if(es->offset)
		  last = es->offset;
	  else {
		  last = es->last_committed_bs;
	  }

	  il_log(LOG_DEBUG, "    setting starting file position to  %ld\n", last);
	  il_log(LOG_DEBUG, "    bytes sent to destination: %d\n", es->last_committed_bs);

	  if(last > 0) {
		  int c;

		  /* skip all committed or already enqueued events */
		  /* be careful - check, if the offset really points to the
		     beginning of event string */
		  if(fseek(ef, last - 1, SEEK_SET) < 0) {
			  set_error(IL_SYS, errno, "event_store_recover: error setting position for read");
			  event_store_unlock(es);
			  fclose(ef);
			  return(-1);
		  }
		  /* the last enqueued event MUST end with \n */
		  if((c=fgetc(ef)) != '\n') {
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
			  return(-1);
		  }
		  break;
	  }
  }

  /* enqueue all remaining events */
  ret = 1;
  msg = NULL;
  while(read_event_string(ef, &hmsg) >= 0) {
	
    /* last holds the starting position of event_s in file */
    il_log(LOG_DEBUG, "    reading event at %ld\n", last);

    /* break from now on means there was some error */
    ret = -1;

    /* create message for server */
    msg = server_msg_create((il_octet_string_t*)&hmsg, last);
    if(msg == NULL) {
	    il_log(LOG_ALERT, "    event file corrupted! I will try to move it to quarantine (ie. rename it).\n");
	    /* actually do not bother if quarantine succeeded or not - we could not do more */
	    event_store_quarantine(es);
	    fclose(ef);
	    event_store_unlock(es);
	    return(-1);
    }
    msg->es = es;

    /* first enqueue to the LS */
    if(!bs_only && (last >= es->last_committed_ls)) {
      
	    il_log(LOG_DEBUG, "      queueing event at %ld to server %s\n", last, eq_l->dest_name);

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
       (last >= es->last_committed_bs)) {
      
	    il_log(LOG_DEBUG, "      queueing event at %ld to server %s\n", last, eq_b->dest_name);
      
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
  es->last_modified = stbuf.st_mtime;
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

	/* all events are actually read from file, the event on socket
	 * is ignored and serves just to notify us about file change
	 */
	ret = event_store_recover(es);
	ret = (ret < 0) ? ret : 0;
	return(ret);
}


int
event_store_next(struct event_store *es, long offset, int len)
{
	assert(es != NULL);
  
	/* offsets are good only to detect losses (differences between socket and file),
	   which is not possible now */
	return 0;
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

  /* the file can only be removed when all the events were successfully sent
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
	  /* XXX - in that case we can not continue because there may be
	     some undelivered events referring to that event store */
	  fclose(ef);
	  event_store_unlock(es);
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


event_store_from_file(char *filename)
{
	struct event_store *es;
	FILE *event_file;
	char *job_id_s = NULL, *p;
	il_http_message_t hmsg;
	int ret;
	
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
	ret = read_event_string(event_file, &hmsg);
	fclose(event_file);
	if(ret < 0) 
		return(0);
	
	/* get id aka dest */
	job_id_s = hmsg.host;

	il_log(LOG_DEBUG, "  message dest: '%s'\n", job_id_s);
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

	if((es->last_committed_ls == 0) &&
	   (es->last_committed_bs == 0) &&
	   (es->offset == 0)) {
		ret = event_store_read_ctl(es);
	} else 
		ret = 0;
	
	event_store_release(es);

out:
	if(hmsg.data) free(hmsg.data);
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
		  /* remove this event store */
		  (*prev) = slnext;
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

