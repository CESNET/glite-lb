#ident "$Header$"

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "glite/lb/context-int.h"
#include "glite/lbu/trio.h"

#include "lb_authz.h"
#include "lb_xml_parse.h"
#include "query.h"
#include "il_notification.h"
#include "db_supp.h"
#include "index.h"

static int notif_match_conditions(edg_wll_Context,const edg_wll_JobStat *,const char *);
static int notif_check_acl(edg_wll_Context,const edg_wll_JobStat *,const char *);

int edg_wll_NotifExpired(edg_wll_Context,const char *);

int edg_wll_NotifMatch(edg_wll_Context ctx, const edg_wll_JobStat *stat)
{
	edg_wll_NotifId		nid = NULL;
	char	*jobq,*ju = NULL,*jobc[5];
	glite_lbu_Statement	jobs = NULL;
	int	ret,i,expires;
	time_t	now = time(NULL);
	
	char *cond_where = NULL;
	char *cond_and_where = NULL;

	edg_wll_ResetError(ctx);

	if (ctx->notif_index) {
		edg_wll_IColumnRec *notif_index_cols = ctx->notif_index_cols;

		for (i=0; notif_index_cols[i].qrec.attr; i++) {
			char	*val = NULL;

			if (notif_index_cols[i].qrec.attr != EDG_WLL_QUERY_ATTR_JDL_ATTR) {
				val = edg_wll_StatToSQL(stat,notif_index_cols[i].qrec.attr);
				assert(val != (char *) -1);
			}
			else { // Special Treatment for JDL attributes
				val = edg_wll_JDLStatToSQL(stat,notif_index_cols[i].qrec);
			} 

			if (val) {
				char	*aux;
				if (!cond_where) cond_where = strdup("");
				trio_asprintf(&aux, "%s or %s = '%s'",cond_where,
						notif_index_cols[i].colname,val);
				free(cond_where);
				cond_where = aux;
				free(val);
			}
			else if (notif_index_cols[i].qrec.attr == EDG_WLL_QUERY_ATTR_JDL_ATTR) {
				char	*aux;
				if (!cond_and_where) cond_and_where = strdup("");
				trio_asprintf(&aux, "%s AND %s is NULL",cond_and_where,
						notif_index_cols[i].colname);
				free(cond_and_where);
				cond_and_where = aux;
				free(val);
			}
		}
	}

	if ( (ret = edg_wll_NotifIdCreate(ctx->srvName, ctx->srvPort, &nid)) )
	{
		edg_wll_SetError(ctx, ret, "edg_wll_NotifMatch()");
		goto err;
	}

	trio_asprintf(&jobq,
		"select distinct n.notifid,n.destination,n.valid,u.cert_subj,n.conditions "
		"from notif_jobs j,users u,notif_registrations n "
		"where j.notifid=n.notifid and n.userid=u.userid "
		"   and (j.jobid = '%|Ss' or j.jobid = '%|Ss' %s) %s",
		ju = edg_wlc_JobIdGetUnique(stat->jobId),NOTIF_ALL_JOBS,cond_where ? cond_where : "",cond_and_where ? cond_and_where : "");

	free(ju);

	if (edg_wll_ExecSQL(ctx,jobq,&jobs) < 0) goto err;

	while ((ret = edg_wll_FetchRow(ctx,jobs,sizeof(jobc)/sizeof(jobc[0]),NULL,jobc)) > 0) {
		if (now > (expires = glite_lbu_DBToTime(jobc[2])))
			edg_wll_NotifExpired(ctx,jobc[0]);
		else if (notif_match_conditions(ctx,stat,jobc[4]) &&
				notif_check_acl(ctx,stat,jobc[3]))
		{
			char			   *dest, *aux;
			int					port;

			fprintf(stderr,"NOTIFY: %s, job %s\n",jobc[0],
					ju = edg_wlc_JobIdGetUnique(stat->jobId));
			free(ju);

			dest = strdup(jobc[1]);
			if ( !(aux = strchr(dest, ':')) )
			{
				edg_wll_SetError(ctx, EINVAL, "Can't parse notification destination");
				free(dest);
				for (i=0; i<sizeof(jobc)/sizeof(jobc[0]); i++) free(jobc[i]);
				goto err;
			}
			*aux = 0;
			aux++;
			port = atoi(aux);
			
			if (   edg_wll_NotifIdSetUnique(&nid, jobc[0]) )
			{
				free(dest);
				goto err;
			}
			/* XXX: only temporary hack!!!
			 */
			ctx->p_instance = strdup("");
			if ( edg_wll_NotifJobStatus(ctx, nid, dest, port, jobc[3], expires, *stat) )
			{
				free(dest);
				for (i=0; i<sizeof(jobc)/sizeof(jobc[0]); i++) free(jobc[i]);
				goto err;
			}
			free(dest);
		}
		
		for (i=0; i<sizeof(jobc)/sizeof(jobc[0]); i++) free(jobc[i]);
	}
	if (ret < 0) goto err;
	
err:
	free(ctx->p_instance); ctx->p_instance = NULL;
	if ( nid ) edg_wll_NotifIdFree(nid);
	free(jobq);
	glite_lbu_FreeStmt(&jobs);
	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_NotifExpired(edg_wll_Context ctx,const char *notif)
{
	char	*dn = NULL,*dj = NULL;

	trio_asprintf(&dn,"delete from notif_registrations where notifid='%|Ss'",notif);
	trio_asprintf(&dj,"delete from notif_jobs where notifid='%|Ss'",notif);

	if (edg_wll_ExecSQL(ctx,dn,NULL) < 0 ||
		edg_wll_ExecSQL(ctx,dj,NULL) < 0)
	{
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);

		syslog(LOG_WARNING,"delete notification %s: %s (%s)",notif,et,ed);
		free(et); free(ed);
	}

	free(dn);
	free(dj);
	return edg_wll_ResetError(ctx);
}


