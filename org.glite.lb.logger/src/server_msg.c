#ident "$Header$"

#include <errno.h>
#include <assert.h>
#include <string.h>

#include "interlogd.h"
#include "glite/lb/il_msg.h" 
#include "glite/lb/events_parse.h"
#include "glite/lb/consumer.h"
#include "glite/lb/context.h"

static
int 
create_msg(il_octet_string_t *ev, char **buffer, long *receipt, time_t *expires)
{
  char *p;  int  len;
  char *event = ev->data;

  *receipt = 0;

#if defined(INTERLOGD_EMS)
  /* find DG.LLLID */
  if(strncmp(event, "DG.LLLID",8) == 0 ||
	strncmp(event, "DG.LLPID",8) == 0) { /* 8 == strlen("DG.LLLID") */

    /* skip the key */
    event += 9;  /* 9 = strlen("DG.LLLID=") */
    *receipt = atol(event);
    p = strchr(event, ' ');
    if(!p) {
      set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, 
		"create_msg: error parsing locallogger PID");
      return(-1);
    }
    /* skip the value */
    event = p + 1;

    /* find DG.PRIORITY */
    p = strstr(event, "DG.PRIORITY");
    if(p) {
      int n;
      
      p += 12; /* skip the key and = */
      if((n = atoi(p)) == 0) {
	/* normal asynchronous message */
	*receipt = 0;
      }
    } else {
      /* could not find priority key */
      *receipt = 0;
    }
    
  } else {
    /* could not find local logger PID, confirmation can not be sent */
    *receipt = 0;
  }
#endif

  if(p = strstr(event, "DG.EXPIRES")) {
	  int n;

	  p += 11;
	  *expires = atoi(p);
  }
  len = encode_il_msg(buffer, ev);
  if(len < 0) {
    set_error(IL_NOMEM, ENOMEM, "create_msg: out of memory allocating message");
    return(-1);
  }
  return(len);
}


struct server_msg *
server_msg_create(il_octet_string_t *event, long offset)
{
  struct server_msg *msg;

  msg = malloc(sizeof(*msg));
  if(msg == NULL) {
    set_error(IL_NOMEM, ENOMEM, "server_msg_create: out of memory allocating message");
    return(NULL);
  }

  if(server_msg_init(msg, event) < 0) {
    server_msg_free(msg);
    return(NULL);
  }
  msg->offset = offset;

  return(msg);
}


struct server_msg *
server_msg_copy(struct server_msg *src)
{
  struct server_msg *msg;

  msg = malloc(sizeof(*msg));
  if(msg == NULL) {
    set_error(IL_NOMEM, ENOMEM, "server_msg_copy: out of memory allocating message");
    return(NULL);
  }
  
  msg->msg = malloc(src->len);
  if(msg->msg == NULL) {
    set_error(IL_NOMEM, ENOMEM, "server_msg_copy: out of memory allocating server message");
    server_msg_free(msg);
    return(NULL);
  }
  msg->len = src->len;
  memcpy(msg->msg, src->msg, src->len);

  msg->job_id_s = strdup(src->job_id_s);
  msg->ev_len = src->ev_len;
  msg->es = src->es;
  msg->receipt_to = src->receipt_to;
  msg->offset = src->offset;
#if defined(IL_NOTIFICATIONS)
  msg->dest_name = strdup(src->dest_name);
  msg->dest_port = src->dest_port;
  msg->dest = strdup(src->dest);
#endif
  msg->expires = src->expires;
  return(msg);
}


int
server_msg_init(struct server_msg *msg, il_octet_string_t *event)
{
#if defined(IL_NOTIFICATIONS)
	edg_wll_Context context;
	edg_wll_Event *notif_event;
	int ret;
#endif

	assert(msg != NULL);
	assert(event != NULL);

	memset(msg, 0, sizeof(*msg));


#if defined(IL_NOTIFICATIONS)
	edg_wll_InitContext(&context);

	/* parse the notification event */
	if((ret=edg_wll_ParseNotifEvent(context, event->data, &notif_event))) {
		set_error(IL_LBAPI, ret, "server_msg_init: error parsing notification event");
		return(-1);
	}
	/* FIXME: check for allocation error */
	if(notif_event->notification.dest_host && 
	   (strlen(notif_event->notification.dest_host) > 0)) {
		msg->dest_name = strdup(notif_event->notification.dest_host);
		msg->dest_port = notif_event->notification.dest_port;
		asprintf(&msg->dest, "%s:%d", msg->dest_name, msg->dest_port);
	}
	msg->job_id_s = edg_wll_NotifIdUnparse(notif_event->notification.notifId);
	if(notif_event->notification.jobstat && 
	   (strlen(notif_event->notification.jobstat) > 0)) {
		msg->len = create_msg(event, &msg->msg, &msg->receipt_to, &msg->expires);
	}
	edg_wll_FreeEvent(notif_event);
	free(notif_event);
	if(msg->len < 0) {
		return(-1);
	}
#else
	msg->len = create_msg(event, &msg->msg, &msg->receipt_to, &msg->expires);
	if(msg->len < 0) {
		return(-1);
	}
#ifdef LB_PERF
	if(noparse) {
		msg->job_id_s = strdup("https://localhost:9000/not_so_unique_string");
	} else 
#endif
		msg->job_id_s = edg_wll_GetJobId(event->data);
#endif
	/* remember to add event separator to the length */
	msg->ev_len = event->len + 1;

	if(msg->job_id_s == NULL) {
		set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "server_msg_init: error getting id");
		return(-1);
	}
	
	return(0);
}


int
server_msg_is_priority(struct server_msg *msg)
{
  assert(msg != NULL);

  return(msg->receipt_to != 0);
}


int
server_msg_free(struct server_msg *msg)
{
  assert(msg != NULL);

  if(msg->msg) free(msg->msg);
  if(msg->job_id_s) free(msg->job_id_s);
#if defined(IL_NOTIFICATIONS)
  if(msg->dest_name) free(msg->dest_name);
  if(msg->dest) free(msg->dest);
#endif
  free(msg);
  return 0;
}
