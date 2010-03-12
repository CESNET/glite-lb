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


#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "glite/lb/context-int.h"
#include "lb_authz.h"
#include "glite/lbu/log.h"

#ifndef NO_VOMS

#include <libxml/parser.h> 
#undef WITHOUT_TRIO

#include "glite/jobid/strmd5.h"
#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "db_supp.h"
#include <glite/security/lcas/lcas_pem.h>

#include "glite/security/voms/voms_apic.h"

/* XXX should be defined in gridsite-gacl.h */
GRSTgaclEntry *GACLparseEntry(xmlNodePtr cur);

extern char *server_key;
extern char *server_cert;

static int 
get_fqans(edg_wll_Context ctx, struct vomsdata *voms_info,
      	  char ***fqans)
{
   struct voms **voms_cert = NULL;
   char **f, **attrs, **tmp;
   int num;

   attrs = NULL;
   num = 0;

   for (voms_cert = voms_info->data; voms_cert && *voms_cert; voms_cert++) {
      for (f = (*voms_cert)->fqan; f && *f; f++) {
         tmp = realloc(attrs, (num + 1) * sizeof(*attrs));
         if (tmp == NULL) {
            free(attrs);
            return ENOMEM;
         }
         attrs = tmp;
         attrs[num++] = strdup(*f);
      }
   }
   if (attrs) {
      tmp = realloc(attrs, (num + 1) * sizeof(*attrs));
      if (tmp == NULL) {
         free(attrs);
         return ENOMEM;
      }
      attrs = tmp;
      attrs[num++] = NULL;
   }
   
   *fqans = attrs;
   return 0;
}

static int
add_groups(edg_wll_Context ctx, struct voms *voms_cert, char *vo_name,
      	   edg_wll_VomsGroups *groups)
{
   struct data **voms_data;
   edg_wll_VomsGroup *tmp = NULL;

   if (voms_cert->type != TYPE_STD) {
      edg_wll_SetError(ctx, EINVAL, "not supported VOMS certificate type");
      return EINVAL;
   }

   for (voms_data = voms_cert->std; voms_data && *voms_data; voms_data++) {
      if ((*voms_data)->group && *(*voms_data)->group) {
	 tmp = realloc(groups->val, (groups->len + 1) * sizeof(*groups->val));
	 if (tmp == NULL)
	    return edg_wll_SetError(ctx, ENOMEM, "not enough memory");
	 groups->val = tmp;
	 groups->val[groups->len].vo = strdup(vo_name);
	 groups->val[groups->len].name = strdup((*voms_data)->group);
	 groups->len++;
      }
   }
   return 0;
}

static int 
get_groups(edg_wll_Context ctx, struct vomsdata *voms_info,
      	   edg_wll_VomsGroups *res_groups)
{
   struct voms **voms_cert = NULL;
   edg_wll_VomsGroups groups;
   int ret;

   memset(&groups, 0, sizeof(groups));

   for (voms_cert = voms_info->data; voms_cert && *voms_cert; voms_cert++) {
      if ((*voms_cert)->voname) {
	 ret = add_groups(ctx, *voms_cert, (*voms_cert)->voname, &groups);
	 if (ret) {
	    edg_wll_FreeVomsGroups(&groups);
	    return ret;
	 }
      }
   }

   res_groups->len = groups.len;
   res_groups->val = groups.val;
   return 0;
}

