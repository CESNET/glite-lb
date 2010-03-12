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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>

#define edg_wll_gss_read_full(a,b,c,d,e,f)  test_edg_wll_gss_read_full(a,b,c,d,e,f)
#define edg_wll_gss_write_full(a,b,c,d,e,f) test_edg_wll_gss_write_full(a,b,c,d,e,f)
#define edg_wll_GssConnection               int

#include "logd_proto.h"
#include "glite/lb/context-int.h"
#include "glite/lb/escape.h"
#include "glite/lb/events_parse.h"

int
test_edg_wll_gss_read_full(int *fd,
			   void *buf,
			   size_t bufsize,
			   struct timeval *timeout,
			   size_t *total,
			   edg_wll_GssStatus *code) 
{
  *total = read(*fd, buf, bufsize);
  return(*total < 0 ? *total : 0);
}

int
test_edg_wll_gss_write_full(int *fd,
			    const void *buf,
			    size_t bufsize,
			    struct timeval *timeout,
			    size_t *total,
			    edg_wll_GssStatus *code) 
{
  return(0);
}

#include "logd_proto.c"
