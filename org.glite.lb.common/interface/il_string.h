#ifndef NET_STRING_H
#define NET_STRING_H

#ident "$Header$"

#define MAXLEN 1024

char *put_int(char *p, int d);
char *_put_int(char *p, int d);
char *put_string(char *p, char *s);

char *get_int(char *p, int *d);
char *_get_int(char *p, int *d);
char *get_string(char *p, char **s);

int len_string(char *s);
int len_int(int d);

enum {
  LB_OK    = 0,
  LB_NOMEM = 200,
  LB_PROTO = 400,
  LB_AUTH  = 500,
  LB_PERM  = 600,
  LB_DBERR = 700,
  LB_SYS   = 800,
  LB_TIME  = 900
};

#endif
