#ident "$Header$"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
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

struct reader_data {
	edg_wll_GssConnection *gss;
	struct timeval *timeout;
};


static
int
gss_reader(void *user_data, char *buffer, int max_len)
{
  int ret;
  struct reader_data *data = (struct reader_data *)user_data;
  edg_wll_GssStatus gss_stat;

  ret = edg_wll_gss_read(data->gss, buffer, max_len, data->timeout, &gss_stat);
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
 *         code > 0 - http status code from server
 */
static
int 
get_reply(struct event_queue *eq, char **buf, int *code_min)
{
  int ret, code;
  int len;
  struct timeval tv;
  struct reader_data data;
  il_http_message_t msg;

  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  data.gss = &eq->gss;
  data.timeout = &tv;
  len = receive_http(&data, gss_reader, &msg);
  if(len < 0) {
    set_error(IL_PROTO, LB_PROTO, "get_reply: error reading server reply");
    return(-1);
  }
  if(msg.data) free(msg.data);
  if(msg.reply_string) *buf = msg.reply_string;
  *code_min = 0; /* XXX fill in flag for fault */
  return(msg.reply_code);
}



/*
 *  Returns: 0 - not connected, timeout set, 1 - OK
 */
int 
event_queue_connect(struct event_queue *eq)
{
  int ret;
  struct timeval tv;
  edg_wll_GssStatus gss_stat;
  cred_handle_t *local_cred_handle;

  assert(eq != NULL);

#ifdef LB_PERF
  if(!nosend) {
#endif

  if(eq->gss.context == NULL) {

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    /* get pointer to the credentials */
    if(pthread_mutex_lock(&cred_handle_lock) < 0)
	    abort();
    local_cred_handle = cred_handle;
    local_cred_handle->counter++;
    if(pthread_mutex_unlock(&cred_handle_lock) < 0)
	    abort();
    
    il_log(LOG_DEBUG, "    trying to connect to %s:%d\n", eq->dest_name, eq->dest_port);
    ret = edg_wll_gss_connect(local_cred_handle->creds, eq->dest_name, eq->dest_port, &tv, &eq->gss, &gss_stat);
    if(pthread_mutex_lock(&cred_handle_lock) < 0)
	    abort();
    /* check if we need to release the credentials */
    --local_cred_handle->counter;
    if(local_cred_handle != cred_handle && local_cred_handle->counter == 0) {
	    edg_wll_gss_release_cred(&local_cred_handle->creds, NULL);
	    free(local_cred_handle);
	    il_log(LOG_DEBUG, "   freed credentials, not used anymore\n");
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
      eq->gss.context = NULL;
      eq->timeout = TIMEOUT;
      return(0);
    }
  }

#ifdef LB_PERF
  }
#endif

  return(1);
}


int
event_queue_close(struct event_queue *eq)
{
  assert(eq != NULL);

#ifdef LB_PERF
  if(!nosend) {
#endif

  if(eq->gss.context != NULL) {
    edg_wll_gss_close(&eq->gss, NULL);
    eq->gss.context = NULL;
  }
#ifdef LB_PERF
  }
#endif
  return(0);
}


/* 
 * Send all events from the queue.
 *   Returns: -1 - system error, 0 - not sent, 1 - queue empty
 */
int 
event_queue_send(struct event_queue *eq)
{
  int events_sent = 0;
  assert(eq != NULL);

#ifdef LB_PERF
  if(!nosend) {
#endif
  if(eq->gss.context == NULL)
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

    if(event_queue_get(eq, &msg) < 0) 
      return(-1);

    il_log(LOG_DEBUG, "    trying to deliver event at offset %d for job %s\n", msg->offset, msg->job_id_s);

#ifdef LB_PERF
    if(!nosend) {
#endif
        /* XXX: ljocha -- does it make sense to send empty messages ? */
	if (msg->len) {
	    tv.tv_sec = TIMEOUT;
	    tv.tv_usec = 0;
	    ret = edg_wll_gss_write_full(&eq->gss, msg->msg, msg->len, &tv, &bytes_sent, &gss_stat);
	    if(ret < 0) {
		    if (ret == EDG_WLL_GSS_ERROR_ERRNO && errno == EPIPE && events_sent > 0) {
			    eq->timeout = 0;
		    }  else {
			    il_log(LOG_ERR, "send_event: %s\n", error_get_msg());
			    eq->timeout = TIMEOUT;
		    }
		    return(0);
 	    }
	    if((code = get_reply(eq, &rep, &code_min)) < 0) {
		    /* could not get the reply properly, so try again later */
		    if (events_sent>0) 
			    eq->timeout = 1;
		    else {
			    eq->timeout = TIMEOUT;
			    il_log(LOG_ERR, "  error reading server %s reply:\n    %s\n", eq->dest_name, error_get_msg());
		    }
		    return(0);
	    }
	}
	else { code = 200; code_min = 0; rep = strdup("not sending empty message"); }
#ifdef LB_PERF
    } else {
	    glite_wll_perftest_consumeEventIlMsg(msg->msg+17);
	    code = 200;
	    rep = strdup("OK");
    }
#endif
    
    il_log(LOG_DEBUG, "    event sent, server %s replied with %d, %s\n", eq->dest_name, code, rep);
    free(rep);

    /* the reply is back here, decide what to do with message */
    /* HTTP error codes:
       1xx - informational (eg. 100 Continue)
       2xx - successful (eg. 200 OK)
       3xx - redirection (eg. 301 Moved Permanently)
       4xx - client error (eq. 400 Bad Request)
       5xx - server error (eq. 500 Internal Server Error)
    */
    if(code >= 500 && code < 600) {

	    /* non fatal errors (for us), try to deliver later */
	    eq->timeout = TIMEOUT;
	    return(0);
    }

    /* the message was consumed (successfully or not) */
    /* update the event pointer */
    if(event_store_commit(msg->es, msg->ev_len, queue_list_is_log(eq)) < 0) 
	    /* failure committing message, this is bad */
	    return(-1);
    
    event_queue_remove(eq);
    events_sent++;
  } /* while */

  return(1);

} /* send_events */


/* this is just not used */
int
send_confirmation(long lllid, int code)
{
	return 0;
}
