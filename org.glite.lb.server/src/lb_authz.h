#ifndef LB_AUTHZ_H
#define LB_AUTHZ_H

#ifndef NO_GACL
#include <gacl.h>
#endif 

typedef struct _edg_wll_Acl {
#ifndef NO_GACL
	GACLacl	*value;
#else
	void	*value;	/* XXX */
#endif
	char	*string;
} _edg_wll_Acl;
typedef struct _edg_wll_Acl *edg_wll_Acl;

#ifndef NO_VOMS

# ifndef NO_GACL

#include <stdio.h>

#include "glite/lb/context-int.h"

extern int
edg_wll_InitAcl(edg_wll_Acl *);

extern void
edg_wll_FreeAcl(edg_wll_Acl);

extern int
edg_wll_UpdateACL(edg_wll_Context, edg_wlc_JobId, char *, int, int, int, int);

extern int
edg_wll_CheckACL(edg_wll_Context, edg_wll_Acl, int);

extern int
edg_wll_DecodeACL(char *, GACLacl **);

extern int
edg_wll_EncodeACL(GACLacl *, char **);

extern int
edg_wll_GetACL(edg_wll_Context, edg_wlc_JobId, edg_wll_Acl *);

#endif /* NO_GACL */

extern int
edg_wll_GetVomsGroups(edg_wll_Context, char *, char *);

extern void
edg_wll_FreeVomsGroups(edg_wll_VomsGroups *);

#endif /* NO_VOMS */

#endif
