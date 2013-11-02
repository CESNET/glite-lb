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


#include "il_string.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define IL_PROTOCOL_MAGIC_WORD "michal"

int 
encode_il_msg(char **buffer, const il_octet_string_t *event)
{
  int len;
  char *p;
  il_octet_string_t protocol_magic_word;
  
  protocol_magic_word.data = IL_PROTOCOL_MAGIC_WORD;
  protocol_magic_word.len = strlen(IL_PROTOCOL_MAGIC_WORD);

  /* allocate enough room to hold the message */
  len = 17 + len_string(&protocol_magic_word) + len_string((il_octet_string_t*)event);
  if((*buffer = malloc(len)) == NULL) {
    return(-1);
  }

  p = *buffer;

  /* write header */
  sprintf(p, "%16d\n", len - 17);
  p += 17;

  /* write rest of the message */
  p = put_string(p, &protocol_magic_word);
  p = put_string(p, (il_octet_string_t*)event);

  return(p - *buffer);
}


int
encode_il_reply(char **buffer, 
		int err_code, int err_code_min, 
		const char *err_msg)
{
  int len;
  char *p;
  il_octet_string_t emsg;

  emsg.data = (char*)err_msg;
  emsg.len = strlen(err_msg);
  len = 17 + len_int(err_code) + len_int(err_code_min) + len_string(&emsg);
  if((*buffer = malloc(len)) == NULL) {
    return(-1);
  }

  sprintf(*buffer, "%16d\n", len - 17);
  p = *buffer + 17;
  p = put_int(p, err_code);
  p = put_int(p, err_code_min);
  p = put_string(p, &emsg);
  return(p - *buffer);
}


int 
decode_il_msg(il_octet_string_t *event, const char *buf)
{
  const char *p;
  il_octet_string_t protocol_magic_word;
  int magic_word_check_failed = 0;

  assert( event != NULL );

  /* First check that the protocol 'magic' word is there */
  p = get_string((char*)buf, &protocol_magic_word);
  if (protocol_magic_word.data) {
    if (strcmp (protocol_magic_word.data, IL_PROTOCOL_MAGIC_WORD) != 0) {
      magic_word_check_failed = 1;
    }
    free(protocol_magic_word.data);
  }

  if (magic_word_check_failed != 0) return (-1);

  p = get_string(p, event);
  if(p == NULL) {
    if(event->data) { free(event->data); event->data = NULL; };
    return(-1);
  }
  return(p - buf);
}


int
decode_il_reply(int *maj, int *min, char **err, const char * buf)
{
	const char *p = buf;
  il_octet_string_t e;

  p = get_int(p, maj);
  if(p == NULL) return(-1);
  p = get_int(p, min);
  if(p == NULL) return(-1);
  p = get_string(p, &e);
  if(p == NULL) {
    if(e.data) { free(e.data); e.data = NULL; };
    return(-1);
  }
  *err = e.data;
  return(p - buf);
}


int
read_il_data(void *user_data,
	     char **buffer, 
	     int (*reader)(void *, char *, const int))
{
	char buf[17], *p;
  int ret, len;

  /* read 17 byte header */
  len = (*reader)(user_data, buf, 17);
  if(len < 0) {
    goto err;
  }

  /* perform some sanity checks on the received header */
  if(buf[16] != '\n') {
	  len = -1;
	  goto err;
  }
  buf[16] = 0;
  /* skip leading spaces */
  for(p = buf; *p == ' '; p++);
  /* skip digits */
  for(; (*p >= '0') && (*p <= '9'); p++);
  /* this must be the end of string */
  if(*p != 0) {
	  len = -1;
	  goto err;
  }

  if((len=atoi(buf)) <= 0) {
    len = -1;
    goto err;
  }

  /* allocate room for the body */
  *buffer = malloc(len+1);
  if(*buffer == NULL) {
    len = -1;
    goto err;
  }

  /* read body */
  ret = (*reader)(user_data, *buffer, len);
  if(ret < 0) {
    free(*buffer);
    *buffer = NULL;
    len = ret;
    goto err;
  }

  (*buffer)[len] = 0;

 err:
  return(len);
}
