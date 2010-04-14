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

#include "glite/lb/context-int.h"
#include "soap_version.h"
#include "glite/security/glite_gscompat.h"

#include "bk_ws_H.h"
#include "bk_ws_Stub.h"


#if GSOAP_VERSION >= 20709
  #define GFNUM SOAP_TYPE_lbt__genericFault
#else
  #define GFNUM SOAP_TYPE__genericFault
#endif


void edg_wll_ErrToFault(const edg_wll_Context ctx,struct soap *soap)
{
	char	*et,*ed;
	struct SOAP_ENV__Detail	*detail;
#if GSOAP_VERSION >= 20709
	struct lbt__genericFault *f = soap_malloc(soap,sizeof *f);
	struct lbt__genericFault *item = f;
#else
	struct _genericFault *f = soap_malloc(soap,sizeof *f);
	struct lbt__genericFault *item = f->lbe__genericFault = soap_malloc(soap, sizeof *item);
#endif

	memset(item, 0, sizeof(*item));

	item->code = edg_wll_Error(ctx,&et,&ed);
	item->text = soap_malloc(soap,strlen(et)+1);
	strcpy(item->text, et); 
	free(et);
	if (ed) {
		item->description = soap_malloc(soap,strlen(ed)+1);
		strcpy(item->description,ed); 
		free(ed);
	}

	detail = (struct SOAP_ENV__Detail *)soap_faultdetail(soap);
	detail->__type = GFNUM;
#if GSOAP_VERSION >= 20700
	detail->fault = f;
#else
	detail->value = f;
#endif
	detail->__any = NULL;

	soap_receiver_fault(soap,"An error occurred, see detail",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}
