#ident "$Header$"

#include <assert.h>
#include <errno.h>
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

#include "glite/wmsutils/jobid/cjobid.h"
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

  il_log(LOG_DEBUG, "  sent code %d back to client\n", code);

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


/*
 * Read reply from server.
 *  Returns: -1 - error reading message, 
 *         code > 0 - error code from server
 */
static
int 
get_reply(struct event_queue *eq, char **buf, int *code_min)
{
  char buffer[17];
  char *msg, *p;
  int len, code, l;
  SSL *ssl;
  struct timeval tv;

  ssl = eq->ssl;

  /* get message header */
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  code = edg_wll_ssl_read_full(ssl, buffer, 17, &tv, &len);
  if(code < 0) {
    set_error(IL_DGSSL, code, "get_reply (header)");
    return(-1);
  }
    
  buffer[16] = 0;

  sscanf(buffer, "%d", &len);
  if(len > MAXLEN) {
    set_error(IL_PROTO, LB_NOMEM, "get_reply: error reading reply length");
    return(-1);
  }

  /* allocate room for message body */
  if((msg = malloc(len)) == NULL) {
    set_error(IL_NOMEM, ENOMEM, "get_reply: no room for message body");
    return(-1);
  }

  /* read all the data */
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  code = edg_wll_ssl_read_full(ssl, msg, len, &tv, &l);
  if(code < 0) {
    set_error(IL_DGSSL, code, "get_reply (body)");
    return(-1);
  }

  p = msg;
  p = get_int(p, &code);
  if(p == NULL) {
    set_error(IL_PROTO, LB_PROTO, "get_reply: error receiving result code");
    free(msg);
    return(-1);
  }
  p = get_int(p, code_min);
  if(p == NULL) {
    set_error(IL_PROTO, LB_PROTO, "get_reply: error receiving result code minor");
    free(msg);
    return(-1);
  }
  p = get_string(p, buf);
  if(p == NULL) {
    if(*buf) {
      free(*buf);
      *buf = NULL;
    }
    free(msg);
    set_error(IL_PROTO, LB_PROTO, "get_reply: error receiving result string");
    return(-1);
  }
  free(msg);
  return(code);
}



/*
 *  Returns: -1 - internal error, 0 - not connected, timeout set, 1 - OK
 */
int 
event_queue_connect(struct event_queue *eq)
{
  int ret;
  struct timeval tv;

  assert(eq != NULL);

  if(eq->ssl == NULL) {

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if(pthread_mutex_lock(&cred_handle_lock) < 0)
	    abort();
    il_log(LOG_DEBUG, "    trying to connect to %s:%d\n", eq->dest_name, eq->dest_port);
    ret = edg_wll_ssl_connect(cred_handle, eq->dest_name, eq->dest_port, &tv, &eq->ssl);
    if(pthread_mutex_unlock(&cred_handle_lock) < 0)
	    abort();
    if(ret < 0) {
      set_error(IL_DGSSL, ret, "event_queue_connect: edg_wll_ssl_connect");
      eq->ssl = NULL;
      eq->timeout = TIMEOUT;
      return(0);
    }
  }

  return(1);
}


int
event_queue_close(struct event_queue *eq)
{
  assert(eq != NULL);

  if(eq->ssl != NULL) {
    edg_wll_ssl_close(eq->ssl);
    eq->ssl = NULL;
  }
  return(0);
}


/* 
 * Send all events from the queue.
 *   Returns: -1 - system error, 0 - not send, 1 - queue empty
 */
int 
event_queue_send(struct event_queue *eq)
{
  assert(eq != NULL);

  if(eq->ssl == NULL)
    return(0);

  /* feed the server with events */
  while (!event_queue_empty(eq)) {
    struct server_msg *msg;
    char *rep;
    int  ret, code, code_min, bytes_sent;
    struct timeval tv;

    clear_error();

    if(event_queue_get(eq, &msg) < 0) 
      return(-1);

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    ret = edg_wll_ssl_write_full(eq->ssl, msg->msg, msg->len, &tv, &bytes_sent);
    if(ret < 0) {
      eq->timeout = TIMEOUT;
      return(0);
    }
    
    if((code = get_reply(eq, &rep, &code_min)) < 0) {
      /* could not get the reply properly, so try again later */
      il_log(LOG_ERR, "  error reading server %s reply:\n    %s\n", eq->dest_name, error_get_msg());
      eq->timeout = TIMEOUT;
      return(0);
    }
    
    il_log(LOG_DEBUG, "    event sent, server %s replied with %d, %s\n", eq->dest_name, code, rep);
    free(rep);

    /* the reply is back here */
    switch(code) {
      
	    /* NOT USED: case LB_TIME: */
    case LB_NOMEM:
	    /* NOT USED: case LB_SYS:  */
	    /* NOT USED: case LB_AUTH: */
      /* non fatal errors (for us) */
      eq->timeout = TIMEOUT;
      return(0);
	
    case LB_OK:
      /* event succesfully delivered */
      
    default: /* LB_DBERR, LB_PROTO */
      /* the event was not accepted by the server */
      /* update the event pointer */
      if(event_store_commit(msg->es, msg->ev_len, queue_list_is_log(eq)) < 0) 
	/* failure committing message, this is bad */
	return(-1);
      /* if we have just delivered priority message from the queue, send confirmation */
      ret = 1;
#if defined(INTERLOGD_EMS)
      if(server_msg_is_priority(msg) &&
	 ((ret=confirm_msg(msg, code, code_min)) < 0))
	return(ret);
#endif

      if((ret == 0) &&
	 (error_get_maj() != IL_OK))
	  il_log(LOG_ERR, "send_event: %s\n", error_get_msg());
	
      event_queue_remove(eq);
      break;
      
    } /* switch */
  } /* while */

  return(1);

} /* send_events */


