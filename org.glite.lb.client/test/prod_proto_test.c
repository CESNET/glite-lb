#define edg_wll_gss_read_full(a,b,c,d,e,f)  test_edg_wll_gss_read_full(a,b,c,d,e,f)
#define edg_wll_gss_write_full(a,b,c,d,e,f) test_edg_wll_gss_write_full(a,b,c,d,e,f)
#define edg_wll_GssConnection               int

#include "prod_proto.h"
#include "glite/lb/producer.h"
#include "glite/lb/escape.h"
#include "glite/lb/lb_gss.h"

int
test_edg_wll_gss_read_full(int *fd,
			   void *buf,
			   size_t bufsize,
			   struct timeval *timeout,
			   size_t *total,
			   edg_wll_GssStatus *code) 
{
  return(0);
}

int
test_edg_wll_gss_write_full(int *fd,
			    const void *buf,
			    size_t bufsize,
			    struct timeval *timeout,
			    size_t *total,
			    edg_wll_GssStatus *code) 
{
  *total = write(*fd, buf, bufsize);
  return(*total < 0 ? *total : 0);
}

#include "prod_proto.c"
