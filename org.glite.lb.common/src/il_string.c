#ident "$Header$"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "il_string.h"

char *
put_string(char *p, char *s)
{
  int len = strlen(s);
  
  assert( p != NULL );

  p = _put_int(p, len);
  *p++ = ' ';
  strncpy(p, s, len);
  p += len;
  *p++ = '\n';
  return(p);
}


int
len_string(char *s)
{
  int len, slen;

  assert( s != NULL );

  slen = strlen(s);
  len = len_int(slen);

  return(len + slen + 1);
}


char *
get_string(char *p, char **s)
{
  int len;

  assert( p != NULL );

  *s = NULL;

  p = _get_int(p, &len);
  if(*p != ' ')
    return(NULL);
  else
    {
      *s = malloc(len + 1);
      if(*s == NULL)
	return(NULL);
      strncpy(*s, ++p, len);
      (*s)[len] = '\0';
      p += len;
      return( *p++ == '\n' ? p : NULL );
    }
}
