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

#include <string.h>
#include <stdio.h>

#include "soap_version.h"
#include "glite/security/glite_gscompat.h"

#include "bk_ws_H.h"
#include "bk_ws_Stub.h"


#if GSOAP_VERSION >= 20709
  #define GFITEM reason
  #define GFNUM SOAP_TYPE_lbt__genericFault
#else
  #define GFITEM lbe__genericFault
  #define GFNUM SOAP_TYPE__genericFault
#endif


int glite_lb_FaultToErr(const struct soap *soap,char **text)
{
	struct SOAP_ENV__Detail	*detail;
	struct lbt__genericFault	*f;

	if (!soap->fault) {
		*text = NULL;
		return EINVAL;
	}

	detail = soap->version == 2 ? soap->fault->SOAP_ENV__Detail : soap->fault->detail;
	if (detail->__type == GFNUM) {
#if GSOAP_VERSION >= 20709
		f = detail->lbe__genericFault;
#elif GSOAP_VERSION >= 20700
		f = ((struct _genericFault *) detail->fault)
			->lbe__genericFault;
#else
		f = ((struct _genericFault *) detail->value)
			->lbe__genericFault;
#endif
		*text = strdup(f->description);
		return f->code;
	}
	else {
		asprintf(text,"SOAP: %s", soap->version == 2 ?
			GLITE_SECURITY_GSOAP_REASON(soap) : soap->fault->faultstring);
		return EINVAL;
	}
}
