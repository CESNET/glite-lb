#ifndef LB_AUTHZ_H
#define LB_AUTHZ_H

#include <stdio.h>
#include <gacl.h>

#include "glite/lb/context-int.h"

typedef struct _edg_wll_Acl {
	GACLacl	*value;
	char	*string;
} _edg_wll_Acl;
typedef struct _edg_wll_Acl *edg_wll_Acl;

extern int
edg_wll_InitAcl(edg_wll_Acl *);

extern void
edg_wll_FreeAcl(edg_wll_Acl);

extern int
edg_wll_GetVomsGroups(edg_wll_Context, char *, char *);

extern void
edg_wll_FreeVomsGroups(edg_wll_VomsGroups *);

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

#endif
