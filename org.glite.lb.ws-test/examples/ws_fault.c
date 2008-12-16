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
