#ident "$Header$"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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


int
store_to_file(il_http_message_t *msg, long *offset) {
	char s_len[20];
	char filename[PATH_MAX];
	FILE *outfile;
	int i, filedesc;
	int ret = -1;

	if(msg->host == NULL) {
		set_error(IL_PROTO, EINVAL, "store_to_file: no message destination specified");
	}

	snprintf(filename, sizeof(filename), "%s.%s", file_prefix, msg->host);
	filename[sizeof(filename) - 1] = 0;
	snprintf(s_len+1, sizeof(s_len)-1, "%18d\n", msg->len);
	s_len[sizeof(s_len) - 1] = 0;
	s_len[0] = 0;

try_again:
	if((outfile = fopen(filename, "a")) == NULL) {
		set_error(IL_SYS, errno, "store_to_file: error opening file");
		goto cleanup;
	}
	if((filedesc = fileno(outfile)) < 0) {
		set_error(IL_SYS, errno, "store_to_file: error getting file descriptor");
		goto cleanup;
	}

	for(i = 0; i < 5; i++) {
		struct flock filelock;
		int filelock_status;
		struct stat statbuf;

		filelock.l_type = F_WRLCK;
		filelock.l_whence = SEEK_SET;
		filelock.l_start = 0;
		filelock.l_len = 0;

		if((filelock_status=fcntl(filedesc, F_SETLK, &filelock)) < 0) {
			switch(errno) {
			case EAGAIN:
			case EACCES:
			case EINTR:
				if((i+1) < 5) sleep(1);
				break;
			default:
				set_error(IL_SYS, errno, "store_to_file: error locking file");
				goto cleanup;
			}
		} else {
			if(stat(filename, &statbuf)) {
				if(errno == ENOENT) {
					fclose(outfile);
					goto try_again;
				} else {
					set_error(IL_SYS, errno, "store_file: could not stat file");
					goto cleanup;
				}
			} else {
				/* success */
				break;
			}
		}
	}

	if(i == 5) {
		set_error(IL_SYS, ETIMEDOUT, "store_to_file: timed out trying to lock file");
		goto cleanup;
	}
	if(fseek(outfile, 0, SEEK_END) < 0) {
		set_error(IL_SYS, errno, "store_to_file: error seeking at end of file");
		goto cleanup;
	}
	if((*offset=ftell(outfile)) < 0) {
		set_error(IL_SYS, errno, "store_to_file: error getting current position");
		goto cleanup;
	}
	if(fwrite(s_len, sizeof(s_len), 1, outfile) != 1) {
		set_error(IL_SYS, errno, "store_to_file: error writing data header to file");
		goto cleanup;
	}
	if(fwrite(msg->data, msg->len, 1, outfile) != 1) {
		set_error(IL_SYS, errno, "store_to_file: error writing data to file");
		goto cleanup;
	}
	ret = 0;
	fflush(outfile);

cleanup:
	if(outfile) fclose(outfile);
	return ret;
}


int 
send_reply(int sd)
{
	const char reply[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SOAP-ENV:Envelope"
		" xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\""
		" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
		" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
		" xmlns:ns3=\"http://glite.org/wsdl/types/jp\""
		" xmlns:ns1=\"http://glite.org/wsdl/services/jp\""
		" xmlns:ns2=\"http://glite.org/wsdl/elements/jp\">"
		" <SOAP-ENV:Body>"
		"  <ns2:UpdateJobsResponse>"
		"  </ns2:UpdateJobsResponse>"
		" </SOAP-ENV:Body>"
		"</SOAP-ENV:Envelope>";
	
	return(write(sd, reply, sizeof(reply)));
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

  if(store_to_file(&msg, offset) < 0) {
	  close(accepted);
	  return -1;
  }

  send_reply(accepted);
  close(accepted);
  return(msg.len);
}
#endif

