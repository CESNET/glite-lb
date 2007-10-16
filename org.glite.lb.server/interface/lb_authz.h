#ifndef GLITE_LB_LB_AUTHZ_H
#define GLITE_LB_LB_AUTHZ_H

#ident "$Header$"

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

#ifndef NO_GACL

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

#endif /* GLITE_LB_LB_AUTHZ_H */
