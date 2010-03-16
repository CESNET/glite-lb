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

#ifndef LB_AUTHZ_H
#define LB_AUTHZ_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_GACL
#include <gridsite.h>
#endif 

typedef struct _edg_wll_Acl {
#ifndef NO_GACL
	GRSTgaclAcl	*value;
#else
	void	*value;	/* XXX */
#endif
	char	*string;
} _edg_wll_Acl;
typedef struct _edg_wll_Acl *edg_wll_Acl;

# ifndef NO_GACL

extern int
edg_wll_DecodeACL(char *, GRSTgaclAcl **);

extern int
edg_wll_EncodeACL(GRSTgaclAcl *, char **);

#else

extern int
edg_wll_DecodeACL(char *, void **);

extern int
edg_wll_EncodeACL(void *, char **);

#endif /* NO_GACL */

/* we have NO_VOMS||NO_GACL placeholders for following routines */

extern int
edg_wll_InitAcl(edg_wll_Acl *);

extern void
edg_wll_FreeAcl(edg_wll_Acl);

extern int
edg_wll_UpdateACL(edg_wll_Context, edg_wlc_JobId, char *, int, int, int, int);

extern int
edg_wll_CheckACL(edg_wll_Context, edg_wll_Acl, int);

extern int
edg_wll_GetACL(edg_wll_Context, edg_wlc_JobId, edg_wll_Acl *);

extern int
edg_wll_SetVomsGroups(edg_wll_Context, edg_wll_GssConnection *, char *, char *, char *, char *);

extern void
edg_wll_FreeVomsGroups(edg_wll_VomsGroups *);

#ifdef __cplusplus
}
#endif

#endif