int
edg_wll_SetVomsGroups(edg_wll_Context ctx, edg_wll_GssConnection *gss, char *server_cert, char *server_key, char *voms_dir, char *ca_dir)
{
   int ret;
   int err = 0;
   struct vomsdata *voms_info = NULL;
   edg_wll_GssPrincipal principal;
   edg_wll_GssStatus gss_code;


   /* XXX DK: correct cleanup ?? */
   memset (&ctx->vomsGroups, 0, sizeof(ctx->vomsGroups));
   edg_wll_ResetError(ctx);

   if (ctx->fqans) {
      char **f;
      for (f = ctx->fqans; f && *f; f++)
         free(*f);
      free(ctx->fqans);
      ctx->fqans = NULL;
   }

   ret = edg_wll_gss_get_client_conn(gss, &principal, &gss_code);
   if (ret) {
	if (ret == EDG_WLL_GSS_ERROR_GSS) {
		edg_wll_SetErrorGss(ctx,"edg_wll_SetVomsGroups()",&gss_code);
	}
	edg_wll_SetError(ctx, ret, "edg_wll_SetVomsGroups() - failed to get peer credentials");
	goto end;	
   }

   /* uses X509_CERT_DIR and X509_VOMS_DIR vars */
   voms_info = VOMS_Init(voms_dir, ca_dir);
   if (voms_info == NULL) {
      edg_wll_SetError(ctx, errno, "failed to initialize VOMS structures");
      ret = -1; /* XXX VOMS Error */
      goto end;
   }

   ret = VOMS_RetrieveFromCtx(gss->context, RECURSE_CHAIN, voms_info, &err);
   if (ret == 0) {
      if (err == VERR_NOEXT)
	 /* XXX DK:
	 edg_wll_SetError(ctx, EINVAL, "no client VOMS certificates found");
	 */
	 ret = 0;
      else {
	 edg_wll_SetError(ctx, -1, "failed to retrieve VOMS info");
	 ret = -1; /* XXX VOMS Error */
      }
      goto end;
   }

   ret = get_groups(ctx, voms_info, &ctx->vomsGroups);
   if (ret)
      goto end;

   ret = get_fqans(ctx, voms_info, &ctx->fqans);

end:
   edg_wll_gss_free_princ(principal);

   if (voms_info)
      VOMS_Destroy(voms_info);

   return ret;
}

void
edg_wll_FreeVomsGroups(edg_wll_VomsGroups *groups)
{
   size_t len;

   if (groups == NULL)
      return;

   for (len = 0; len < groups->len; len++) {
      if (groups->val[len].vo)
	 free(groups->val[len].vo);
      if (groups->val[len].name)
	 free(groups->val[len].name);
   }
}

#else /* NO_VOMS */

int
edg_wll_SetVomsGroups(edg_wll_Context ctx, edg_wll_GssConnection *gss, char *server_cert,
	char *server_key, char *voms_dir, char *ca_dir)
{
	return 0;
}

void edg_wll_FreeVomsGroups(edg_wll_VomsGroups *groups) {}

#endif


#if !defined(NO_VOMS) && !defined(NO_GACL)

static int
parse_creds(edg_wll_Context ctx, edg_wll_VomsGroups *groups, char **fqans,
            char *subject, GRSTgaclUser **gacl_user)
{
   GRSTgaclCred *cred = NULL;
   GRSTgaclUser *user = NULL;
   int i;
   char **f;

   edg_wll_ResetError(ctx);

   GRSTgaclInit();

   cred = GRSTgaclCredNew("person");
   if (cred == NULL)
      return edg_wll_SetError(ctx, ENOMEM, "Failed to create GACL person");
   
   if (!GRSTgaclCredAddValue(cred, "dn", subject)) {
      edg_wll_SetError(ctx, EINVAL, "Failed to create GACL DN credential");
      goto fail;
   }

   user = GRSTgaclUserNew(cred);
   if (user == NULL) {
      edg_wll_SetError(ctx, ENOMEM, "Failed to create GACL user");
      goto fail;
   }
   cred = NULL; /* GACLnewUser() doesn't copy content, just store the pointer */

   for (i = 0; i < groups->len; i++) {
      cred = GRSTgaclCredNew("voms-cred");
      if (cred == NULL) {
	 edg_wll_SetError(ctx, ENOMEM, "Failed to create GACL voms-cred credential");
	 goto fail;
      }
      if (!GRSTgaclCredAddValue(cred, "vo", groups->val[i].vo) ||
	  !GRSTgaclCredAddValue(cred, "group", groups->val[i].name)) {
         edg_wll_SetError(ctx, EINVAL, "Failed to populate GACL voms-cred credential");
	 goto fail;
      }
      if (!GRSTgaclUserAddCred(user, cred)) {
         edg_wll_SetError(ctx, EINVAL, "Failed to add GACL voms-cred credential");
	 goto fail;
      }
      cred = NULL;
      /* GACLuserAddCred() doesn't copy content, just store the pointer. Cred
       * mustn't be free()ed */
   }

   for (f = fqans; f && *f; f++) {
      cred = GRSTgaclCredNew("voms");
      if (cred == NULL) {
         edg_wll_SetError(ctx, ENOMEM, "Failed to create GACL voms credential");
         goto fail;
      }
      if (!GRSTgaclCredAddValue(cred, "fqan", *f)) {
         edg_wll_SetError(ctx, EINVAL, "Failed to populate GACL voms credential");
         goto fail;
      }
      if (!GRSTgaclUserAddCred(user, cred)) {
         edg_wll_SetError(ctx, EINVAL, "Failed to add GACL voms credential");
         goto fail;
      }
      cred = NULL;
   }

   *gacl_user = user;

   return 0;

fail:
   if (cred)
      GRSTgaclCredFree(cred);
   if (user)
      GRSTgaclUserFree(user);

   return edg_wll_Error(ctx,NULL,NULL);
}

