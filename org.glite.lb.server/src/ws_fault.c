#include <string.h>

#include "glite/lb/context-int.h"
#include "soap_version.h"

#include "bk_ws_H.h"
#include "bk_ws_Stub.h"


void edg_wll_ErrToFault(const edg_wll_Context ctx,struct soap *soap)
{
	char	*et,*ed;
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
	struct _genericFault *f = soap_malloc(soap,sizeof *f);


	f->lbe__genericFault = soap_malloc(soap,sizeof *f->lbe__genericFault);

	f->lbe__genericFault->code = edg_wll_Error(ctx,&et,&ed);
	f->lbe__genericFault->text = soap_malloc(soap,strlen(et)+1);
	strcpy(f->lbe__genericFault->text,et); 
	free(et);
	f->lbe__genericFault->description = soap_malloc(soap,strlen(ed)+1);
	strcpy(f->lbe__genericFault->description,ed); 
	free(ed);

	detail->__type = SOAP_TYPE__genericFault;
#if GSOAP_VERSION >= 20700
	detail->fault = f;
#else
	detail->value = f;
#endif
	detail->__any = NULL;

	soap_receiver_fault(soap,"shit",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}


void edg_wll_FaultToErr(const struct soap *soap,edg_wll_Context ctx)
{
	struct SOAP_ENV__Detail	*detail = soap->version == 2 ?
		soap->fault->SOAP_ENV__Detail : soap->fault->detail;

	struct lbt__genericFault	*f;

	if (detail->__type == SOAP_TYPE__genericFault) {
#if GSOAP_VERSION >= 20700
		f = ((struct _genericFault *) detail->fault)
			->lbe__genericFault;
#else
		f = ((struct _genericFault *) detail->value)
			->lbe__genericFault;
#endif
		edg_wll_SetError(ctx,f->code,f->description);
	}
	else {
		char	*s;

		asprintf(&s,"SOAP: %s", soap->version == 2 ?
			soap->fault->SOAP_ENV__Reason : soap->fault->faultstring);
		edg_wll_SetError(ctx,EINVAL,s);
		free(s);
	}
}
