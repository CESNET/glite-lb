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
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


/*
 *   - L/B server protocol handling routines 
 */

#include "glite/jobid/cjobid.h"
#include "glite/lb/il_string.h"
#include "glite/lb/context.h"

#include "interlogd.h"

#if defined(INTERLOGD_EMS) || (defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH))
/* 
 * Send confirmation to client.
 *
 */
int
send_confirmation(long lllid, int code)
{
  struct sockaddr_un saddr;
  char sname[256];
  int  sock, ret;

  if((sock=socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    set_error(IL_SYS, errno, "send_confirmation: error creating socket");
    return(-1);
  }

  if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
	  set_error(IL_SYS, errno, "send_confirmation: error setting socket options");
	  return(-1);
  }

  ret = 0;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sun_family = AF_UNIX;
  snprintf(sname, sizeof(sname), "/tmp/dglogd_sock_%ld", lllid);
  strcpy(saddr.sun_path, sname);
  if(connect(sock, (struct sockaddr *)&saddr, sizeof(saddr.sun_path)) < 0) {
    set_error(IL_SYS, errno, "send_confirmation: error connecting socket");
    goto out;
  }

  if(send(sock, &code, sizeof(code), MSG_NOSIGNAL) < 0) {
    set_error(IL_SYS, errno, "send_confirmation: error sending data");
    goto out;
  }
  ret = 1;

  glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
		   "  sent code %d back to client", 
		   code);

 out:
  close(sock);
  return(ret);
}


static 
int
confirm_msg(struct server_msg *msg, int code, int code_min)
{
	switch(code) {
	case LB_OK:
		code_min = 0;
		break;
	case LB_DBERR:
		/* code_min already contains apropriate error code */
		break;
	case LB_PROTO:
		code_min = EDG_WLL_IL_PROTO;
		break;
	default:
		code_min = EDG_WLL_IL_SYS;
		break;
	}
  
	return(send_confirmation(msg->receipt_to, code_min));
}
#endif



struct reader_data {
	edg_wll_GssConnection *gss;
	struct timeval *timeout;
};


static
int
gss_reader(void *user_data, char *buffer, int max_len)
{
  int ret;
  size_t len;
  struct reader_data *data = (struct reader_data *)user_data;
  edg_wll_GssStatus gss_stat;

  ret = edg_wll_gss_read_full(data->gss, buffer, max_len, data->timeout, &len, &gss_stat);
  if(ret < 0) {
    char *gss_err = NULL;

    if(ret == EDG_WLL_GSS_ERROR_GSS) {
      edg_wll_gss_get_error(&gss_stat, "get_reply", &gss_err);
      set_error(IL_DGGSS, ret, gss_err);
      free(gss_err);
    } else 
      set_error(IL_DGGSS, ret, "get_reply");
  }
  return(ret);
}


/*
 * Read reply from server.
 *  Returns: -1 - error reading message, 
 *         code > 0 - error code from server
 */
static
int 
get_reply(struct event_queue *eq, struct queue_thread *me, char **buf, int *code_min)
{
  char *msg=NULL;
  int ret, code;
  int len;
  struct timeval tv;
  struct reader_data data;

  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  data.gss = &me->gss;
  data.timeout = &tv;
  len = read_il_data(&data, &msg, gss_reader);
  if(len < 0) {
    set_error(IL_PROTO, LB_PROTO, "get_reply: error reading server reply");
    return(-1);
  }
  ret = decode_il_reply(&code, code_min, buf, msg);
  if(msg) free(msg);
  if(ret < 0) {
    set_error(IL_PROTO, LB_PROTO, "get_reply: error decoding server reply");
    return(-1);
  }
  return(code);
}



/*
 *  Returns: 0 - not connected, timeout set, 1 - OK
 */
int 
event_queue_connect(struct event_queue *eq, struct queue_thread *me)
{
  int ret;
  struct timeval tv;
  edg_wll_GssStatus gss_stat;
  cred_handle_t *local_cred_handle;

  assert(eq != NULL);
  assert(me != NULL);

#ifdef LB_PERF
  if(!nosend) {
#endif

  if(me->gss.context == NULL) {

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    /* get pointer to the credentials */
    if(pthread_mutex_lock(&cred_handle_lock) < 0)
	    abort();
    local_cred_handle = cred_handle;
    local_cred_handle->counter++;
    if(pthread_mutex_unlock(&cred_handle_lock) < 0)
	    abort();
    
    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
		     "    trying to connect to %s:%d", 
		     eq->dest_name, eq->dest_port);
#if defined(IL_NOTIFICATIONS)
    ret = edg_wll_gss_connect_name(local_cred_handle->creds, eq->dest_name, eq->dest_port, eq->owner, /* mech */NULL, &tv, &me->gss, &gss_stat);
#else
    ret = edg_wll_gss_connect(local_cred_handle->creds, eq->dest_name, eq->dest_port, &tv, &me->gss, &gss_stat);
#endif
    if(pthread_mutex_lock(&cred_handle_lock) < 0)
	    abort();
    /* check if we need to release the credentials */
    --local_cred_handle->counter;
    if(local_cred_handle != cred_handle && local_cred_handle->counter == 0) {
	    edg_wll_gss_release_cred(&local_cred_handle->creds, NULL);
	    free(local_cred_handle);
	    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "   freed credentials, not used anymore");
    }
    if(pthread_mutex_unlock(&cred_handle_lock) < 0) 
	    abort();

    if(ret < 0) {
      char *gss_err = NULL;

      if (ret == EDG_WLL_GSS_ERROR_GSS)
	 edg_wll_gss_get_error(&gss_stat, "event_queue_connect: edg_wll_gss_connect", &gss_err);
      set_error(IL_DGGSS, ret,
	        (ret == EDG_WLL_GSS_ERROR_GSS) ? gss_err : "event_queue_connect: edg_wll_gss_connect");
      if (gss_err) free(gss_err);
      me->gss.context = NULL;
      me->timeout = TIMEOUT;
      return(0);
    }
    me->first_event_sent = 0;
  }

