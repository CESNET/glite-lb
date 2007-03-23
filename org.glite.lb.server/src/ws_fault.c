#include <string.h>

#include "glite/lb/context-int.h"
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


void edg_wll_ErrToFault(const edg_wll_Context ctx,struct soap *soap)
{
	char	*et,*ed;
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
#if GSOAP_VERSION >= 20709
	struct lbt__genericFault *f = soap_malloc(soap,sizeof *f);
#else
	struct _genericFault *f = soap_malloc(soap,sizeof *f);
#endif

	f->GFITEM = soap_malloc(soap,sizeof *f->GFITEM);
	memset(f->GFITEM, 0, sizeof(*f->GFITEM));

	f->GFITEM->code = edg_wll_Error(ctx,&et,&ed);
	f->GFITEM->text = soap_malloc(soap,strlen(et)+1);
	strcpy(f->GFITEM->text,et); 
	free(et);
	if (ed) {
		f->GFITEM->description = soap_malloc(soap,strlen(ed)+1);
		strcpy(f->GFITEM->description,ed); 
		free(ed);
	}

	detail->__type = GFNUM;
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
	struct SOAP_ENV__Detail	*detail;
	struct lbt__genericFault	*f;

	if (!soap->fault) {
		edg_wll_SetError(ctx,EINVAL,"SOAP: (no error info)");
		return;
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
		edg_wll_SetError(ctx,f->code,f->description);
	}
	else {
		char	*s;

		asprintf(&s,"SOAP: %s", soap->version == 2 ?
			GLITE_SECURITY_GSOAP_REASON(soap) : soap->fault->faultstring);
		edg_wll_SetError(ctx,EINVAL,s);
		free(s);
	}
}
