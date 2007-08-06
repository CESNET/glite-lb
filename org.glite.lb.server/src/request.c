#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "glite/lb/il_string.h"
#include "glite/lb/il_msg.h"
#include "glite/lb/context-int.h"

#include "store.h"
#include "lbs_db.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif


int
trans_db_store(edg_wll_Context ctx, char *event_data, edg_wll_Event *e, intJobStat *is)
{
  int ret;

  if ((ret = edg_wll_Transaction(ctx) != 0)) goto err;

  if (e) ret = db_parent_store(ctx, e, is);
  else ret = db_store(ctx, "NOT USED", event_data);

  if (ret == 0) {
    if ((ret = edg_wll_Commit(ctx)) != 0) goto err;
  } else {
    edg_wll_Rollback(ctx);
  }

err:
  return(ret);
}
    
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

  ret = trans_db_store(ctx, event.data, NULL, NULL);

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