static int
cmp_gacl_names(GRSTgaclNamevalue *n1, GRSTgaclNamevalue *n2)
{
   if (n1 == NULL && n2 == NULL)
      return 1;

   for ( ; n1; n1 = (GRSTgaclNamevalue *)n1->next, n2 = (GRSTgaclNamevalue *) n2->next) {
      if (n2 == NULL)
	 return 0;
      if (strcmp(n1->name, n2->name) != 0 ||
	  strcmp(n1->value, n2->value) != 0)
	 return 0;
   }

   return (n2 == NULL);
}

static int
cmp_gacl_creds(GRSTgaclCred *c1, GRSTgaclCred *c2)
{
   /* XXX the GRSTgaclCred contains a bit more information to handle */
   return (strcmp(c1->auri, c2->auri) == 0);
}

static int
addEntry(edg_wll_Context ctx, GRSTgaclAcl *acl, GRSTgaclEntry *entry)
{
	GRSTgaclEntry   *cur = NULL;
   

	if ( acl == NULL )
		return EINVAL;

	if ( acl->firstentry == NULL )
		return (GRSTgaclAclAddEntry(acl, entry) == 0) ? -1 /* GACL_ERR */ : 0;

	for ( cur = acl->firstentry; cur; cur = cur->next )
		if (   cmp_gacl_creds(cur->firstcred, entry->firstcred)
			&& cur->allowed == entry->allowed
			&& cur->denied == entry->denied ) 
	 		return edg_wll_SetError(ctx,EEXIST,"ACL entry already exists");;

	return (GRSTgaclAclAddEntry(acl, entry) == 0) ? -1 /* GACL_ERR */ : 0;
}

static int
delEntry(edg_wll_Context ctx, GRSTgaclAcl *acl, GRSTgaclEntry *entry)
{
   GRSTgaclEntry *cur = NULL, *prev = NULL;
   int found = 0;
   
   if (acl == NULL || acl->firstentry == NULL)
      return EINVAL;

   cur = acl->firstentry;
   while (cur) {
      if (cmp_gacl_creds(cur->firstcred, entry->firstcred) &&
	  cur->allowed == entry->allowed &&
	  cur->denied == entry->denied) {
	 if (prev)
	    prev->next = cur->next;
	 else
	    acl->firstentry = cur->next;
	 /* XXX GRSTgaclEntryFree(cur); */
	 found = 1;
	 break;
      }
      prev = cur;
      cur = cur->next; 
   }

   return (found) ? 0 : edg_wll_SetError(ctx,EINVAL,"ACL entry doesn't exist");
}

static int
create_cred(edg_wll_Context ctx, char *userid, int user_type, GRSTgaclCred **cred)
{
   GRSTgaclCred *c = NULL;
   char *group = NULL;

   if (user_type == EDG_WLL_CHANGEACL_DN) {
      c = GRSTgaclCredNew("person");
      if (c == NULL)
	 return ENOMEM;
      if (!GRSTgaclCredAddValue(c, "dn", userid)) {
	 GRSTgaclCredFree(c);
	 return -1; /* GACL_ERR */
      }
   } else if(user_type == EDG_WLL_CHANGEACL_GROUP) {
      c = GRSTgaclCredNew("voms-cred");
      if (c == NULL)
	 return ENOMEM;
      group = strchr(userid, ':');
      if ( !group )
	 return EINVAL;
      *group++ = '\0';
      if (!GRSTgaclCredAddValue(c, "vo", userid) ||
	  !GRSTgaclCredAddValue(c, "group", group)) {
	  GRSTgaclCredFree(c);
	 return -1; /* GACL_ERR */
      }
   } else if (user_type == EDG_WLL_CHANGEACL_FQAN) {
      c = GRSTgaclCredNew("voms");
      if (c == NULL)
         return ENOMEM;
      if (!GRSTgaclCredAddValue(c, "fqan", userid)) {
         GRSTgaclCredFree(c);
         return -1; /* GACL_ERR */
      }
   } else
      return edg_wll_SetError(ctx,EINVAL,"Unknown user type for ACL");

   *cred = c;

   return 0;
}

