#ifndef GLITE_LB_HTTP_H
#define GLITE_LB_HTTP_H

#ident "$Header$"

#include "glite/lb/context.h"

int edg_wll_AcceptHTTP(edg_wll_Context ctx, char **body, char **resp, char ***hdrOut, char **bodyOut, int *httpErr);
int edg_wll_DoneHTTP(edg_wll_Context ctx, char *resp, char **hdrOut, char *bodyOut);

#endif /* GLITE_LB_HTTP_H */
