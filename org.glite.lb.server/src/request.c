#ident "$Header$"
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "glite/lb/il_string.h"
#include "glite/lb/il_msg.h"
#include "glite/lb/context-int.h"

#include "store.h"
#include "db_supp.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif


int 
handle_il_message(edg_wll_Context ctx,char *buf)
{
	il_octet_string_t event;
  int ret;

  edg_wll_ResetError(ctx);

  ret = decode_il_msg(&event, buf);
  if(ret < 0) {
    edg_wll_SetError(ctx,EDG_WLL_IL_PROTO,"decoding event string failed");
    return EDG_WLL_IL_PROTO;
  }

  ret = db_store(ctx, event.data);

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

  switch(err_code = edg_wll_Error(ctx,NULL,&err_msg)) {

  case 0:
    err_code = LB_OK;
    break;

  case ENOMEM:
    err_code = LB_NOMEM;
    break;

  case EPERM:
	  err_code = LB_PERM;
	  break;

  case EDG_WLL_IL_PROTO:
    err_code = LB_PROTO;
    break;
    
  default:
	  if(err_code > EDG_WLL_ERROR_BASE) {
		  /* that would mean parse error, do not bother sending it again */
		  err_code = LB_PROTO;
	  } else {
		  err_code = LB_DBERR;
	  }
	  err_code_min = edg_wll_Error(ctx,NULL,NULL);
	  break;

  } 

  if (!err_msg) err_msg=strdup("OK");
  
  len = encode_il_reply(buf, err_code, err_code_min, err_msg);
  free(err_msg);
  return(len);
}


