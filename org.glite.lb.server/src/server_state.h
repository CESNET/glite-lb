#ifndef GLITE_LB_SERVER_STATE_H
#define GLITE_LB_SERVER_STATE_H

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


#define EDG_WLL_STATE_DUMP_START	"StartDump"
#define EDG_WLL_STATE_DUMP_END		"EndDump"
#define EDG_WLL_STATE_ANONYMIZATION_SALT	"AnonymizationSalt"

int edg_wll_GetServerState(edg_wll_Context,const char *,char **);
int edg_wll_SetServerState(edg_wll_Context,const char *,const char *);

#endif /* GLITE_LB_SERVER_STATE_H */
