#ifndef IL_ERROR_H
#define IL_ERROR_H

#ident "$Header$"

#include <syslog.h>

enum err_code_maj { /* minor =                   */
  IL_OK,            /*     0                     */
  IL_SYS,           /*     errno                 */
  IL_NOMEM,         /*     ENOMEM                */
  IL_PROTO,         /*     LB_*                  */
  IL_LBAPI,         /*     dgLBErrCode           */
  IL_DGGSS,         /*     EDG_WLL_GSS_*         */
  IL_HOST           /*     h_errno               */
};

struct error_inf {
  int  code_maj;
  long code_min;
  char *msg;
};

int init_errors(int);
int set_error(int, long, char *);
int clear_error();
int error_get_maj();
long error_get_min();
char *error_get_msg();

int il_log(int, char *, ...);

#endif
