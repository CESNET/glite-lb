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


#include <string.h>
#include <errno.h>

#include "interlogd.h"


int 
parse_request(const char *s, il_http_message_t *msg)
{
	if(!strncasecmp(s, "HTTP", 4)) {
		msg->msg_type = IL_HTTP_REPLY;
	} else if(!strncasecmp(s, "POST", 4)) {
		msg->msg_type = IL_HTTP_POST;
	} else if(!strncasecmp(s, "GET", 3)) {
		msg->msg_type = IL_HTTP_GET;
	} else {
		msg->msg_type = IL_HTTP_OTHER;
	}
	if(msg->msg_type == IL_HTTP_REPLY) {
		char *p = strchr(s, ' ');

		if(!p) goto parse_end;
		p++;
		msg->reply_code=atoi(p);
		p = strchr(p, ' ');
		if(!p) goto parse_end;
		p++;
		msg->reply_string = strdup(p);

	parse_end:
		;
	}
}


int
parse_header(const char *s, il_http_message_t *msg)
{
	if(!strncasecmp(s, "Content-Length:", 15)) {
		msg->content_length = atoi(s + 15);
	} else if(!strncasecmp(s, "Host:", 5)) {
		const char *p = s + 4;
		while(*++p == ' '); /* skip spaces */
		msg->host = strdup(p);
	}
	return(0);
}


#define DEFAULT_CHUNK_SIZE 1024

// read what is available and parse what can be parsed
// returns the result of read operation of the underlying connection,
// ie. the number of bytes read or error code
int
receive_http(void *user_data, int (*reader)(void *, char *, const int), il_http_message_t *msg)
{
	static enum { NONE, IN_REQUEST, IN_HEADERS, IN_BODY } state = NONE;
	int  len, alen, clen, i, buffer_free, min_buffer_free = DEFAULT_CHUNK_SIZE;
	char *buffer, *p, *s, *cr;
	
	memset(msg, 0, sizeof(*msg));
	// msg->data = NULL;
	// msg->len = 0;
	state = IN_REQUEST;
	alen = 0;
	buffer = NULL;
	buffer_free = 0;
	p = NULL;
	s = NULL;

	do {
		/* p - first empty position in buffer
		   alen - size of allocated buffer
		   len - number of bytes received in last read
		   s - points behind last scanned CRLF or at buffer start 
		   buffer_free = alen - (p - buffer) 
		*/

		/* prepare at least chunk_size bytes for next data */
		if(buffer_free < min_buffer_free) {
		        char *n;
			
			alen += min_buffer_free;
			n = realloc(buffer, alen);
			if(n == NULL) {
				free(buffer);
				set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
				return(-1);
			}
			buffer_free += min_buffer_free;
			p = n + (p - buffer);
			s = n + (s - buffer);
			buffer = n;
		}

		if(buffer_free > 0) {
			len = (*reader)(user_data, p, buffer_free); 
			if(len < 0) {
				// error
				free(buffer);
				// set_error(IL_SYS, errno, "receive_http: error reading data");
				return -1;
			} else if(len == 0) {
				// EOF
				free(buffer);
				set_error(IL_PROTO, errno, "receive_http: error reading data - premature EOF");
				return -1;
			}
			buffer_free -= len;
			p+= len;
		}


		switch(state) {

			// parse buffer, look for CRLFs
			//   s - start scan position
			//   p - start of current token
			//   cr - current CRLF position

		case IN_REQUEST:
			if((s < p - 1) &&
			   (cr = (char*)memchr(s, '\r', p - s - 1)) &&
			   (cr[1] == '\n')) {
				*cr = 0;
				parse_request(s, msg);
				*cr = '\r';
				// change state
				state = IN_HEADERS;
				// start new tokens (cr < p - 1 -> s < p + 1 <-> s <= p)
				s = cr + 2;
			} else {
			  break;
			}

		case IN_HEADERS:  
			while((state != IN_BODY) &&
			      (s < p - 1) && 
			      (cr = (char*)memchr(s, '\r', p - s - 1)) &&
			      (cr[1] == '\n')) {
				if(s == cr) { /* do not consider request starting with CRLF */
					// found CRLFCRLF
					state = IN_BODY;
				} else {
					*cr = 0;
					parse_header(s, msg);
					*cr = '\r';
				}
				// next scan starts after CRLF
				s = cr + 2; 
			}
			if(state == IN_BODY) {
				// we found body
				// content-length should be set at the moment
				if(msg->content_length > 0) {
					int need_free = msg->content_length - (p - s);
					char *n;
			
					alen += need_free - buffer_free + 1;
					n = realloc(buffer, alen);
					if(n == NULL) {
						free(buffer);
						set_error(IL_NOMEM, ENOMEM, "read_event: no room for event");
						return(-1);
					}
					buffer_free = need_free;
					min_buffer_free = 0;
					p = n + (p - buffer);
					s = n + (s - buffer);
					buffer = n;
				} else {
					// report error
					free(buffer);
					set_error(IL_PROTO, EINVAL, "receive_http: error reading data - no content length specified\n");
					return -1;
				}
			}
			break;
			
		case IN_BODY:
			if(buffer_free == 0) {
				// finished reading
				*p = 0;
				state = NONE;
			}
			break;
		}
	} while(state != NONE);
	
	msg->data = buffer;
	msg->len = p - buffer;

	return 0;
}
