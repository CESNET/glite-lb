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


const char *
get_string(const char *p, il_octet_string_t *s)
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
