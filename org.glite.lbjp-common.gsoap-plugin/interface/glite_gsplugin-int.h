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

#ifndef GLITE_SECURITY_GSOAP_PLUGIN_INTERNAL_H
#define GLITE_SECURITY_GSOAP_PLUGIN_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "glite/security/glite_gss.h"

struct _glite_gsplugin_ctx {
	struct timeval			_timeout, *timeout;

	char				   *error_msg;

	edg_wll_GssConnection  *connection;
	edg_wll_GssCred			cred;
	int				internal_connection;
	int				internal_credentials;

	void				   *user_data;
};


#ifdef __cplusplus
}
#endif

#endif
