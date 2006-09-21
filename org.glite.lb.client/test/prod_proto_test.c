#define edg_wll_gss_read_full(a,b,c,d,e,f)  test_edg_wll_gss_read_full(a,b,c,d,e,f)
#define edg_wll_gss_write_full(a,b,c,d,e,f) test_edg_wll_gss_write_full(a,b,c,d,e,f)

#include "glite/lb-utils/escape.h"
#include "prod_proto.h"
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
