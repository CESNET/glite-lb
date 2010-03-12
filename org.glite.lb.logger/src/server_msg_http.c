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

#include "interlogd.h"
#include "glite/lb/il_msg.h" 
#include "glite/lb/events_parse.h"
#include "glite/lb/context.h"

static
int 
create_msg(il_http_message_t *ev, char **buffer, long *receipt, time_t *expires)
{
  char *event = ev->data;

  *receipt = 0;
  *expires = 0;

  *buffer = ev->data;
  return ev->len;;
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
	il_http_message_t *hmsg = (il_http_message_t *)event;

	assert(msg != NULL);
	assert(event != NULL);

	memset(msg, 0, sizeof(*msg));


	msg->job_id_s = hmsg->host;
	if(msg->job_id_s == NULL) {
		set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "server_msg_init: error getting id");
		return -1;
	}
	msg->len = create_msg(hmsg, &msg->msg, &msg->receipt_to, &msg->expires);
	if(msg->len < 0)
		return -1;
	/* set this to indicate new data owner */
	hmsg->data = NULL;
	hmsg->host = NULL;
	msg->ev_len = hmsg->len + 1; /* must add separator size too */
	return 0;

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
  free(msg);
  return 0;
}
