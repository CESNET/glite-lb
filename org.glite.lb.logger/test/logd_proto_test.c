#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>

#include "logd_proto.h"
#include "glite/lb/context-int.h"
#include "glite/lb/escape.h"
#include "glite/lb/events_parse.h"

#define edg_wll_gss_read_full(a,b,c,d,e,f)  test_edg_wll_gss_read_full(a,b,c,d,e,f)
#define edg_wll_GssConnection               int

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

#include "logd_proto.c"