static int
change_acl(edg_wll_Context ctx, GRSTgaclAcl *acl, GRSTgaclEntry *entry, int operation)
      /* creds, permission, permission_type */
{
   if (operation == EDG_WLL_CHANGEACL_ADD)
      return addEntry(ctx, acl, entry);
   
   if (operation == EDG_WLL_CHANGEACL_REMOVE)
      return delEntry(ctx, acl, entry);

   return edg_wll_SetError(ctx,EINVAL,"Unknown ACL operation requested");
}

static int
edg_wll_change_acl(edg_wll_Context ctx, edg_wll_Acl acl, char *user_id,
		   int user_id_type, int permission, int perm_type,
		   int operation)
{
   GRSTgaclCred *cred = NULL;
   GRSTgaclEntry *entry = NULL;
   int ret,p;

   GRSTgaclInit();

   if (acl == NULL || acl->value == NULL)
      return edg_wll_SetError(ctx,EINVAL,"Change ACL");

   ret = create_cred(ctx, user_id, user_id_type, &cred);
   if (ret)
      return ret;

   entry = GRSTgaclEntryNew();
   if (entry == NULL) {
      ret = edg_wll_SetError(ctx,ENOMEM,"Change ACL");
      goto end;
   }

   if (!GRSTgaclEntryAddCred(entry, cred)) {
      ret = edg_wll_SetError(ctx,EINVAL,"Can't create ACL");
      goto end;
   }

   switch (permission) {
      case EDG_WLL_CHANGEACL_READ:
          p = EDG_WLL_CHANGEACL_READ;
	  break;
      default:
          ret = edg_wll_SetError(ctx,EINVAL,"Unknown permission for ACL");
	  goto end;
   }

   if (perm_type == EDG_WLL_CHANGEACL_ALLOW)
      GRSTgaclEntryAllowPerm(entry, p);
   else if (perm_type == EDG_WLL_CHANGEACL_DENY)
      GRSTgaclEntryDenyPerm(entry, p);
   else {
      ret = edg_wll_SetError(ctx,EINVAL,"Unknown permission type");
      goto end;
   }

   ret = change_acl(ctx, acl->value, entry, operation);
   if (ret)
   {
/*    XXX: mem leak?
      GRSTgaclEntryFree(entry);
*/
      goto end;
   }

   if (acl->string) free(acl->string);
   ret = edg_wll_EncodeACL(acl->value, &acl->string);

end:

   return ret;
}

int
edg_wll_CheckACL(edg_wll_Context ctx, edg_wll_Acl acl, int requested_perm)
{
   int ret;
   GRSTgaclUser *user = NULL;
   unsigned int perm;

   if (acl == NULL || acl->value == NULL)
      return edg_wll_SetError(ctx,EINVAL,"CheckACL");

   if (!ctx->peerName) return edg_wll_SetError(ctx,EPERM,"CheckACL");

   ret = parse_creds(ctx, &ctx->vomsGroups, ctx->fqans, ctx->peerName, &user);
   if (ret)
      return edg_wll_Error(ctx,NULL,NULL);

   perm = GRSTgaclAclTestUser(acl->value, user);

   GRSTgaclUserFree(user);
   
   if (perm & requested_perm) return edg_wll_ResetError(ctx);
   else return edg_wll_SetError(ctx,EPERM,"CheckACL");
}

int
edg_wll_EncodeACL(GRSTgaclAcl *acl, char **str)
{
   int tmp_fd, ret;
   FILE *fd = NULL;
   char filename[16];
   char line[4096];
   char *buf = NULL;
   size_t buf_len = 0;
   char *p;

   snprintf(filename, sizeof(filename), "/tmp/XXXXXX");
   tmp_fd = mkstemp(filename);
   if (tmp_fd == -1)
      return errno;

   fd = fdopen(tmp_fd, "r");

   ret = GRSTgaclAclSave(acl, filename);
   unlink(filename);
   if (ret == 0) {
      ret = -1; /* GACL_ERR */
      goto end;
   }

   buf_len = 1024;
   buf = calloc(buf_len, 1);
   if (buf == NULL) {
      ret = ENOMEM;
      goto end;
   }

   while (fgets(line, sizeof(line), fd) != NULL) {
      p = strchr(line, '\n');
      if (p)
	 *p = '\0';

      if (strlen(buf) + strlen(line) > buf_len) {
	 char *tmp;

	 tmp =  realloc(buf, buf_len + 1024);
	 if (tmp == NULL) {
	    ret = ENOMEM;
	    goto end;
	 }
	 buf = tmp;
	 buf_len += 1024;
      }

      strcat(buf, line);
   }

   *str = buf;
   ret = 0;

end:
   fclose(fd);
   return ret;
}

