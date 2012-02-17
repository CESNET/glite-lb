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


#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "interlogd.h"
#include "glite/lb/il_msg.h" 
#include "glite/lb/events_parse.h"
#include "glite/lb/context.h"

static
int 
create_msg(il_octet_string_t *ev, char **buffer, long *receipt, time_t *expires)
{
  char *p;  int  len;
  char *event = ev->data;

  *receipt = 0L;

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
      n = atoi(p);
      if((n & (EDG_WLL_LOGFLAG_SYNC|EDG_WLL_LOGFLAG_SYNC_COMPAT)) == 0) {
	/* normal asynchronous message */
	      *receipt = 0L;
      }
    } else {
      /* could not find priority key */
      *receipt = 0L;
    }
    
  } else {
    /* could not find local logger PID, confirmation can not be sent */
    *receipt = 0L;
  }
#endif

  if((p = strstr(event, "DG.EXPIRES")) != NULL) {
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
  msg->dest_name = src->dest_name ? strdup(src->dest_name) : NULL;
  msg->dest_port = src->dest_port;
  msg->dest = src->dest ? strdup(src->dest) : NULL;
  msg->owner = src->owner ? strdup(src->owner) : NULL;
#endif
  msg->expires = src->expires;
  msg->generation = src->generation;
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

	/* parse the notification event */
	edg_wll_InitContext(&context);
	ret=edg_wll_ParseNotifEvent(context, event->data, &notif_event);
	edg_wll_FreeContext(context);
	if(ret) {
		set_error(IL_LBAPI, ret, "server_msg_init: error parsing notification event");
		return(-1);
	}

	/* FIXME: check for allocation error */
	if(notif_event->notification.dest_url &&
		(strlen(notif_event->notification.dest_url) > 0)) {
		/* destination URL */
		msg->dest = strdup(notif_event->notification.dest_url);
		msg->dest_name = NULL;
		msg->dest_port = 0;
	} else if(notif_event->notification.dest_host &&
	   (strlen(notif_event->notification.dest_host) > 0)) {
		/* destination host and port */
		msg->dest_name = strdup(notif_event->notification.dest_host);
		msg->dest_port = notif_event->notification.dest_port;
		asprintf(&msg->dest, "%s:%d", msg->dest_name, msg->dest_port);
	}
	msg->job_id_s = edg_wll_NotifIdUnparse(notif_event->notification.notifId);
	if(notif_event->notification.jobstat && 
	   (strlen(notif_event->notification.jobstat) > 0)) {
		msg->len = create_msg(event, &msg->msg, &msg->receipt_to, &msg->expires);
	}
	msg->expires = notif_event->notification.expires;
	if(notif_event->notification.owner) {
		msg->owner = strdup(notif_event->notification.owner);
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
  if(msg->owner) free(msg->owner);
#endif
  free(msg);
  return 0;
}
