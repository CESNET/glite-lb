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


#define DEFAULT_CHUNK_SIZE 1024

static
int
read_event(int sock, long *offset, il_octet_string_t *msg)
{
  char *buffer, *p, *n;
  int  len, alen, i, chunk_size = DEFAULT_CHUNK_SIZE;
  static char buf[1024];

  msg->data = NULL;
  msg->len = 0;

  /* receive offset */
  len = recv(sock, offset, sizeof(*offset), MSG_NOSIGNAL);
  if(len < sizeof(*offset)) {
    set_error(IL_PROTO, errno, "read_event: error reading offset");
    return(-1);
  }
  
  /* receive event string */
  buffer=malloc(8*chunk_size);
  if(buffer == NULL) {
    set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
    return(-1);
  }
  p = buffer;
  alen = 8*chunk_size;

  /* Variables used here:
        - buffer points to allocated memory,
	- alen is the allocated memory size,
	- p points to the first free location in buffer,
	- len is the amount actually read by recv,
	- i is the amount of data belonging to the current event (including separator).
	- n points to event separator or is NULL
    Hence:
         (p - buffer) gives the amount of valid data read so far,
	 (alen - (p - buffer)) is the free space,
  */ 
 
#if 1
  /* Reading events - optimized version. Attempts to increase chunks read by recv
   * when there are more data, reads directly into destination memory (instead of 
   * copying from static buffer) etc.
   *
   * For some reason it is not much faster than the old variant.
   */
  do {
	  /* prepare at least chunk_size bytes for next data */
	  if(alen - (p - buffer) < chunk_size) {
		  alen += (chunk_size < 8192) ? 8192 : 8*chunk_size;
		  n = realloc(buffer, alen);
		  if(n == NULL) {
			  free(buffer);
			  set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
			  return(-1);
		  }
		  p = n + (p - buffer);
		  buffer = n;
	  }

	  /* read chunk */
	  if((len=recv(sock, p, chunk_size, MSG_PEEK | MSG_NOSIGNAL)) > 0) {
		  /* find the end of event, if any */
		  /* faster (and dirty) way of doing strnchr (which is not in libc, anyway) */
		  if((n=memccpy(p, p, EVENT_SEPARATOR, len)) != NULL) {
			  i = n - p; /* length including separator */
		  } else {
			  i = len;
			  /* long event, huh? try reading more data at once */
			  chunk_size += 1024;
		  }
		  /* remove the relevant data from input */
		  /* i > 0 */
		  if(recv(sock, p, i, MSG_NOSIGNAL) != i) {
			  set_error(IL_SYS, errno, "read_event: error reading data");
			  free(buffer);
			  return(-1);
		  }
		  /* move the pointer to the first free place, separator is considered free space */
		  p = (n == NULL) ? p + len : n - 1;
	  }
  } while ( (len > 0) && (n == NULL) );

#else
  /* Reading events - original version.
   * Appears to behave quite good, anyway.
   */
  while((len=recv(sock, buf, sizeof(buf), MSG_PEEK | MSG_NOSIGNAL)) > 0) {

    /* we have to be prepared for sizeof(buf) bytes */
    if(alen - (p - buffer) < (int)sizeof(buf)) {
      alen += 8192;
      n = realloc(buffer, alen);
      if(n == NULL) {
	free(buffer);
	set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
	return(-1);
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
	return(-1);
      }
    if(i < len)
      /* the event is complete */
      break;
  }
#endif

  /* terminate buffer */
  *p = 0;

  if(len < 0) {
    set_error(IL_SYS, errno, "read_event: error reading data");
    free(buffer);
    return(-1);
  }

  /* if len == 0, we have not encountered EVENT_SEPARATOR and thus the event is not complete */
  if(len == 0) {
    set_error(IL_PROTO, errno, "read_event: error reading data - premature EOF");
    free(buffer);
    return(-1);
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
    return(-1);
  }
#endif

  msg->data = buffer;
  msg->len = p - buffer;
  return(msg->len);
}


/*
 * Returns: -1 on error, 0 if no message available, message length otherwise
 *
 */
#ifdef PERF_EVENTS_INLINE
int
input_queue_get(il_octet_string *buffer, long *offset, int timeout)
{
	static long o = 0;
	int len;

	len = glite_wll_perftest_produceEventString(&buffer->data);
	buffer->len = len;
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
input_queue_get(il_octet_string_t *buffer, long *offset, int timeout)
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

  read_event(accepted, offset, buffer);
  close(accepted);

  if(buffer->data == NULL) {
    if(error_get_maj() != IL_OK)
      return(-1);
    else
      return(0);
  }
    
  return(buffer->len);
}
#endif