int
edg_wll_DecodeACL(char *buf, GRSTgaclAcl **result_acl)
{
   /* Got from GACLloadAcl() available from GACL API */
   xmlDocPtr   doc;
   xmlNodePtr  cur;
   GRSTgaclAcl    *acl;
   GRSTgaclEntry  *entry;
        
   doc = xmlParseMemory(buf, strlen(buf));
   if (doc == NULL) return EINVAL;
    
   cur = xmlDocGetRootElement(doc);
  
   if (xmlStrcmp(cur->name, (const xmlChar *) "gacl"))
    {
       free(doc);
       free(cur);
       return EINVAL;
    }

   cur = cur->xmlChildrenNode;

   acl = GRSTgaclAclNew();
  
   while (cur != NULL)
       {
	 /*
	 if (cur->type == XML_TEXT_NODE && cur->content == '\n') {
	    cur=cur->next;
	    continue;
	 }
	 */
         entry = GACLparseEntry(cur);
         if (entry == NULL)
           {
             /* XXX GRSTgaclAclFree(acl); */
             xmlFreeDoc(doc);
             return EINVAL;
           }

         GRSTgaclAclAddEntry(acl, entry);
         
         cur=cur->next;
       }

   xmlFreeDoc(doc);
   *result_acl = acl;
   return 0;
}

int
edg_wll_InitAcl(edg_wll_Acl *acl)
{
   edg_wll_Acl tmp;

   tmp = malloc(sizeof(*tmp));
   if ( !tmp )
      return ENOMEM;

   tmp->value = GRSTgaclAclNew();
   tmp->string = NULL;
   *acl = tmp;
   return 0;
}

void
edg_wll_FreeAcl(edg_wll_Acl acl)
{
   if ( acl->value ) GRSTgaclAclFree(acl->value);
   if ( acl->string ) free(acl->string);
   free(acl);
}

int
edg_wll_HandleCounterACL(edg_wll_Context ctx, edg_wll_Acl acl,
      			 char *aclid, int incr)
{
	char	   *q1 = NULL,
			   *q2 = NULL;

	edg_wll_ResetError(ctx);

	if ( incr > 0 )
	{
		trio_asprintf(&q1,
				"insert into acls(aclid,value,refcnt) "
				"values ('%|Ss','%|Ss',%d)",
				aclid, acl->string, incr);

		for ( ; ; )
		{
			 glite_common_log(LOG_CATEGORY_LB_SERVER_DB, 
				LOG_PRIORITY_DEBUG, q1);
			if ( edg_wll_ExecSQL(ctx, q1, NULL) > 0 )
				goto end;

			if ( edg_wll_Error(ctx,NULL,NULL) != EEXIST )
				goto end;

			/*
			 *	row allready in DB
			 */
			if ( !q2 ) trio_asprintf(&q2,
						"update acls set refcnt = refcnt+%d "
						"where aclid = '%|Ss'",
						incr, aclid);
			glite_common_log(LOG_CATEGORY_LB_SERVER_DB,  
                                LOG_PRIORITY_DEBUG, q2);
			if ( edg_wll_ExecSQL(ctx, q2, NULL) < 0 )
				continue;

			goto end; 
		}
	}
	else if (incr < 0)
	{
		trio_asprintf(&q1,
				"update acls set refcnt = refcnt-%d "
				"where aclid='%|Ss' and refcnt>=%d",
				-incr, aclid, -incr);

		glite_common_log(LOG_CATEGORY_LB_SERVER_DB,  
                                LOG_PRIORITY_DEBUG, q1);
		if ( edg_wll_ExecSQL(ctx, q1, NULL) > 0 )
		{
			trio_asprintf(&q2,
						"delete from acls "
						"where aclid='%|Ss' and refcnt=0",
						aclid);
			glite_common_log(LOG_CATEGORY_LB_SERVER_DB,  
                                LOG_PRIORITY_DEBUG, q2);
			edg_wll_ExecSQL(ctx, q2, NULL);
		}
		else
		{
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "ACL with ID: %s has invalid reference count", aclid);
		}
	}