static int notif_match_conditions(edg_wll_Context ctx,const edg_wll_JobStat *stat,const char *cond)
{
	edg_wll_QueryRec	**c,**p;
	int			match,i;

	if (!cond) return 1;

	if (parseJobQueryRec(ctx,cond,strlen(cond),&c)) {
		fputs("notif_match_conditions(): parseJobQueryRec failed\n",stderr);
		syslog(LOG_ERR,"notif_match_conditions(): parseJobQueryRec failed");
		return 1;
	}

	match = match_status(ctx,stat,(const edg_wll_QueryRec **) c);
	if ( c )
	{
		for (p = c; *p; p++) {
			for (i=0; (*p)[i].attr; i++)
				edg_wll_QueryRecFree((*p)+i);
			free(*p);
		}
		free(c);
	}
	return match;
}

/* FIXME: does not favour any VOMS information in ACL
 * effective VOMS groups of the recipient are not available here, should be 
 * probably stored along with the registration.
 */
static int notif_check_acl(edg_wll_Context ctx,const edg_wll_JobStat *stat,const char *recip)
{
	edg_wll_Acl	acl = calloc(1,sizeof *acl);
/* XXX: NO_GACL	GACLacl		*gacl; */
	void		*gacl;
	int		ret;

	edg_wll_ResetError(ctx);
	if (ctx->noAuth || strcmp(stat->owner,recip) == 0) return 1;

	ret = edg_wll_DecodeACL(stat->acl,&gacl);
	if (ret) {
		edg_wll_SetError(ctx,EINVAL,"decoding ACL");
		return 0;
	}

	acl->string = stat->acl; 
	acl->value = gacl;

	ret = edg_wll_CheckACL(ctx, acl, EDG_WLL_PERM_READ);

	acl->string = NULL;
	edg_wll_FreeAcl(acl);

	return !ret;
}
