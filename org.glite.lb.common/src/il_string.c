#ident "$Header$"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "il_string.h"

char *
put_string(char *p, il_octet_string_t *s)
{
  assert( p != NULL );
  assert( s != NULL );

  p = _put_int(p, s->len);
  *p++ = ' ';
  /* strncpy(p, s->data, s->len); */
  memcpy(p, s->data, s->len);
  p += s->len;
  *p++ = '\n';
  return(p);
}


int
len_string(il_octet_string_t *s)
{
  int len;

  assert( s != NULL );

  len = len_int(s->len);
  return(len + s->len + 1);
}


char *
get_string(char *p, il_octet_string_t *s)
{
  int len;

  assert( p != NULL );
  assert( s != NULL );

  s->data = NULL;

  p = _get_int(p, &len);
  if(*p != ' ')
    return(NULL);
  else
    {
      s->data = malloc(len + 1);
      if(s->data == NULL)
	return(NULL);
      /* strncpy(s->data, ++p, len); */
      memcpy(s->data, ++p, len);
      (s->data)[len] = '\0';
      p += len;
      return( *p++ == '\n' ? p : NULL );
    }
}
