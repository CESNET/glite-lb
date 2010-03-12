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


#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "interlogd.h"

static const int   SOCK_QUEUE_MAX = 50;
extern char *socket_path;
extern char *file_prefix;

static int sock;
static int accepted;

static
int plain_reader(void *user_data, char *buffer, const int len)
{
	return (recv(*(int*)user_data, buffer, len, MSG_NOSIGNAL));
}

		 
int 
input_queue_attach()
{ 
  struct sockaddr_un saddr;

  if((sock=socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    set_error(IL_SYS, errno, "input_queue_attach: error creating socket");
    return(-1);
  }

  memset(&saddr, 0, sizeof(saddr));
  saddr.sun_family = AF_UNIX;
  strcpy(saddr.sun_path, socket_path);

  /* test for the presence of the socket and another instance 
     of interlogger listening */
  if(connect(sock, (struct sockaddr *)&saddr, sizeof(saddr.sun_path)) < 0) {
	  if(errno == ECONNREFUSED) {
		  /* socket present, but no one at the other end; remove it */
		  il_log(LOG_WARNING, "  removing stale input socket %s\n", socket_path);
		  unlink(socket_path);
	  }
	  /* ignore other errors for now */
  } else {
	  /* connection was successful, so bail out - there is 
	     another interlogger running */
	  set_error(IL_SYS, EADDRINUSE, "input_queue_attach: another instance of interlogger is running");
	  return(-1);
  }
  
  if(bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    set_error(IL_SYS, errno, "input_queue_attach: error binding socket");
    return(-1);
  }

  if (listen(sock, SOCK_QUEUE_MAX)) {
    set_error(IL_SYS, errno, "input_queue_attach: error listening on socket");
    return -1;
  }

  return(0);
}


void input_queue_detach()
{
  if (sock >= 0)
    close(sock);
  unlink(socket_path);
}



/*
 * Returns: -1 on error, 0 if no message available, message length otherwise
 *
 */
#ifdef PERF_EVENTS_INLINE
int
input_queue_get(il_octet_string_t **buffer, long *offset, int timeout)
{
	static long o = 0;
	int len;
	char *jobid;
	static il_octet_string_t my_buffer;
	
	assert(buffer != NULL);

	*buffer = &my_buffer;

	len = glite_wll_perftest_produceEventString(&my_buffer.data, &jobid);
	my_buffer.len = len;
	if(len) {
		o += len;
		*offset = o;
	} else if (len == 0) {
		sleep(timeout);
	}
	return(len);
}
#else
int
input_queue_get(il_octet_string_t **buffer, long *offset, int timeout)
{
  fd_set fds;
  struct timeval tv;
  int msg_len;
  static il_http_message_t msg;

  assert(buffer != NULL);

  *buffer = (il_octet_string_t *)&msg;

  FD_ZERO(&fds);
  FD_SET(sock, &fds);
  
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  
  msg_len = select(sock + 1, &fds, NULL, NULL, timeout >= 0 ? &tv : NULL);
  switch(msg_len) {
     
  case 0: /* timeout */
    return(0);
    
  case -1: /* error */
	  switch(errno) {
	  case EINTR:
		  il_log(LOG_DEBUG, "  interrupted while waiting for event!\n");
		  return(0);

	  default:
		  set_error(IL_SYS, errno, "input_queue_get: error waiting for event");
		  return(-1);
	  }
  default:
	  break;
  }
  
  if((accepted=accept(sock, NULL, NULL)) < 0) {
    set_error(IL_SYS, errno, "input_queue_get: error accepting connection");
    return(-1);
  }

  msg_len = receive_http(&accepted, plain_reader, &msg);

  if(msg_len < 0) {
	  close(accepted);
	  if(error_get_maj() != IL_OK) 
		  return -1;
	  else
		  return 0;
  }

  close(accepted);
  *offset = -1;
  return(msg.len);
}
#endif

