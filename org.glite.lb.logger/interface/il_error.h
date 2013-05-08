#ifndef IL_ERROR_H
#define IL_ERROR_H

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


#include <syslog.h>

enum err_code_maj { /* minor =                   */
  IL_OK,            /*     0                     */
  IL_SYS,           /*     errno                 */
  IL_NOMEM,         /*     ENOMEM                */
  IL_PROTO,         /*     LB_*                  */
  IL_LBAPI,         /*     dgLBErrCode           */
  IL_DGGSS,         /*     EDG_WLL_GSS_*         */
  IL_HOST,          /*     h_errno               */
  IL_DL             /*     dlerror               */
};

struct error_inf {
  int  code_maj;
  long code_min;
  char *msg;
};

int init_errors();
int set_error(int, long, const char *);
int clear_error();
int error_get_maj();
long error_get_min();
char *error_get_msg();

#endif
