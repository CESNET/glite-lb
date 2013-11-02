#ifndef GLITE_LB_NET_STRING_H
#define GLITE_LB_NET_STRING_H

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


typedef struct il_octet_string {
	int len;
	char *data;
} il_octet_string_t;

char *put_int(char *p, int d);
char *_put_int(char *p, int d);
char *put_string(char *p, il_octet_string_t *s);

const char *get_int(const char *p, int *d);
const char *_get_int(const char *p, int *d);
const char *get_string(const char *p, il_octet_string_t *s);

int len_string(il_octet_string_t *s);
int len_int(int d);

#endif /* GLITE_LB_NET_STRING_H */
