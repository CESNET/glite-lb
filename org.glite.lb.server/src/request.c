#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glite/lb/il_string.h"
#include "glite/lb/context-int.h"

#include "store.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif
    
int 
handle_request(edg_wll_Context ctx,char *buf, int len UNUSED_VAR)
{
  char *p = buf;
  char *event, *ucs;
  int ret;

  edg_wll_ResetError(ctx);

  p = get_string(p, &ucs);
  if(p == NULL) return edg_wll_SetError(ctx,EDG_WLL_IL_PROTO,"reading UCS");

  p = get_string(p, &event);
  if(p == NULL) {
    edg_wll_SetError(ctx,EDG_WLL_IL_PROTO,"reading event string");
    if(ucs) free(ucs);
    return EDG_WLL_IL_PROTO;
  }

  ret = db_store(ctx,ucs, event);

  if(ucs)
    free(ucs);
  if(event)
    free(event);

  return(ret);
}


int 
create_reply(const edg_wll_Context ctx,char *buf, int max_len)
{
  int len, err_code, err_code_min;
  char *p;
  char *err_msg;

  err_code_min = 0;

  switch(edg_wll_Error(ctx,NULL,&err_msg)) {

  case 0:
    err_code = LB_OK;
    break;

  case ENOMEM:
    err_code = LB_NOMEM;
    break;

  case EDG_WLL_IL_PROTO:
    err_code = LB_PROTO;
    break;
    
  default:
    err_code = LB_DBERR;
    err_code_min = edg_wll_Error(ctx,NULL,NULL);
    break;

  } 

  if (!err_msg) err_msg=strdup("OK");
  
  len = 17 + len_int(err_code) + len_int(err_code_min) + len_string(err_msg);
  if(len > max_len) {
    free(err_msg);
    return(0);
  }

  snprintf(buf, max_len, "%16d\n", len - 17);
  p = buf + 17;
  p = put_int(p, err_code);
  p = put_int(p, err_code_min);
  p = put_string(p, err_msg);
  free(err_msg);
  
  return(len);
}


