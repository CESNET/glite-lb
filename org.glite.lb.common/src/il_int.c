#ident "$Header$"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "glite/lb/il_string.h"

char *
_put_int(char *p, int d)
{
  char buf[32];
  int len;
  
  assert( p != NULL );

  snprintf(buf, sizeof(buf), "%d", d);
  len = strlen(buf);
  strncpy(p, buf, len);
  return(p + len);
}


char *
put_int(char *p, int d)
{
  assert( p != NULL );

  p = _put_int(p, d);
  *p++ = '\n';
  return(p);
}


char *
_get_int(char *p, int *d)
{
   char *end;

  assert( p != NULL );
  assert( d != NULL );

  *d = strtol(p, &end, 10);
  return(end);
}
  

char *
get_int(char *p, int *d)
{
  assert( p != NULL );
  assert( d != NULL );

  p = _get_int(p, d);
  if(*p != '\n') 
    return(NULL);
  else
    return(p + 1);
}


int 
len_int(int d)
{
  char buffer[256];

  return(put_int(buffer, d) - buffer);
}