end:
	if ( q1 ) free(q1);
	if ( q2 ) free(q2);

	return edg_wll_Error(ctx, NULL, NULL);
}

int
edg_wll_UpdateACL(edg_wll_Context ctx, glite_jobid_const_t job, 
      		  char *user_id, int user_id_type,
		  int permission, int perm_type, int operation)
{
   char *md5_jobid;
   edg_wll_Acl acl = NULL;
   int ret;
   char *stmt = NULL;
   char *new_aclid = NULL, *old_aclid = NULL;
   int updated;

   edg_wll_ResetError(ctx);

   md5_jobid = edg_wlc_JobIdGetUnique(job);

   do {
      if (acl)
      {
	 edg_wll_FreeAcl(acl);
	 acl = NULL;
      }
      if (old_aclid)
      {
	 free(old_aclid);
      	 old_aclid = NULL;
      }
      if (new_aclid)
      {
	 free(new_aclid);
         new_aclid = NULL;
      }
	 
      if ( (ret = edg_wll_GetACL(ctx, job, &acl)) )
	 goto end;
      if ( !acl && (ret = edg_wll_InitAcl(&acl)) )
	 goto end;
	 
      old_aclid = acl->string? strdup(strmd5(acl->string, NULL)): NULL;

      ret = edg_wll_change_acl(ctx, acl, user_id, user_id_type, 
	    		       permission, perm_type, operation);
      if (ret)
      {
	 if ( ret == EEXIST )
            ret = edg_wll_ResetError(ctx);
	 goto end;
      }

      new_aclid = strdup(strmd5(acl->string, NULL));

      /* store new ACL or increment its counter if already present in db */
      ret = edg_wll_HandleCounterACL(ctx, acl, new_aclid, 1);
      if  (ret)
	 goto end;

      if ( old_aclid )
	 trio_asprintf(&stmt,
	    "update jobs set aclid='%|Ss' where jobid='%|Ss' and aclid='%|Ss'",
	    new_aclid, md5_jobid, old_aclid);
      else
	 trio_asprintf(&stmt,
	    "update jobs set aclid='%|Ss' where jobid='%|Ss' and ISNULL(aclid)",
	    new_aclid, md5_jobid);
      glite_common_log(LOG_CATEGORY_LB_SERVER_DB,
         LOG_PRIORITY_DEBUG, stmt);
      updated = edg_wll_ExecSQL(ctx, stmt, NULL);
      free(stmt); stmt = NULL;

      if (updated > 0)
	 /* decrement reference counter of the old ACL, and possibly remove
	  * whole ACL if the counter becames zero */
	 ret = edg_wll_HandleCounterACL(ctx, NULL, old_aclid, -1);
      else
         /* We failed to store new ACL to db, most likely because the ACL has
	  * been changed. Decrement counter of new ACL set before trying
	  * updating */
	 ret = edg_wll_HandleCounterACL(ctx, NULL, new_aclid, -1);
   } while (updated <= 0);

end:
   free(md5_jobid);
   if (acl)
      edg_wll_FreeAcl(acl);
   if (new_aclid)
      free(new_aclid);
   if (old_aclid)
      free(old_aclid);

   return ret;
}

