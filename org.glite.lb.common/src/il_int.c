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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "il_string.h"

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


const char *
_get_int(const char *p, int *d)
{
   char *end;

  assert( p != NULL );
  assert( d != NULL );

  *d = strtol(p, &end, 10);
  return(end);
}
  

const char *
get_int(const char *p, int *d)
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
