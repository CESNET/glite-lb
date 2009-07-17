#ident "$Header$"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#ifdef LB_PROF
#include <sys/gmon.h>
extern void _start (void), etext (void);
#endif

#include "glite/security/glite_gss.h"
#include "il_error.h"


static pthread_key_t err_key;

static int IL_ERR_MSG_LEN = 1024;

static
void
error_key_delete(void *err)
{
  if(((struct error_inf*)err)->msg)
    free(((struct error_inf*)err)->msg);
  free(err);
}

static 
void
error_key_create()
{
  pthread_key_create(&err_key, error_key_delete);
}

static 
struct error_inf *
error_get_err ()
{
  struct error_inf *err;

  /* get thread specific error structure */
  err = (struct error_inf *)pthread_getspecific(err_key);
  assert(err != NULL);

  return(err);
}


int
init_errors()
{
  static pthread_once_t error_once = PTHREAD_ONCE_INIT;
  struct error_inf *err;

  /* create the key for thread specific error only once */
  pthread_once(&error_once, error_key_create);
  
  /* there is no thread error yet, try to create one */
  if((err = (struct error_inf *)malloc(sizeof(*err)))) {
    /* allocation successfull, make it thread specific data */
    if(pthread_setspecific(err_key, err)) {
	free(err);
	return(-1);
    }
  } else 
    return(-1);

  err->code_maj = 0;
  err->code_min = 0;
  err->msg = malloc(IL_ERR_MSG_LEN + 1);
  if(err->msg == NULL) 
	  return(-1);

#ifdef LB_PROF
  monstartup((u_long)&_start, (u_long)&etext);
#endif

  return(0);
}

int 
set_error(int code, long minor, char *msg)
{
  struct error_inf *err;

  err = error_get_err();

  err->code_maj = code;
  err->code_min = minor;

  switch(code) {

  case IL_SYS:
    snprintf(err->msg, IL_ERR_MSG_LEN, "%s: %s", msg, strerror(err->code_min));
    break;

  case IL_HOST:
    snprintf(err->msg, IL_ERR_MSG_LEN, "%s: %s", msg, hstrerror(err->code_min));
    break;

  case IL_DGGSS:
    switch(err->code_min) {

    case EDG_WLL_GSS_ERROR_GSS:
      snprintf(err->msg, IL_ERR_MSG_LEN, "%s", msg);
      break;

    case EDG_WLL_GSS_ERROR_TIMEOUT:
      snprintf(err->msg, IL_ERR_MSG_LEN, "%s: Timeout in GSS connection.", msg);
      break;

    case EDG_WLL_GSS_ERROR_EOF:
      snprintf(err->msg, IL_ERR_MSG_LEN, "%s: Connection lost.", msg);
      break;

    case EDG_WLL_GSS_ERROR_ERRNO:
      snprintf(err->msg, IL_ERR_MSG_LEN, "%s: %s", msg, strerror(errno));
      break;
      
    case EDG_WLL_GSS_ERROR_HERRNO:
      snprintf(err->msg, IL_ERR_MSG_LEN, "%s: %s", msg, hstrerror(errno));
      break;
    }

  default:
	  strncpy(err->msg, msg, IL_ERR_MSG_LEN);
  }

  err->msg[IL_ERR_MSG_LEN] = 0; /* OK, malloc()ed IL_ERR_MSG_LEN + 1 */

  return(code);
}


int 
clear_error() {
  struct error_inf *err;

  err = error_get_err();

  err->code_maj = IL_OK;
  err->code_min = 0;
  *(err->msg) = 0;

  return(0);
} 


int
error_get_maj()
{
  struct error_inf *err;

  err = error_get_err();

  return(err->code_maj);
}


long
error_get_min()
{
  struct error_inf *err;

  err = error_get_err();

  return(err->code_min);
}


char * 
error_get_msg()
{
  struct error_inf *err;

  err = error_get_err();

  return(err->msg);
}
