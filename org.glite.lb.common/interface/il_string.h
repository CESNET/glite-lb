#ifndef GLITE_LB_NET_STRING_H
#define GLITE_LB_NET_STRING_H

#ident "$Header$"

typedef struct il_octet_string {
	int len;
	char *data;
} il_octet_string_t;

char *put_int(char *p, int d);
char *_put_int(char *p, int d);
char *put_string(char *p, il_octet_string_t *s);

char *get_int(char *p, int *d);
char *_get_int(char *p, int *d);
char *get_string(char *p, il_octet_string_t *s);

int len_string(il_octet_string_t *s);
int len_int(int d);

#endif /* GLITE_LB_NET_STRING_H */
