#ident "$Header$"

#include "il_string.h"

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

int 
receive_msg(int sd, char **ucs, char **event)
{
  char buffer[17];
  char *p, *q, *msg;
  int len;

  len = read(sd, buffer, 17);

  if(buffer[16] != '\n') {
    printf("Error in header!\n");
    goto err;
  }

  sscanf(buffer, "%d", &len);
  if(len > MAXLEN) {
    printf("Message too long!\n");
    goto err;
  }
  p = msg = malloc(len+1);
  if(p == NULL) {
    printf("Error allocating %d bytes\n", len+1);
    goto err;
  }

  read(sd, p, len);
  p[len] = 0;

  if((q = get_string(p, ucs)) == NULL) {
    printf("Protocol error at %s\n", p);
    goto err;
  }
  p = q;
  if((q = get_string(p, event)) == NULL) {
    printf("Protocol error at %s\n", p);
    goto err;
  }

  free(msg);
  return(0);

 err: 
  
  return(-1);
}
