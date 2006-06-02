#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glite/lb/il_string.h"
#include "glite/lb/il_msg.h"
#include "glite/lb/context-int.h"

#include "store.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif
    
int 
handle_request(edg_wll_Context ctx,char *buf)
{
	il_octet_string_t event;
  int ret;

  edg_wll_ResetError(ctx);

  ret = decode_il_msg(&event, buf);
  if(ret < 0) {
    edg_wll_SetError(ctx,EDG_WLL_IL_PROTO,"decoding event string failed");
    return EDG_WLL_IL_PROTO;
  }

  ret = db_store(ctx, "NOT USED", event.data);

  if(event.data)
    free(event.data);

  return(ret);
}


int 
create_reply(const edg_wll_Context ctx, char **buf)
{
  int len, err_code, err_code_min;
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
  
  len = encode_il_reply(buf, err_code, err_code_min, err_msg);
  free(err_msg);
  return(len);
}