int edg_wll_GetACL(edg_wll_Context ctx, glite_jobid_const_t jobid, edg_wll_Acl *acl)
{
	char	*q = NULL;
	char	*acl_id = NULL;
	char	*acl_str = NULL;
	glite_lbu_Statement    stmt = NULL;
	int	ret;
	GRSTgaclAcl	*gacl = NULL;
	char	*jobstr = edg_wlc_JobIdGetUnique(jobid);

	if (jobid == NULL || jobstr == NULL)
	   return edg_wll_SetError(ctx,EINVAL,"edg_wll_GetACL()");

	edg_wll_ResetError(ctx);

	trio_asprintf(&q,
		"select aclid from jobs where jobid = '%|Ss'", jobstr);

	glite_common_log(LOG_CATEGORY_LB_SERVER_DB,
        	LOG_PRIORITY_DEBUG, q);
	if (edg_wll_ExecSQL(ctx, q, &stmt) < 0 ||
		edg_wll_FetchRow(ctx, stmt, 1, NULL, &acl_id) < 0) {
		goto end;
	}
	glite_lbu_FreeStmt(&stmt); stmt = NULL;
	free(q); q = NULL;

	if (acl_id == NULL || *acl_id == '\0') {
		free(acl_id);
		free(jobstr);
		*acl = NULL;
		return 0;
	}

	trio_asprintf(&q,
		"select value from acls where aclid = '%|Ss'", acl_id);
	glite_common_log(LOG_CATEGORY_LB_SERVER_DB,
        	LOG_PRIORITY_DEBUG, q);
	if (edg_wll_ExecSQL(ctx, q, &stmt) < 0 ||
		edg_wll_FetchRow(ctx, stmt, 1, NULL, &acl_str) < 0) {
		goto end;
	}

	ret = edg_wll_DecodeACL(acl_str, &gacl);
	if (ret) {
		edg_wll_SetError(ctx, EINVAL, "encoding ACL");
		goto end;
	}

	*acl = calloc(1, sizeof(**acl));
	if (*acl == NULL) {
		ret = ENOMEM;
		edg_wll_SetError(ctx, ENOMEM, "not enough memory");
		goto end;
	}

	(*acl)->value = gacl;
	(*acl)->string = acl_str;
	gacl = NULL; acl_str = NULL;
	ret = 0;

end:
	if (q) free(q);
	if (stmt) glite_lbu_FreeStmt(&stmt);
	if (acl_id) free(acl_id);
	if (acl_str) free(acl_str);
	if (gacl) GRSTgaclAclFree(gacl);
	if (jobstr) free(jobstr);

	return edg_wll_Error(ctx, NULL, NULL);
}

int
check_store_authz(edg_wll_Context ctx, edg_wll_Event *ev)
{
   char *pem_string = NULL;
   char *request = NULL;
   int ret;

   /* XXX make a real RSL ? */
   request = edg_wll_EventToString(ev->any.type);
   if (request == NULL)
      return edg_wll_SetError(ctx, EINVAL, "Unknown event type");

   ret = edg_wll_gss_get_client_pem(&ctx->connections->serverConnection->gss,
				    server_cert, server_key,
                                    &pem_string);
   if (ret)
      return edg_wll_SetError(ctx, ret, "Failed to extract client's PEM string");

   ret = lcas_pem(pem_string, request);
   if (ret)
      ret = edg_wll_SetError(ctx, EPERM, "Not allowed to log events here");

   free(pem_string);

   return ret;
}

#else /* VOMS & GACL */


int edg_wll_CheckACL(edg_wll_Context ctx, edg_wll_Acl acl, int requested_perm) { return EPERM; }

#ifndef NO_GACL
int edg_wll_EncodeACL(GRSTgaclAcl *acl, char **str) { return 0; }
int edg_wll_DecodeACL(char *buf, GRSTgaclAcl **result_acl) { return 0; }
#else
int edg_wll_EncodeACL(void *acl, char **str) { return 0; }
int edg_wll_DecodeACL(char *buf, void **result_acl) { return 0; }
#endif

int edg_wll_InitAcl(edg_wll_Acl *acl) { return 0; }
void edg_wll_FreeAcl(edg_wll_Acl acl) { }
int edg_wll_HandleCounterACL(edg_wll_Context ctx, edg_wll_Acl acl,
                         char *aclid, int incr) { return 0; }
int edg_wll_UpdateACL(edg_wll_Context ctx, glite_jobid_const_t job,
                  char *user_id, int user_id_type,
                  int permission, int perm_type, int operation) { return 0; }
int edg_wll_GetACL(edg_wll_Context ctx, glite_jobid_const_t jobid, edg_wll_Acl *acl) { return 0; }


#endif


int edg_wll_amIroot(const char *subj, char **fqans,char **super_users)
{
	int	i;
	char	**f;

	if (!subj && !fqans ) return 0;
	for (i=0; super_users && super_users[i]; i++)
		if (strncmp(super_users[i], "FQAN:", 5) == 0) {
			for (f = fqans; f && *f; f++)
				if (strcmp(*f, super_users[i]+5) == 0) return 1;
		} else
			if (edg_wll_gss_equal_subj(subj,super_users[i])) return 1;

	return 0;
}