#ifdef LB_PERF
  }
#endif

  eq->last_connected = time(NULL);
  return(1);
}


int
event_queue_close(struct event_queue *eq, struct queue_thread *me)
{
  assert(eq != NULL);
  assert(me != NULL);

#ifdef LB_PERF
  if(!nosend) {
#endif

  if(me->gss.context != NULL) {
    edg_wll_gss_close(&me->gss, NULL);
    me->gss.context = NULL;
  }
  me->first_event_sent = 0;
#ifdef LB_PERF
  }
#endif
  return(0);
}


/* 
 * Send all events from the queue.
 *   Returns: -1 - system error, 0 - not send, 1 - queue empty
 */
int 
event_queue_send(struct event_queue *eq, struct queue_thread *me)
{
  assert(eq != NULL);
  assert(me != NULL);

#ifdef LB_PERF
  if(!nosend) {
#endif
  if(me->gss.context == NULL)
    return(0);
#ifdef LB_PERF
  }
#endif

  /* feed the server with events */
  while (!event_queue_empty(eq)) {
    struct server_msg *msg;
    char *rep;
    int  ret, code, code_min;
    size_t bytes_sent;
    struct timeval tv;
    edg_wll_GssStatus gss_stat;

    clear_error();

    if(event_queue_get(eq, me, &msg) == 0) 
      return(1);

    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
		     "    trying to deliver event at offset %ld for job %s", 
		     msg->offset, msg->job_id_s);

#ifdef LB_PERF
    if(!nosend) {
#endif
	if (msg->len) {
	    tv.tv_sec = TIMEOUT;
	    tv.tv_usec = 0;
	    ret = edg_wll_gss_write_full(&me->gss, msg->msg, msg->len, &tv, &bytes_sent, &gss_stat);
	    if(ret < 0) {
	      if (ret == EDG_WLL_GSS_ERROR_ERRNO && errno == EPIPE && me->first_event_sent )
	        me->timeout = 0;
	      else
	        me->timeout = TIMEOUT;
	      server_msg_release(msg);
	      return(0);
	    }
 	    
	    if((code = get_reply(eq, me, &rep, &code_min)) < 0) {
		    /* could not get the reply properly, so try again later */
		    if (me->first_event_sent) {
			/* could be expected server connection preemption */
			clear_error();
			me->timeout = 1;
		    } else {
			me->timeout = TIMEOUT;
		        glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN, "  error reading server %s reply: %s", 
					 eq->dest_name, error_get_msg());
                    }
		    server_msg_release(msg);
		    return(0);
	    }
	}
	else { code = LB_OK; code_min = 0; rep = strdup("not sending empty message"); }
#ifdef LB_PERF
    } else {
	    glite_wll_perftest_consumeEventIlMsg(msg->msg+17);
	    code = LB_OK;
	    rep = strdup("OK");
    }
#endif
    
    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
		     "    event sent, server %s replied with %d, %s", 
		     eq->dest_name, code, rep);
    free(rep);

    /* the reply is back here */
    switch(code) {
      
	    /* NOT USED: case LB_TIME: */
    case LB_NOMEM:
	    /* NOT USED: case LB_SYS:  */
	    /* NOT USED: case LB_AUTH: */
    case LB_DBERR:
	    /* check minor code */
	    if(!(ENOENT == code_min)) {
		    /* non fatal errors (for us) */
		    me->timeout = TIMEOUT;
		    server_msg_release(msg);
		    return(0);
	    }
	
    case LB_OK:
      /* event 'succesfully' delivered */
      
    case LB_PERM:
    default: /* LB_PROTO */
      /* the event was not accepted by the server */
      /* update the event pointer */
      if(event_store_commit(msg->es, msg->ev_len, queue_list_is_log(eq), msg->generation) < 0) {
	      /* failure committing message, this is bad */
	      server_msg_release(msg);
	      return(-1);
      }
      /* if we have just delivered priority message from the queue, send confirmation */
      ret = 1;
#if defined(INTERLOGD_EMS)
      if(server_msg_is_priority(msg) &&
	 ((ret=confirm_msg(msg, code, code_min)) < 0))
	return(ret);
#endif

      if((ret == 0) &&
	 (error_get_maj() != IL_OK))
	      glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
			       "send_event: %s", 
			       error_get_msg());
	
      event_queue_remove(eq, me);
      me->first_event_sent = 1;
      eq->last_sent = time(NULL);
      break;
      
    } /* switch */
  } /* while */

  return(1);

} /* send_events */


