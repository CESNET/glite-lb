#ifndef GLITE_LB_LB_AUTHZ_H
#define GLITE_LB_LB_AUTHZ_H

#ident "$Header$"
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


#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_GACL
#include <gridsite.h>
#endif
#include <voms/voms_apic.h>

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
edg_wll_UpdateACL(edg_wll_Context, glite_jobid_const_t, char *, int, int, int, int);

extern int
edg_wll_CheckACL(edg_wll_Context, edg_wll_Acl, int);

extern int
edg_wll_CheckACL_princ(edg_wll_Context, edg_wll_Acl, int, edg_wll_GssPrincipal);

extern int
edg_wll_GetACL(edg_wll_Context, glite_jobid_const_t, edg_wll_Acl *);

extern int
edg_wll_SetVomsGroups(edg_wll_Context, edg_wll_GssConnection *, char *, char *, char *, char *);

extern void
edg_wll_FreeVomsGroups(edg_wll_VomsGroups *);

extern int
check_store_authz(edg_wll_Context ctx, edg_wll_Event *ev);

int edg_wll_amIroot(const char *subj, char **fqans,edg_wll_authz_policy policy);

edg_wll_authz_policy
edg_wll_get_server_policy();

int
edg_wll_get_fqans(edg_wll_Context ctx, struct vomsdata *voms_info,
                  char ***fqans);
int
edg_wll_acl_print(edg_wll_Context ctx, edg_wll_Acl a, char **policy);

int
check_jobstat_authz(edg_wll_Context ctx,
                    const edg_wll_JobStat *stat,
                    int job_flags,
                    edg_wll_Acl acl,
                    struct _edg_wll_GssPrincipal_data *peer,
                    int *authz_flags);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_LB_AUTHZ_H */
