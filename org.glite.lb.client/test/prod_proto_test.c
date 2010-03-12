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

#define edg_wll_gss_read_full(a,b,c,d,e,f)  test_edg_wll_gss_read_full(a,b,c,d,e,f)
#define edg_wll_gss_write_full(a,b,c,d,e,f) test_edg_wll_gss_write_full(a,b,c,d,e,f)

#include "glite/lb/prod_proto.h"
#include "glite/lb/producer.h"

/* virtual read will return all zeroes (answer from logger always without error) */
int
test_edg_wll_gss_read_full(edg_wll_GssConnection *con,
			   void *buf,
			   size_t bufsize,
			   struct timeval *timeout,
			   size_t *total,
			   edg_wll_GssStatus *code) 
{
  code->major_status = 0;
  code->minor_status = 0;
  if (bufsize > 0) memset(buf, 0, bufsize);
  return bufsize;
}

int
test_edg_wll_gss_write_full(edg_wll_GssConnection *con,
			    const void *buf,
			    size_t bufsize,
			    struct timeval *timeout,
			    size_t *total,
			    edg_wll_GssStatus *code) 
{
  *total = write(*(int *)con, buf, bufsize);
  code->major_status = 0;
  code->minor_status = *total < 0 ? *total : 0;
  return *total < 0 ? *total : 0;
}

#include "prod_proto.c"
