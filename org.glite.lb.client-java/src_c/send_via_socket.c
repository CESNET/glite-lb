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

#include <jni.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <sys/un.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "org_glite_lb_ContextIL.h"

#define tv_sub(a, b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}


/*!
 * Write to socket
 * Needn't write entire buffer. Timeout is applicable only for non-blocking
 * connections 
 * \param sock IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to write
 * \param timeout INOUT: max time allowed for operation, remaining time on return
 * \retval bytes written (>0) on success
 * \retval -1 on write error
 */
static ssize_t
edg_wll_socket_write(
	int sock,
	const void *buf,
	size_t bufsize,
	struct timeval *timeout)
{ 
	ssize_t	len = 0;
	fd_set fds;
	struct timeval to, before, after;


	if ( timeout ) {
		memcpy(&to, timeout, sizeof to);
		gettimeofday(&before, NULL);
	}
	len = write(sock, buf, bufsize);
	if ( len <= 0  && errno == EAGAIN ) {
		FD_ZERO(&fds);
		FD_SET(sock,&fds);
		if ( select(sock+1, NULL, &fds, NULL, timeout? &to: NULL) < 0 ) {
			len = -1;
		} else {
			len = write(sock, buf, bufsize);
		}
	}
	if ( timeout ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*timeout, after);
		if ( timeout->tv_sec < 0 ) {
			timeout->tv_sec = 0;
			timeout->tv_usec = 0;
		}
	}

	return len;
}

/*!
 * Write specified amount of data to socket
 * Attempts to call edg_wll_socket_write() untill the entire request is satisfied
 * (or times out).
 * \param sock IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to write
 * \param timeout INOUT: max time allowed for operation, remaining time on return
 * \param total OUT: bytes actually written
 * \retval bytes written (>0) on success
 * \retval -1 on write error
 */
static ssize_t
edg_wll_socket_write_full(
	int sock,
	void *buf,
	size_t bufsize,
	struct timeval *timeout,
	ssize_t *total)
{
	ssize_t	len;
	*total = 0;

	while ( *total < bufsize ) {
		len = edg_wll_socket_write(sock, buf+*total, bufsize-*total, timeout);
		if (len < 0) return len;
		*total += len;
	}

	return 0;
}

/*
 * edg_wll_log_event_send - send event to the socket
 *
 * Returns: 0 if done properly or errno
 *
 */
/*int edg_wll_log_event_send(
	const char *socket_path,
	long filepos,
	const char *msg,
	int msg_size,
	int conn_attempts,
	int timeout_int)*/
/*JNIEXPORT jint JNICALL Java_org_glite_lb_ContextIL_edg_wll_log_event_send
   (JNIEnv *env, 
    jobject jobj, 
    jstring socket_path_j, 
    jlong filepos_j, 
    jstring msg_j, 
    jint msg_size_j, 
    jint conn_attempts_j, 
    jint timeout_int_j)*/
JNIEXPORT jint JNICALL
Java_org_glite_lb_ContextIL_sendToSocket
   (JNIEnv *env, 
    jobject jobj, 
    jstring socket_path_j, 
    jlong filepos_j, 
    jstring msg_j, 
    jint msg_size_j,
    jint conn_attempts_j,
    jint timeout_int_j)

{
	const char *socket_path = (*env)->GetStringUTFChars(env, socket_path_j, 0);
	const char *msg = (*env)->GetStringUTFChars(env, msg_j, 0);
	int timeout_int = (int) timeout_int_j;
        //int timeout_int = 3;
        long filepos = (long) filepos_j;
        int msg_size = (int) msg_size_j; 
        int conn_attempts = (int) conn_attempts_j; 
        //int conn_attempts = 3;
        struct timeval timeout;
        timeout.tv_sec = timeout_int;
	timeout.tv_usec = 0;	
	struct sockaddr_un saddr;
	int msg_sock,
	    flags,
	    conn_timeout, i;
        ssize_t count = 0;


	if ( (msg_sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 ) {
		goto event_send_end;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, socket_path);

	if (   (flags = fcntl(msg_sock, F_GETFL, 0)) < 0
		|| fcntl(msg_sock, F_SETFL, flags | O_NONBLOCK) < 0 ) {
		goto cleanup;
	}

	conn_timeout = floor(timeout.tv_sec/(conn_attempts + 1));
	for ( i = 0; i < conn_attempts; i++) {
		if ( connect(msg_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0 ) {
			if ( errno == EISCONN ) break;
			else if ((errno == EAGAIN) || (errno == ETIMEDOUT)) {
				sleep(conn_timeout);
				timeout.tv_sec -= conn_timeout;
				continue;
			} else {
				goto cleanup;
			}
		} else break;
	}

	if ( edg_wll_socket_write_full(msg_sock, &filepos, sizeof(filepos), &timeout, &count) < 0
) {
		goto cleanup;
	}

	if ( edg_wll_socket_write_full(msg_sock, (void *)msg, msg_size,
&timeout, &count) < 0 ) {
		goto cleanup;
	}

cleanup:
	close(msg_sock); 

event_send_end:
	return 0;
}
