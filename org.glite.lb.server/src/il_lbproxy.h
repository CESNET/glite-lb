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

#ifndef IL_LBPROXY_H
#define IL_LBPROXY_H

#ifdef __cplusplus
#extern "C" {
#endif

extern char *lbproxy_ilog_socket_path;
extern char *lbproxy_ilog_file_prefix;

int edg_wll_EventSendProxy(edg_wll_Context ctx, const edg_wlc_JobId jobid, const char *event);

#ifdef __cplusplus
}
#endif

#endif
