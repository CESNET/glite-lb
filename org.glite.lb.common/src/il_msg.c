#ident "$Header$"

#include "il_string.h"

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>


int 
encode_il_msg(char **buffer, const char *event)
{
  int len;
  char *p;


  /* allocate enough room to hold the message */
  len = 17 + len_string(event);
  if((*buffer = malloc(len)) == NULL) {
    return(-1);
  }

  p = *buffer;

  /* write header */
  sprintf(p, "%16d\n", len - 17);
  p += 17;

  /* write rest of the message */
  p = put_string(p, event);

  return(p - *buffer);
}


int
encode_il_reply(char **buffer, 
		int err_code, int err_code_min, 
		const char *err_msg)
{
  len = 17 + len_int(err_code) + len_int(err_code_min) + len_string(err_msg);
  if((*buffer = malloc(len)) == NULL) {
    return(-1);
  }

  sprintf(*buffer, "%16d\n", len - 17);
  p = *buffer + 17;
  p = put_int(p, err_code);
  p = put_int(p, err_code_min);
  p = put_string(p, err_msg);
  return(p - *buffer);
}


int 
decode_il_msg(char **event, const char *buf)
{
  char *p;

  p = get_string(buf, event);
  if(p == NULL) {
    if(*event) { free(*event); *event = NULL; };
    return(EINVAL);
  }
  return(p - buf);
}


int
decode_il_reply(int *maj, int *min, char **err, const char * buf)
{
  char *p = buf;

  p = get_int(p, maj);
  if(p == NULL) return(EINVAL);
  p = get_int(p, min);
  if(p == NULL) return(EINVAL);
  p = get_string(p, err);
  if(p == NULL) {
    if(*err) { free(*err); *err = NULL; };
    return(EINVAL);
  }
  return(p - buf);
}


int
read_il_data(char **buffer, 
	     int (*reader)(char *, const int))
{
  char buffer[17];
  int ret, len;

  /* read 17 byte header */
  len = (*reader)(buffer, 17);
  if(len < 0) {
    goto err;
  }
  if((len=atoi(buffer)) <= 0) {
    len = EINVAL;
    goto err;
  }

  /* allocate room for the body */
  *buffer = malloc(len+1);
  if(*buffer == NULL) {
    len = ENOMEM;
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
