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

#ifndef GLITE_SECURITY_GSOAP_PLUGIN_H
#define GLITE_SECURITY_GSOAP_PLUGIN_H

#include <stdsoap2.h>

#include "glite/security/glite_gss.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_ID		"GLITE_GSOAP_PLUGIN"

typedef struct _glite_gsplugin_ctx *glite_gsplugin_Context;

extern int glite_gsplugin_init_context(glite_gsplugin_Context *);
extern int glite_gsplugin_free_context(glite_gsplugin_Context);
extern glite_gsplugin_Context glite_gsplugin_get_context(struct soap *);
extern void *glite_gsplugin_get_udata(struct soap *);
extern void glite_gsplugin_set_udata(struct soap *, void *);

extern void glite_gsplugin_set_timeout(glite_gsplugin_Context, struct timeval const *);
extern int glite_gsplugin_set_credential(glite_gsplugin_Context, const char *, const char *);
extern void glite_gsplugin_use_credential(glite_gsplugin_Context, edg_wll_GssCred);
extern int glite_gsplugin_set_connection(glite_gsplugin_Context, edg_wll_GssConnection *);

extern int glite_gsplugin(struct soap *, struct soap_plugin *, void *);
extern char *glite_gsplugin_errdesc(struct soap *);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_SECURITY_GSOAP_PLUGIN_H */
