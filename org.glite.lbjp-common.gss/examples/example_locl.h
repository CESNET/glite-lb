#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <glite_gss.h>

void
print_gss_err(int ret, edg_wll_GssStatus *gss_ret, char *msg);
