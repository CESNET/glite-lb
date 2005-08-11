#ident "$Header$"

#include "il_string.h"

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define IL_PROTOCOL_MAGIC_WORD "michal"

int 
encode_il_msg(char **buffer, const char *event)
{
  int len;
  char *p;
  char *protocol_magic_word = IL_PROTOCOL_MAGIC_WORD;


  /* allocate enough room to hold the message */
  len = 17 + len_string(protocol_magic_word) + len_string((char*)event);
  if((*buffer = malloc(len)) == NULL) {
    return(-1);
  }

  p = *buffer;

  /* write header */
  sprintf(p, "%16d\n", len - 17);
  p += 17;

  /* write rest of the message */
  p = put_string(p, protocol_magic_word);
  p = put_string(p, (char*)event);

  return(p - *buffer);
}


int
encode_il_reply(char **buffer, 
		int err_code, int err_code_min, 
		const char *err_msg)
{
  int len;
  char *p;

  len = 17 + len_int(err_code) + len_int(err_code_min) + len_string((char*)err_msg);
  if((*buffer = malloc(len)) == NULL) {
    return(-1);
  }

  sprintf(*buffer, "%16d\n", len - 17);
  p = *buffer + 17;
  p = put_int(p, err_code);
  p = put_int(p, err_code_min);
  p = put_string(p, (char*)err_msg);
  return(p - *buffer);
}


int 
decode_il_msg(char **event, const char *buf)
{
  char *p;
  char *protocol_magic_word=NULL;
  int magic_word_check_failed = 0;

  /* First check that the protocol 'magic' word is there */
  p = get_string((char*)buf, &protocol_magic_word);
  if (protocol_magic_word) {
    if (strcmp (protocol_magic_word, IL_PROTOCOL_MAGIC_WORD) != 0) {
      magic_word_check_failed = 1;
    }
    free(protocol_magic_word);
  }

  if (magic_word_check_failed != 0) return (-1);

  p = get_string(p, event);
  if(p == NULL) {
    if(*event) { free(*event); *event = NULL; };
    return(-1);
  }
  return(p - buf);
}


int
decode_il_reply(int *maj, int *min, char **err, const char * buf)
{
  char *p = buf;

  p = get_int(p, maj);
  if(p == NULL) return(-1);
  p = get_int(p, min);
  if(p == NULL) return(-1);
  p = get_string(p, err);
  if(p == NULL) {
    if(*err) { free(*err); *err = NULL; };
    return(-1);
  }
  return(p - buf);
}


int
read_il_data(char **buffer, 
	     int (*reader)(char *, const int))
{
  char buf[17];
  int ret, len;

  /* read 17 byte header */
  len = (*reader)(buf, 17);
  if(len < 0) {
    goto err;
  }
  buf[16] = 0;
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
  ret = (*reader)(*buffer, len);
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
