#include <string.h>

#include "glite/lb/context-int.h"

#include "bk_ws_H.h"
#include "bk_ws_Stub.h"


void edg_wll_ErrToFault(const edg_wll_Context ctx,struct soap *soap)
{
	char	*et,*ed;
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
	struct _GenericLBFault *f = soap_malloc(soap,sizeof *f);


	f->edgwll__GenericLBFault = soap_malloc(soap,sizeof *f->edgwll__GenericLBFault);

	f->edgwll__GenericLBFault->code = edg_wll_Error(ctx,&et,&ed);
	f->edgwll__GenericLBFault->text = soap_malloc(soap,strlen(et)+1);
	strcpy(f->edgwll__GenericLBFault->text,et); 
	free(et);
	f->edgwll__GenericLBFault->description = soap_malloc(soap,strlen(ed)+1);
	strcpy(f->edgwll__GenericLBFault->description,ed); 
	free(ed);

	detail->__type = SOAP_TYPE__GenericLBFault;
	detail->value = f;
	detail->__any = NULL;

	soap_receiver_fault(soap,"shit",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}


void edg_wll_FaultToErr(const struct soap *soap,edg_wll_Context ctx)
{
	struct SOAP_ENV__Detail	*detail = soap->version == 2 ?
		soap->fault->SOAP_ENV__Detail : soap->fault->detail;

	struct edgwll__GenericLBFaultType	*f;

	if (detail->__type == SOAP_TYPE__GenericLBFault) {
       		f = ((struct _GenericLBFault *) detail->value)
			->edgwll__GenericLBFault;
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
