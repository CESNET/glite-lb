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

#ifndef GLITE_LB_PLAIN_IO_H
#define GLITE_LB_PLAIN_IO_H

#ifdef BUILDING_LB_COMMON
#include "padstruct.h"
#else
#include "glite/lb/padstruct.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

glite_lb_padded_struct(_edg_wll_PlainConnection,6,
	int sock;
	char *buf;
	size_t bufSize;
	size_t bufUse;
)
typedef struct _edg_wll_PlainConnection edg_wll_PlainConnection;

	
int edg_wll_plain_accept(
	int sock,
	edg_wll_PlainConnection *conn);

int edg_wll_plain_close(
	edg_wll_PlainConnection *conn);

int edg_wll_plain_read(
	edg_wll_PlainConnection *conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_read_full(
	edg_wll_PlainConnection *conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_write_full(
	edg_wll_PlainConnection *conn,
	const void *buf,
	size_t bufsz,
	struct timeval *timeout);

#ifdef __cplusplus
} 
#endif
	
#endif /* GLITE_LB_PLAIN_IO_H */
