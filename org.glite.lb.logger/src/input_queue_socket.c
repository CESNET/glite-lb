#ident "$Header$"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "interlogd.h"


static const int   SOCK_QUEUE_MAX = 50;
extern char *socket_path;

static int sock;
static int accepted;

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


static
char *
read_event(int sock, long *offset)
{
  char *buffer, *p, *n;
  int  len, alen;
  char buf[256];

  /* receive offset */
  len = recv(sock, offset, sizeof(*offset), MSG_NOSIGNAL);
  if(len < sizeof(*offset)) {
    set_error(IL_PROTO, errno, "read_event: error reading offset");
    return(NULL);
  }
  
  /* receive event string */
  buffer=malloc(1024);
  if(buffer == NULL) {
    set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
    return(NULL);
  }
  p = buffer;

  alen = 1024;
  while((len=recv(sock, buf, sizeof(buf), MSG_PEEK | MSG_NOSIGNAL)) > 0) {
    int i;

    /* we have to be prepared for sizeof(buf) bytes */
    if(alen - (p - buffer) < (int)sizeof(buf)) {
      alen += 8192;
      n = realloc(buffer, alen);
      if(n == NULL) {
	free(buffer);
	set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
	return(NULL);
      }
      p = p - buffer + n;
      buffer = n;
    }

    /* copy all relevant bytes from buffer */
    n = (char*)memccpy(p, buf, EVENT_SEPARATOR, len);
    if(n) {
	    /* separator found */
	    n--; /* but do not preserve it */
	    i = n - p;
	    p = n;
    } else {
	    /* separator not found */
	    i = len;
	    p += len;
    }
   /* This was definitely slowing us down:
    *    for(i=0; (i < len) && (buf[i] != EVENT_SEPARATOR); i++) 
    *    *p++ = buf[i];
    */

    /* remove the data from queue */
    if(i > 0) 
      if(recv(sock, buf, i, MSG_NOSIGNAL) != i) {
	set_error(IL_SYS, errno, "read_event: error reading data");
	free(buffer);
	return(NULL);
      }
    if(i < len)
      /* the event is complete */
      break;
  }

  /* terminate buffer */
  *p = 0;

  if(len < 0) {
    set_error(IL_SYS, errno, "read_event: error reading data");
    free(buffer);
    return(NULL);
  }

  /* if len == 0, we have not encountered EVENT_SEPARATOR and thus the event is not complete */
  if(len == 0) {
    set_error(IL_PROTO, errno, "read_event: error reading data - premature EOF");
    free(buffer);
    return(NULL);
  }

#if 0
  /* this is probably not necessary at all:
     either len <=0, which was covered before,
     or 0 <= i < len => p > buffer;
     I would say this condition can not be satisfied.
  */
  if(p == buffer) {
    set_error(IL_PROTO, errno, "read_event: error reading data - no data received");
    free(buffer);
    return(NULL);
  }
#endif

  return(buffer);
}


/*
 * Returns: -1 on error, 0 if no message available, message length otherwise
 *
 */
int
input_queue_get(char **buffer, long *offset, int timeout)
{
  fd_set fds;
  struct timeval tv;
  int msg_len;

  assert(buffer != NULL);

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

  *buffer = read_event(accepted, offset);
  close(accepted);

  if(*buffer == NULL) {
    if(error_get_maj() != IL_OK)
      return(-1);
    else
      return(0);
  }
    
  return(strlen(*buffer));
}
