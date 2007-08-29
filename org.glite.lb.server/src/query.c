#ident "$Header$"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include "glite/wmsutils/jobid/strmd5.h"

#include "glite/lb/consumer.h"
#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/trio.h"


#include "get_events.h"
#include "index.h"
#include "query.h"
#include "store.h"
#include "lb_authz.h"

#define FL_SEL_STATUS		1
#define FL_SEL_TAGS			(1<<1)
#define FL_SEL_JOB			(1<<2)


static int check_event_query_index(edg_wll_Context,const edg_wll_QueryRec **,const edg_wll_QueryRec **);
static int check_job_query_index(edg_wll_Context, const edg_wll_QueryRec **);
static char *jc_to_head_where(edg_wll_Context, const edg_wll_QueryRec **, int *);
static char *ec_to_head_where(edg_wll_Context, const edg_wll_QueryRec **);
static int match_flesh_conditions(const edg_wll_Event *,const edg_wll_QueryRec **);
static int check_strict_jobid_cond(edg_wll_Context, const edg_wll_QueryRec **);

static int cmp_string(const char *,edg_wll_QueryOp,const char *);
static int is_all_query(const edg_wll_QueryRec **);



#define sizofa(a) (sizeof(a)/sizeof((a)[0]))

int edg_wll_QueryEventsServer(
        edg_wll_Context ctx,
		int	noAuth,
        const edg_wll_QueryRec **job_conditions,
        const edg_wll_QueryRec **event_conditions,
        edg_wll_Event **events)
{
	char		   *job_where = NULL,
				   *event_where = NULL,
				   *qbase = NULL,
				   *q = NULL,
				   *res[11];
	edg_wll_Event  *out = NULL;
	edg_wll_Stmt	sh = NULL;
	int				i = 0,
					ret = 0,
					offset = 0, limit = 0,
					limit_loop = 1,
					eperm = 0;
	char *peerid = NULL;
	char *can_peername = NULL, *can_peerid = NULL;

	edg_wll_ResetError(ctx);

	if ( (ctx->p_query_results == EDG_WLL_QUERYRES_ALL) &&
			(!job_conditions || !job_conditions[0] || job_conditions[1] ||
			(job_conditions[0][0].attr != EDG_WLL_QUERY_ATTR_JOBID) ||
			(job_conditions[0][1].attr != EDG_WLL_QUERY_ATTR_UNDEF)) )
	{
		edg_wll_SetError(ctx, EINVAL, "Invalid parameter EDG_WLL_PARAM_QUERY_RESULTS");
		goto cleanup;
	}

	if ((!ctx->noIndex && check_event_query_index(ctx,job_conditions,event_conditions)) ||
		check_strict_jobid_cond(ctx,job_conditions) ||
		check_strict_jobid_cond(ctx,event_conditions))
       	goto cleanup;

	if (event_conditions && *event_conditions && (*event_conditions)->attr &&
		!(event_where = ec_to_head_where(ctx,event_conditions)) &&
		edg_wll_Error(ctx,NULL,NULL) != 0)
		goto cleanup;

	if ( job_conditions && *job_conditions && (*job_conditions)->attr &&
		!(job_where = jc_to_head_where(ctx, job_conditions, &i)) )
		goto cleanup;

	peerid = strdup(strmd5(ctx->peerName,NULL));
	can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
	can_peerid = strdup(strmd5(can_peername,NULL));

/* XXX: similar query in srv_purge.c ! They has to match due to common
 * convert_event_head() called on the result
 */
	trio_asprintf(&qbase,"SELECT e.event,j.userid,j.dg_jobid,e.code,"
		"e.prog,e.host,u.cert_subj,e.time_stamp,e.usec,e.level,e.arrived "
		"FROM events e,users u,jobs j%s "
		"WHERE %se.jobid=j.jobid AND e.userid=u.userid AND e.code != %d "
		"%s %s %s %s",
		i & FL_SEL_STATUS			? ",states s"				: "",
		i & FL_SEL_STATUS			? "s.jobid=j.jobid AND "	: "",
		EDG_WLL_EVENT_UNDEF,
		job_where					? "AND"						: "",
		job_where					? job_where					: "",
		event_where					? "AND"						: "",
		event_where					? event_where				: "");

	if ( ctx->softLimit )
	{
		if ( ctx->hardEventsLimit )
			limit = ctx->softLimit < ctx->hardEventsLimit? ctx->softLimit: ctx->hardEventsLimit;
		else
			limit = ctx->softLimit;
	}
	else if ( ctx->hardEventsLimit )
		limit = ctx->hardEventsLimit;
	else
		limit = 0;

	i = 0;
	out = calloc(1, sizeof(*out));
	do
	{
		if ( limit )
			trio_asprintf(&q, "%s LIMIT %d, %d", qbase, offset, limit);
		else if ( !q )
			q = qbase;

//		printf("\nquery: %s\n\n", q);
		ret = edg_wll_ExecStmt(ctx, q, &sh);
		if ( limit )
			free(q);
		if ( ret < 0 )
		{
			edg_wll_FreeStmt(&sh);
			goto cleanup;
		}
		if ( ret == 0 )
		{
			limit_loop = 0;
			goto limit_cycle_cleanup;
		}
		if ( !limit || (ret < limit) )
			limit_loop = 0;

		offset += ret;
		while ( (ret = edg_wll_FetchRow(sh, res)) == sizofa(res) )
		{
			int	n = atoi(res[0]);
			free(res[0]);

			if ( convert_event_head(ctx, res+2, out+i) || edg_wll_get_event_flesh(ctx, n, out+i) )
			{
				free(res[1]);
				memset(out+i, 0, sizeof(*out));
				edg_wll_FreeStmt(&sh);
				goto cleanup;
			}

			if ( !match_flesh_conditions(out+i,event_conditions) || check_strict_jobid(ctx,out[i].any.jobId) )
			{
				edg_wll_FreeEvent(out+i);
				edg_wll_ResetError(ctx);	/* check_strict_jobid() sets it */
				goto fetch_cycle_cleanup;
			}

			if ( !noAuth )
			{
				if (!ctx->peerName || (strcmp(res[1],peerid) && strcmp(res[1], can_peerid))) {
					edg_wll_Acl	acl = NULL;
					char		*jobid = NULL;

					ret = edg_wll_GetACL(ctx, out[i].any.jobId, &acl);
					free(jobid);
					if (ret || acl == NULL) {
						eperm = 1;
						edg_wll_FreeEvent(out+i);
						edg_wll_ResetError(ctx); /* XXX: should be reported somewhere at least in debug mode */
						goto fetch_cycle_cleanup;
					}

					ret = edg_wll_CheckACL(ctx, acl, EDG_WLL_PERM_READ);
					edg_wll_FreeAcl(acl);
					if (ret) {
						eperm = 1;
						edg_wll_FreeEvent(out+i);
						edg_wll_ResetError(ctx); /* XXX: should be reported somewhere at least in debug mode */
						goto fetch_cycle_cleanup;
					}
				}
			}
			
			if ( (ctx->p_query_results != EDG_WLL_QUERYRES_ALL) && limit && (i+1 > limit) )
			{
				free(res[1]);
				memset(out+i, 0, sizeof(*out));
				edg_wll_SetError(ctx, E2BIG, "Query result size limit exceeded");
				if ( ctx->p_query_results == EDG_WLL_QUERYRES_LIMITED )
				{
					limit_loop = 0;
					goto limit_cycle_cleanup;
				}
				goto cleanup;
			}

			i++;
			out = (edg_wll_Event *) realloc(out, (i+1) * sizeof(*out));

fetch_cycle_cleanup:
			memset(out+i, 0, sizeof(*out));
			free(res[1]);
		}
limit_cycle_cleanup:
		edg_wll_FreeStmt(&sh);
	} while ( limit_loop );

	if ( i == 0 && eperm )
		edg_wll_SetError(ctx, EPERM, "matching events found but authorization failed");
	else if ( i == 0 )
		edg_wll_SetError(ctx, ENOENT, "no matching events found");
	else
	{
		edg_wll_SortEvents(out);
		*events = out;
		out = NULL;
	}

cleanup:
	if ( out )
	{
		for ( i = 0; out[i].type; i++ )
			edg_wll_FreeEvent(out+i);
		free(out);
	}
	free(qbase);
	free(job_where);
	free(event_where);
	free(peerid);
	free(can_peername); free(can_peerid);

	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_QueryJobsServer(
		edg_wll_Context ctx,
		const edg_wll_QueryRec **conditions,
		int	flags,
		edg_wlc_JobId **jobs,
		edg_wll_JobStat **states)
{
	char			   *job_where = NULL,
					   *state_where = NULL,
					   *tags_where = NULL,
					   *q = NULL,
					   *qbase = NULL,
					   *res[3];
	edg_wlc_JobId	   *jobs_out = NULL;
	edg_wll_JobStat	   *states_out = NULL;
	edg_wll_Stmt		sh;
	int					i = 0,
						ret = 0,
						eperm = 0,
						limit = 0, offset = 0,
						limit_loop = 1;


	memset(res,0,sizeof res);
	edg_wll_ResetError(ctx);

	if ( !conditions )
	{
		edg_wll_SetError(ctx, EINVAL, "empty condition list");
		goto cleanup;
	}

	if ( (ctx->p_query_results == EDG_WLL_QUERYRES_ALL) &&
			(!conditions[0] || conditions[1] ||
			(conditions[0][0].attr != EDG_WLL_QUERY_ATTR_OWNER) ||
			(conditions[0][1].attr != EDG_WLL_QUERY_ATTR_UNDEF)) )
	{
		edg_wll_SetError(ctx, EINVAL, "Invalid parameter EDG_WLL_PARAM_QUERY_RESULTS");
		goto cleanup;
	}

	if ( (!ctx->noIndex && check_job_query_index(ctx, conditions)) || check_strict_jobid_cond(ctx,conditions))
		goto cleanup;

	if ( !(job_where = jc_to_head_where(ctx, conditions, &i)) )
		goto cleanup;

	if ( (i & FL_SEL_STATUS) )
		trio_asprintf(&qbase,"SELECT DISTINCT j.dg_jobid,j.userid "
						 "FROM jobs j, states s WHERE j.jobid=s.jobid AND %s", job_where);
	else
		trio_asprintf(&qbase,"SELECT DISTINCT j.dg_jobid,j.userid "
						 "FROM jobs j WHERE %s", job_where);

	if ( ctx->softLimit )
	{
		if ( ctx->hardJobsLimit )
			limit = ctx->softLimit < ctx->hardJobsLimit? ctx->softLimit: ctx->hardJobsLimit;
		else
			limit = ctx->softLimit;
	}
	else if ( ctx->hardJobsLimit )
		limit = ctx->hardJobsLimit;
	else
		limit = 0;

	jobs_out	= calloc(1, sizeof(*jobs_out));
	states_out	= calloc(1, sizeof(*states_out));
	i = 0;
	do
	{
		if ( limit )
			trio_asprintf(&q, "%s LIMIT %d, %d", qbase, offset, limit);
		else if ( !q )
			q = qbase;

//		printf("\nquery: %s\n\n", q);
		ret = edg_wll_ExecStmt(ctx, q, &sh);
		if ( limit )
			free(q);
		if ( ret < 0 )
		{
			edg_wll_FreeStmt(&sh);
			goto cleanup;
		}
		if ( ret == 0 )
		{
			limit_loop = 0;
			goto limit_cycle_cleanup;
		}
		if ( !limit || (ret < limit) )
			limit_loop = 0;

		offset += ret;
		while ( (ret=edg_wll_FetchRow(sh,res)) > 0 )
		{
			if ( (ret = edg_wlc_JobIdParse(res[0], jobs_out+i)) )
			{	/* unlikely to happen, internal inconsistency */
				char	buf[200];
				snprintf(buf,sizeof buf,"JobIdParse(%s)",res[0]);
				edg_wll_SetError(ctx,ret,buf);
				free(res[0]); free(res[1]); free(res[2]);
				jobs_out[i]	= NULL;
				goto cleanup;
			}

			if ( check_strict_jobid(ctx, jobs_out[i]) )
			{
				edg_wlc_JobIdFree(jobs_out[i]);
				goto fetch_cycle_cleanup;
			}

			if ( edg_wll_JobStatus(ctx, jobs_out[i], flags, &states_out[i]) )
			{
				edg_wlc_JobIdFree(jobs_out[i]);
				if (edg_wll_Error(ctx,NULL,NULL) == EPERM) eperm = 1;
				goto fetch_cycle_cleanup;
			}

			if ( !match_status(ctx, states_out+i, conditions) )
			{
				edg_wlc_JobIdFree(jobs_out[i]);
				edg_wll_FreeStatus(states_out+i);
				edg_wll_ResetError(ctx);	/* check_strict_jobid() sets it */
				goto fetch_cycle_cleanup;
			}

#if 0
			if ( !ctx->noAuth && (!ctx->peerName || strcmp(res[1], strmd5(ctx->peerName, NULL))) )
			{
				eperm = 1;
				edg_wlc_JobIdFree(jobs_out[i]);
				edg_wll_FreeStatus(states_out+i);
				goto fetch_cycle_cleanup;
			}
#endif

			if ( (ctx->p_query_results != EDG_WLL_QUERYRES_ALL) && limit && (i+1 > limit) )
			{
				edg_wll_SetError(ctx, E2BIG, "Query result size limit exceeded");
				free(res[0]); free(res[1]); free(res[2]);
				memset(states_out+i, 0, sizeof(*states_out));
				jobs_out[i]	= NULL;
				if ( ctx->p_query_results == EDG_WLL_QUERYRES_LIMITED )
				{
					limit_loop = 0;
					goto limit_cycle_cleanup;
				}
				goto cleanup;
			}

			i++;
			jobs_out	= (edg_wlc_JobId *) realloc(jobs_out, (i+1) * sizeof(*jobs_out));
			states_out	= (edg_wll_JobStat *) realloc(states_out, (i+1) * sizeof(*states_out));

fetch_cycle_cleanup:
			free(res[0]); free(res[1]); free(res[2]);
			memset(states_out+i, 0, sizeof(*states_out));
			jobs_out[i]	= NULL;
		}
limit_cycle_cleanup:
		edg_wll_FreeStmt(&sh);
	} while ( limit_loop );

	if ( !*jobs_out ) {
		if (eperm) edg_wll_SetError(ctx, EPERM, "matching jobs found but authorization failed");
		  else     edg_wll_SetError(ctx, ENOENT, "no matching jobs found");
	}

	if ( i && (ret == 0) )
	{
		if ( states )
		{
			*states = states_out;
			states_out = NULL;
		}
		if ( jobs )
		{
			*jobs = jobs_out;
			jobs_out = NULL;
		}
	}


cleanup:
	free(qbase);
	free(state_where);
	free(tags_where);
	free(job_where);

	if (jobs_out)
	{
		for ( i = 0; jobs_out[i]; i++ )
			edg_wlc_JobIdFree(jobs_out[i]);
		free(jobs_out);
	}

	if (states_out)
	{
		for (i=0; states_out[i].state; i++) edg_wll_FreeStatus(states_out+i);
		free(states_out);
	}

	return edg_wll_Error(ctx,NULL,NULL);
}


static int check_event_query_index(edg_wll_Context ctx,const edg_wll_QueryRec **jc,const edg_wll_QueryRec **ec)
{
	int	i, j;

	if (check_job_query_index(ctx,jc) == 0) return 0;
	edg_wll_ResetError(ctx);

	if (ec && *ec) for (i=0; ec[i]; i++) for (j=0; ec[i][j].attr; j++) switch (ec[i][j].attr) {
		case EDG_WLL_QUERY_ATTR_TIME:
			return 0;
		default: break;
	}
	return edg_wll_SetError(ctx,EDG_WLL_ERROR_NOINDEX,
			is_all_query(jc) ? "\"-all\" queries denied by server configuration" : NULL);
}

static int check_job_query_index(edg_wll_Context ctx, const edg_wll_QueryRec **jc)
{
	int		i, j, jj;


	edg_wll_ResetError(ctx);

	if ( !jc || !*jc )
		return edg_wll_SetError(ctx,EDG_WLL_ERROR_NOINDEX,"unrestricted queries unsupported");

	/*
	 *	First check presense of jobid - Primary key
	 */
	for ( i = 0; jc[i]; i++ ) for ( j = 0; jc[i][j].attr; j++ )
		if ( jc[i][j].attr == EDG_WLL_QUERY_ATTR_JOBID ) return 0;

	if ( !ctx->job_index )
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_NOINDEX, "no indices configured: jobid required in query");

	for ( j = 0; ctx->job_index[j]; j++ )
	{
		if ( !ctx->job_index[j][0].attr )
			continue;

		if ( ctx->job_index[j][1].attr )
		{
			/*
			 * check multi comlumn indexes
			 */
			for ( jj = 0; ctx->job_index[j][jj].attr; jj++ )
			{
				for ( i = 0; jc[i]; i++ )
					if ( !edg_wll_CmpColumn(&ctx->job_index[j][jj], &jc[i][0]) ) break;
				if ( !jc[i] )
					break;
			}
			if ( !ctx->job_index[j][jj].attr )
				return 0;
		}
		else
			for ( i = 0; jc[i]; i++ )
				if ( !edg_wll_CmpColumn(&ctx->job_index[j][0], &jc[i][0]) ) return 0;
	}

	return edg_wll_SetError(ctx,EDG_WLL_ERROR_NOINDEX,
			is_all_query(jc) ? "\"-all\" queries denied by server configuration" : NULL);
}

#define opToString(op)										\
	((op) == EDG_WLL_QUERY_OP_EQUAL ? "=" :					\
		((op) == EDG_WLL_QUERY_OP_UNEQUAL ? "!=" :			\
			((op) == EDG_WLL_QUERY_OP_LESS ? "<" : ">" )))

static char *ec_to_head_where(edg_wll_Context ctx,const edg_wll_QueryRec **ec)
{
	int					n, ct, m;
	char				msg[100],
					   *out,
					   *aux,
					   *conds, *retconds,
					   *dbt;
			

	/*
	 * check correctness only
	 */
	for ( ct = m = 0; ec[m]; m++ )
	{
		for ( n = 0; ec[m][n].attr; n++ ) switch ( ec[m][n].attr )
		{
		case EDG_WLL_QUERY_ATTR_HOST:
			if ( ec[m][n].op != EDG_WLL_QUERY_OP_EQUAL && ec[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with host attr");
				return NULL;
			}
			ct++;
			break;

		case EDG_WLL_QUERY_ATTR_SOURCE:
			if ( ec[m][n].op != EDG_WLL_QUERY_OP_EQUAL && ec[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with host attr");
				return NULL;
			}
			ct++;
			break;

		case EDG_WLL_QUERY_ATTR_USERTAG:
			if ( ec[m][n].op != EDG_WLL_QUERY_OP_EQUAL && ec[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with usertag attr");
				return NULL;
			}
			ct++;
			break;

		case EDG_WLL_QUERY_ATTR_INSTANCE:
			if ( ec[m][n].op != EDG_WLL_QUERY_OP_EQUAL && ec[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with instance attr");
				return NULL;
			}
			ct++;
			break;

		case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
		case EDG_WLL_QUERY_ATTR_LEVEL:
			/*
			 *	Any op allowed - but be careful
			 */
		case EDG_WLL_QUERY_ATTR_TIME:
			/* any operator allowed */
			ct++;
			break;

		default:
			sprintf(msg, "ec_to_head_where(): attr=%d unsupported", ec[m][n].attr);
			edg_wll_SetError(ctx, EINVAL, msg);
			return NULL;
		}
	}

	if ( ct == 0 )
	{
		edg_wll_SetError(ctx,EINVAL,"ec_to_head_where(): empty conditions");
		return NULL;
	}

	conds = retconds = NULL;
	for ( m = 0; ec[m]; m++ )
	{
		/* 
		 * conditions captured by match_flesh_conditions() have to be skipped here.
		 * with all conditions in same "or clause"
	 	 */
		for ( n = 0; ec[m][n].attr; n++ )
			if ( 	/* (ec[m][n].attr == EDG_WLL_QUERY_ATTR_USERTAG) || */
				(ec[m][n].attr == EDG_WLL_QUERY_ATTR_INSTANCE) )
				break;
		if ( ec[m][n].attr )
			continue;

		for ( n = 0; ec[m][n].attr; n++ ) switch ( ec[m][n].attr )
		{
		case EDG_WLL_QUERY_ATTR_TIME:
			dbt = edg_wll_TimeToDB(ec[m][n].value.t.tv_sec);
			if ( conds )
			{
				if ( ec[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
				{
					trio_asprintf(&aux, "%s", dbt);
					dbt = edg_wll_TimeToDB(ec[m][n].value2.t.tv_sec);
					trio_asprintf(&out, "%s OR (e.time_stamp >= %s AND e.time_stamp <= %s)", conds, aux, dbt);
					free(aux);
				}
				else if (ec[m][n].op == EDG_WLL_QUERY_OP_EQUAL) {
					trio_asprintf(&out, "%s OR (e.time_stamp = %s AND e.usec = %d)",
							conds, dbt, ec[m][n].value.t.tv_usec);
				}
				else
					trio_asprintf(&out, "%s OR e.time_stamp %s %s", conds, opToString(ec[m][n].op), dbt);
				free(conds);
				conds = out;
			}
			else if ( ec[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
			{
				trio_asprintf(&aux, "%s", dbt);
				dbt = edg_wll_TimeToDB(ec[m][n].value2.t.tv_sec);
				trio_asprintf(&conds, "(e.time_stamp >= %s AND e.time_stamp <= %s)", aux, dbt);
				free(aux);
			}
			else if (ec[m][n].op == EDG_WLL_QUERY_OP_EQUAL) {
				trio_asprintf(&conds, "(e.time_stamp = %s AND e.usec = %d)",
						dbt, ec[m][n].value.t.tv_usec);
			}
			else
				trio_asprintf(&conds, "e.time_stamp %s %s", opToString(ec[m][n].op), dbt);
			break;

		case EDG_WLL_QUERY_ATTR_LEVEL:
			if ( conds )
			{
				if ( ec[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
					trio_asprintf(&out, "%s OR (e.level >= %d AND e.level <= %d)", conds, ec[m][n].value.i, ec[m][n].value2.i);
				else
					trio_asprintf(&out, "%s OR e.level %s %d", conds, opToString(ec[m][n].op), ec[m][n].value.i);
				free(conds); conds = out;
			}
			else
				if ( ec[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
					trio_asprintf(&conds, "(e.level >= %d AND e.level <= %d)", ec[m][n].value.i, ec[m][n].value2.i);
				else
					trio_asprintf(&conds, "e.level %s %d", opToString(ec[m][n].op), ec[m][n].value.i);
			break;

		case EDG_WLL_QUERY_ATTR_SOURCE:
			aux = edg_wll_SourceToString(ec[m][n].value.i);
			if ( conds )
			{
				trio_asprintf(&out, "%s OR e.prog %s '%|Ss'", conds, opToString(ec[m][n].op), aux);
				free(conds); conds = out;
			}
			else
				trio_asprintf(&conds, "e.prog %s '%|Ss'", opToString(ec[m][n].op), aux);
			free(aux);
			break;

		case EDG_WLL_QUERY_ATTR_HOST:
			if ( conds )
			{
				trio_asprintf(&out, "%s OR e.host %s '%|Ss'", conds, opToString(ec[m][n].op), ec[m][n].value.c);
				free(conds); conds = out;
			}
			else
				trio_asprintf(&conds, "e.host %s '%|Ss'", opToString(ec[m][n].op), ec[m][n].value.c);
			break;

		case EDG_WLL_QUERY_ATTR_USERTAG:
			if ( conds )
			{
				trio_asprintf(&out, "%s OR e.code = %d", conds, EDG_WLL_EVENT_USERTAG);
				free(conds); conds = out;
			}
			else
				trio_asprintf(&conds, "e.code = %d", EDG_WLL_EVENT_USERTAG);
			break;
		case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
			if ( conds )
			{
				if ( ec[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
					trio_asprintf(&out, "%s OR (e.code >= %d AND e.code <= %d)", conds, ec[m][n].value.i, ec[m][n].value2.i);
				else
					trio_asprintf(&out, "%s OR e.code %s %d", conds, opToString(ec[m][n].op), ec[m][n].value.i);
				free(conds); conds = out;
			}
			else
				if ( ec[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
					trio_asprintf(&conds, "(e.code >= %d AND e.code <= %d)", ec[m][n].value.i, ec[m][n].value2.i);
				else
					trio_asprintf(&conds, "e.code %s %d", opToString(ec[m][n].op), ec[m][n].value.i);
			break;

		default:
			return NULL;
		}

		if ( !conds )
			continue;	

		if ( retconds )
		{
			trio_asprintf(&out, "%s AND (%s)", retconds, conds);
			free(retconds); retconds = out;
			free(conds); conds = NULL;
		}
		else
		{
			trio_asprintf(&retconds, "(%s)", conds);
			free(conds); conds = NULL;
		}
	}

	return retconds;
}

static int is_indexed(const edg_wll_QueryRec *cond, const edg_wll_Context ctx)
{
	int		i, j;

	if ( !(ctx->job_index) )
		return 0;

	for ( i = 0; ctx->job_index[i]; i++ ) for ( j = 0; ctx->job_index[i][j].attr; j++ )
		if ( !edg_wll_CmpColumn(&ctx->job_index[i][j], cond) ) return 1;
	
	return 0;
}

/*
 *	use where_flags to recognize which table to jois in SQL select
 *	
 */
static char *jc_to_head_where(
				edg_wll_Context ctx,
				const edg_wll_QueryRec **jc,
				int *where_flags)
{
	int		ct, n, m;
	char   *aux,
		   *tmps,
		   *tmps2,
		   *dbt,
		   *cname = NULL,
			msg[100];
	char   *conds, *retconds;
	char 	*can_peername = NULL;

	retconds = conds = NULL;

	/*
	 * check correctness only
	 */
	for ( ct = m = 0; jc[m]; m++ ) for ( n = 0; jc[m][n].attr; n++ )
	{
		if (   (jc[m][0].attr != jc[m][n].attr)
			|| (   (jc[m][n].attr == EDG_WLL_QUERY_ATTR_USERTAG)
				&& strcmp(jc[m][0].attr_id.tag, jc[m][n].attr_id.tag)) )
		{
			edg_wll_SetError(ctx, EINVAL, "only same attribute types supported in 'or' expressions");
			return NULL;
		}

		switch ( jc[m][n].attr )
		{
		case EDG_WLL_QUERY_ATTR_JOBID:
			ct++;
			if ( jc[m][n].op != EDG_WLL_QUERY_OP_EQUAL && jc[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with jobid");
				return NULL;
			}
			break;

		case EDG_WLL_QUERY_ATTR_OWNER:
			ct++;
			if ( jc[m][n].op != EDG_WLL_QUERY_OP_EQUAL && jc[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with job owner");
				return NULL;
			}
			break;

		case EDG_WLL_QUERY_ATTR_LOCATION:
			ct++;
			if ( jc[m][n].op != EDG_WLL_QUERY_OP_EQUAL && jc[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with job location");
				return NULL;
			}
			break;

		case EDG_WLL_QUERY_ATTR_DESTINATION:
			ct++;
			if ( jc[m][n].op != EDG_WLL_QUERY_OP_EQUAL && jc[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with job destinations");
				return NULL;
			}
			break;

		case EDG_WLL_QUERY_ATTR_PARENT:
			ct++;
			if ( jc[m][n].op != EDG_WLL_QUERY_OP_EQUAL && jc[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with job parent");
				return NULL;
			}
			break;

		case EDG_WLL_QUERY_ATTR_RESUBMITTED:
			ct++;
			if ( jc[m][n].op != EDG_WLL_QUERY_OP_EQUAL && jc[m][n].op != EDG_WLL_QUERY_OP_UNEQUAL )
			{
				edg_wll_SetError(ctx, EINVAL, "only `=' and '!=' supported with resubmitted attr");
				return NULL;
			}
			break;

		case EDG_WLL_QUERY_ATTR_TIME:
			if ( jc[m][n].attr_id.state == EDG_WLL_JOB_UNDEF )
			{
				edg_wll_SetError(ctx, EINVAL, "Time attribut have to be associated with status specification");
				return NULL;
			}
			ct++;
			break;
		case EDG_WLL_QUERY_ATTR_DONECODE:
		case EDG_WLL_QUERY_ATTR_EXITCODE:
		case EDG_WLL_QUERY_ATTR_STATUS:
			ct++;
			break;

		case EDG_WLL_QUERY_ATTR_USERTAG:
			ct++;
			break;

		default:
			sprintf(msg, "jc_to_head_where(): attr=%d unsupported", jc[m][n].attr);
			edg_wll_SetError(ctx, EINVAL, msg);
			return NULL;
		}
	}

	if ( ct == 0 )
	{
		edg_wll_SetError(ctx, EINVAL, "jc_to_head_where(): empty conditions");
		return NULL;
	}

	/*
	 * Now process conversion
	 */
	for ( m = 0; jc[m]; m++ )
	{
		for ( n = 0; jc[m][n].attr; n++ ) switch (jc[m][n].attr)
		{
		case EDG_WLL_QUERY_ATTR_TIME:
			if (   !is_indexed(&(jc[m][n]), ctx)
				|| !(cname = edg_wll_QueryRecToColumn(&(jc[m][n]))) )
				break;

			*where_flags |= FL_SEL_STATUS;

			dbt = edg_wll_TimeToDB(jc[m][n].value.t.tv_sec);
			if ( conds )
			{
				if ( jc[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
				{
					trio_asprintf(&aux, "%s", dbt);
					dbt = edg_wll_TimeToDB(jc[m][n].value2.t.tv_sec);
					trio_asprintf(&tmps, "%s OR (s.%s >= %s AND s.%s <= %s)", conds, cname, aux, cname, dbt);
					free(aux);
				}
				else
					trio_asprintf(&tmps, "%s OR s.%s %s %s", conds, cname, opToString(jc[m][n].op), dbt);

				free(conds);
				conds = tmps;
			}
			else if ( jc[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
			{
				trio_asprintf(&aux, "%s", dbt);
				dbt = edg_wll_TimeToDB(jc[m][n].value2.t.tv_sec);
				trio_asprintf(&conds, "(s.%s >= %s AND s.%s <= %s)", cname, aux, cname, dbt);
				free(aux);
			}
			else
				trio_asprintf(&conds, "s.%s %s %s", cname, opToString(jc[m][n].op), dbt);

			free(cname);
			break;

		case EDG_WLL_QUERY_ATTR_JOBID:
			*where_flags |= FL_SEL_JOB;
			aux = edg_wlc_JobIdGetUnique(jc[m][n].value.j);
			if ( conds )
			{
				trio_asprintf(&tmps, "%s OR j.jobid%s'%|Ss'", conds, opToString(jc[m][n].op), aux);
				free(conds); conds = tmps;
			}
			else
				trio_asprintf(&conds, "j.jobid%s'%|Ss'", opToString(jc[m][n].op), aux);
			free(aux);
			break;

		case EDG_WLL_QUERY_ATTR_PARENT:
			if (   !is_indexed(&(jc[m][n]), ctx)
				|| !(cname = edg_wll_QueryRecToColumn(&(jc[m][n]))) )
				break;

			*where_flags |= FL_SEL_STATUS;
			aux = edg_wlc_JobIdGetUnique(jc[m][n].value.j);
			if ( conds )
			{
				trio_asprintf(&tmps, "%s OR s.%s%s'%|Ss'", conds, cname, opToString(jc[m][n].op), aux);
				free(conds); conds = tmps;
			}
			else
				trio_asprintf(&conds, "s.%s%s'%|Ss'", cname, opToString(jc[m][n].op), aux);
			free(aux);
			break;

		case EDG_WLL_QUERY_ATTR_OWNER:
			if (   !is_indexed(&(jc[m][n]), ctx)
				|| !(cname = edg_wll_QueryRecToColumn(&(jc[m][n]))) )
				break;

			if ( !jc[m][n].value.c && !ctx->peerName ) 
			{ 
				edg_wll_SetError(ctx, EPERM, "jc_to_head_where(): ctx->peerName empty");
				free(cname); free(conds); free(retconds);
				return NULL;
			}	

			tmps2 = edg_wll_gss_normalize_subj(jc[m][n].value.c, 0);
			if (!jc[m][n].value.c && !can_peername) {
				can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
			}

			*where_flags |= FL_SEL_STATUS;
			if ( conds )
			{
				if ( jc[m][n].value.c ) 
					trio_asprintf(&tmps, "%s OR s.%s%s'%|Ss'", conds, cname, opToString(jc[m][n].op), tmps2);
				else
					trio_asprintf(&tmps, "%s OR s.%s%s'%|Ss'", conds, cname, opToString(jc[m][n].op), can_peername);
				free(conds); conds = tmps;
			}
			else
			{
				if ( jc[m][n].value.c ) 
					trio_asprintf(&conds, "s.%s%s'%|Ss'", cname, opToString(jc[m][n].op), tmps2);
				else
					trio_asprintf(&conds, "s.%s%s'%|Ss'", cname, opToString(jc[m][n].op), can_peername);
			}
			free(tmps2);
			break;

		case EDG_WLL_QUERY_ATTR_DONECODE:
		case EDG_WLL_QUERY_ATTR_EXITCODE:
		case EDG_WLL_QUERY_ATTR_STATUS:
			if (   !is_indexed(&(jc[m][n]), ctx)
				|| !(cname = edg_wll_QueryRecToColumn(&(jc[m][n]))) )
				break;

			*where_flags |= FL_SEL_STATUS;
			if ( conds )
			{
				if ( jc[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
					trio_asprintf(&tmps, "%s OR (s.%s >= %d AND s.%s <= %d)", conds, cname, jc[m][n].value.i, cname, jc[m][n].value2.i);
				else
					trio_asprintf(&tmps, "%s OR s.%s %s %d", conds, cname, opToString(jc[m][n].op), jc[m][n].value.i);
				free(conds); conds = tmps;
			}
			else
				if ( jc[m][n].op == EDG_WLL_QUERY_OP_WITHIN )
					trio_asprintf(&conds, "(s.%s >= %d AND s.%s <= %d)", cname, jc[m][n].value.i, cname, jc[m][n].value2.i);
				else
					trio_asprintf(&conds, "s.%s %s %d", cname, opToString(jc[m][n].op),jc[m][n].value.i);

			free(cname);
			break;

		case EDG_WLL_QUERY_ATTR_DESTINATION:
		case EDG_WLL_QUERY_ATTR_LOCATION:
		case EDG_WLL_QUERY_ATTR_RESUBMITTED:
		case EDG_WLL_QUERY_ATTR_USERTAG:
			if (   !is_indexed(&(jc[m][n]), ctx)
				|| !(cname = edg_wll_QueryRecToColumn(&(jc[m][n]))) )
				break;

			*where_flags |= FL_SEL_STATUS;
			if ( conds )
			{
				trio_asprintf(&tmps, "%s OR s.%s%s'%s'", conds, cname, opToString(jc[m][n].op), jc[m][n].value.c);
				free(conds); conds = tmps;
			}
			else
				trio_asprintf(&conds, "s.%s%s'%s'", cname, opToString(jc[m][n].op), jc[m][n].value.c);

			free(cname);
			break;

		default:
			/* this may never occure, but keep compiler happy */
			break;
		}

		if ( !conds ) continue;

		if ( retconds )
		{
			trio_asprintf(&tmps, "%s AND (%s)", retconds, conds);
			free(retconds); retconds = tmps;
			free(conds); conds = NULL;
		}
		else
		{
			trio_asprintf(&retconds, "(%s)", conds);
			free(conds); conds = NULL;
		}
	}

	free(can_peername);
	return retconds;
}


static int match_flesh_conditions(const edg_wll_Event *e,const edg_wll_QueryRec **ec)
{
	int			i, j;


	if ( !ec )
		return 1;

	for ( i = 0; ec[i]; i++ )
	{
		j = 0;
		while (    (ec[i][j].attr)
				&& (ec[i][j].attr != EDG_WLL_QUERY_ATTR_USERTAG)
				&& (ec[i][j].attr != EDG_WLL_QUERY_ATTR_INSTANCE) ) j++;
		if ( !ec[i][j].attr )
			continue;

		for ( j = 0; ec[i][j].attr; j++ )
		{
			if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_TIME )
			{
				if (   ((ec[i][j].op == EDG_WLL_QUERY_OP_EQUAL) && (e->any.timestamp.tv_sec == ec[i][j].value.t.tv_sec))
					|| ((ec[i][j].op == EDG_WLL_QUERY_OP_LESS) && (e->any.timestamp.tv_sec < ec[i][j].value.t.tv_sec))
					|| ((ec[i][j].op == EDG_WLL_QUERY_OP_GREATER) && (e->any.timestamp.tv_sec < ec[i][j].value.t.tv_sec))
					|| (	(ec[i][j].op == EDG_WLL_QUERY_OP_WITHIN)
						&& (e->any.timestamp.tv_sec >= ec[i][j].value.t.tv_sec)
						&& (e->any.timestamp.tv_sec <= ec[i][j].value2.t.tv_sec)) )
					break;
			}
			else if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_SOURCE )
			{
				if ( e->any.source == EDG_WLL_SOURCE_NONE )
					continue;
				if (   ((ec[i][j].op == EDG_WLL_QUERY_OP_EQUAL) && (e->any.source == ec[i][j].value.i))
					|| ((ec[i][j].op == EDG_WLL_QUERY_OP_LESS) && (e->any.source < ec[i][j].value.i))
					|| ((ec[i][j].op == EDG_WLL_QUERY_OP_GREATER) && (e->any.source < ec[i][j].value.i))
					|| (   (ec[i][j].op == EDG_WLL_QUERY_OP_WITHIN)
						&& (e->any.source >= ec[i][j].value.i)
						&& (e->any.source <= ec[i][j].value2.i)) )
					break;
			}
			else if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_LEVEL )
			{
				if (   ((ec[i][j].op == EDG_WLL_QUERY_OP_EQUAL) && (e->any.level == ec[i][j].value.i))
					|| ((ec[i][j].op == EDG_WLL_QUERY_OP_LESS) && (e->any.level < ec[i][j].value.i))
					|| ((ec[i][j].op == EDG_WLL_QUERY_OP_GREATER) && (e->any.level < ec[i][j].value.i))
					|| (   (ec[i][j].op == EDG_WLL_QUERY_OP_WITHIN)
						&& (e->any.level >= ec[i][j].value.i)
						&& (e->any.level <= ec[i][j].value2.i)) )
					break;
			}
			else if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_HOST )
			{
				if ( !strcmp(ec[i][j].value.c, e->any.host) )
					break;
			}
			else if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_EVENT_TYPE )
			{
				if ( e->any.type == ec[i][j].value.i )
					break;
			}
			else if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_USERTAG )
			{
				if ( e->any.type == EDG_WLL_EVENT_USERTAG && 
					!strcmp(ec[i][j].attr_id.tag,e->userTag.name) 
					 && cmp_string(e->userTag.value,ec[i][j].op,ec[i][j].value.c))
					break;
			}
			else if ( ec[i][j].attr == EDG_WLL_QUERY_ATTR_INSTANCE )
			{
				if (	e->any.src_instance
					 && cmp_string(ec[i][j].value.c, ec[i][j].op,  e->any.src_instance) )
					break;
			}
		}
		if ( !ec[i][j].attr )
			/*
			 *	No condition in "or" clause is valid
			 */
			return 0;
	}

	return 1;
}

/* XXX: has to match exactly the main query in edg_wll_QueryEvents 
 *      and similar one in srv_purge.c 
 */
int convert_event_head(edg_wll_Context ctx,char **f,edg_wll_Event *e)
{
	int	ret,i;

	memset(e,0,sizeof *e);
	edg_wll_ResetError(ctx);


	if ((ret=edg_wlc_JobIdParse(f[0],&e->any.jobId))) {
		edg_wll_SetError(ctx,-ret,"edg_wlc_JobIdParse()");
		goto err;
	}

	e->type = atoi(f[1]);
	free(f[1]); f[1] = NULL;
	
	e->any.source = edg_wll_StringToSource(f[2]); 
	free(f[2]); f[2] = NULL;
	
	e->any.host = f[3];
	f[3] = NULL;
	
	e->any.user = f[4];
	f[4] = NULL;
	
	e->any.timestamp.tv_sec = edg_wll_DBToTime(f[5]);
	free(f[5]); f[5] = NULL;
	
	e->any.timestamp.tv_usec = atoi(f[6]);
	free(f[6]); f[6] = NULL;

	e->any.level = atoi(f[7]); 
	free(f[7]); f[7] = NULL;

	e->any.arrived.tv_sec = edg_wll_DBToTime(f[8]); 
	e->any.arrived.tv_usec = 0;
	free(f[8]); f[8] = NULL;

	return 0;

err:
	edg_wll_FreeEvent(e);
	memset(e,0,sizeof *e);
	for (i=0; i<9; i++) free(f[i]);
	return edg_wll_Error(ctx,NULL,NULL);
}



/* FIXME: other op's, ORed conditions */
int match_status(edg_wll_Context ctx, const edg_wll_JobStat *stat, const edg_wll_QueryRec **conds)
{
	int		i, j, n;
	char   *s, *s1;


	if ( !conds ) return 1;

	for ( i = 0; conds[i]; i++ )
	{
		for ( j = 0; conds[i][j].attr; j++ )
		{
			if ( is_indexed(&(conds[i][j]), ctx) ) goto or_satisfied;

			switch ( conds[i][j].attr )
			{
			case EDG_WLL_QUERY_ATTR_STATUS: 
				switch ( conds[i][j].op )
				{
				case EDG_WLL_QUERY_OP_EQUAL:
					if ( conds[i][j].value.i == stat->state ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_UNEQUAL:
					if ( conds[i][j].value.i != stat->state ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_LESS:
					if ( conds[i][j].value.i > stat->state ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_GREATER:
					if ( conds[i][j].value.i < stat->state ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_WITHIN:
					if (   conds[i][j].value.i <= stat->state
						&& conds[i][j].value2.i >= stat->state ) goto or_satisfied;
					break;
				}
				break;
			case EDG_WLL_QUERY_ATTR_RESUBMITTED:
				if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) {
					if ( conds[i][j].value.i == stat->resubmitted ) goto or_satisfied;
				} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL )
					if ( conds[i][j].value.i != stat->resubmitted ) goto or_satisfied;
				break;
			case EDG_WLL_QUERY_ATTR_DONECODE:
				switch ( conds[i][j].op )
				{
				case EDG_WLL_QUERY_OP_EQUAL:
					if ( conds[i][j].value.i == stat->done_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_UNEQUAL:
					if ( conds[i][j].value.i != stat->done_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_LESS:
					if ( conds[i][j].value.i > stat->done_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_GREATER:
					if ( conds[i][j].value.i < stat->done_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_WITHIN:
					if (   conds[i][j].value.i <= stat->done_code
						&& conds[i][j].value2.i >= stat->done_code ) goto or_satisfied;
					break;
				}
				break;
			case EDG_WLL_QUERY_ATTR_EXITCODE:
				switch ( conds[i][j].op )
				{
				case EDG_WLL_QUERY_OP_EQUAL:
					if ( conds[i][j].value.i == stat->exit_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_UNEQUAL:
					if ( conds[i][j].value.i != stat->exit_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_LESS:
					if ( conds[i][j].value.i > stat->exit_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_GREATER:
					if ( conds[i][j].value.i < stat->exit_code ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_WITHIN:
					if (   conds[i][j].value.i <= stat->exit_code
						&& conds[i][j].value2.i >= stat->exit_code ) goto or_satisfied;
					break;
				}
				break;
			case EDG_WLL_QUERY_ATTR_OWNER:
				if (stat->owner) {
					if (conds[i][j].value.c) {
						if (edg_wll_gss_equal_subj(conds[i][j].value.c, stat->owner) ) {
							if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
						} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;
					} else if (ctx->peerName) {
						if (edg_wll_gss_equal_subj(ctx->peerName, stat->owner) ) {
							if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
						} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;
					}
				}
				break;
			case EDG_WLL_QUERY_ATTR_LOCATION:
				if ( stat->location )
				{
					if ( !strcmp(conds[i][j].value.c, stat->location) ) {
						if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
					} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;
				}
				break;
			case EDG_WLL_QUERY_ATTR_DESTINATION:
				if ( stat->destination )
				{
					if ( !strcmp(conds[i][j].value.c, stat->destination) ) {
						if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
					} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;
				}
				break;
			case EDG_WLL_QUERY_ATTR_JOBID:
				if ( !stat->jobId )
					break;
				s = edg_wlc_JobIdUnparse(stat->jobId);
				s1 = edg_wlc_JobIdUnparse(conds[i][j].value.j);
				if ( s && s1 )
				{
					int r = strcmp(s1, s);
					free(s); free(s1);
					if ( !r ) {
						if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
					} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;
				}
				break;
			case EDG_WLL_QUERY_ATTR_PARENT:
				if ( !stat->parent_job )
					break;
				s = edg_wlc_JobIdUnparse(stat->parent_job);
				s1 = edg_wlc_JobIdUnparse(conds[i][j].value.j);
				if ( s && s1 )
				{
					int r = strcmp(s1, s);
					free(s); free(s1);
					if ( !r ) {
						if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
					} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;
				}
				break;
			case EDG_WLL_QUERY_ATTR_USERTAG:
				if ( !stat->user_tags )
				{
					if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL )
						goto or_satisfied;

					break;
				}
				for ( n = 0; stat->user_tags[n].tag; n++ )
					if ( !strcasecmp(conds[i][j].attr_id.tag, stat->user_tags[n].tag) )
					{
						if ( !strcmp(conds[i][j].value.c, stat->user_tags[n].value) ) {
							if ( conds[i][j].op == EDG_WLL_QUERY_OP_EQUAL ) goto or_satisfied;
						} else if ( conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL ) goto or_satisfied;

						break;
					}
				if ( !stat->user_tags[n].tag && conds[i][j].op == EDG_WLL_QUERY_OP_UNEQUAL )
					goto or_satisfied;
				break;
			case EDG_WLL_QUERY_ATTR_TIME:
				if ( !stat->stateEnterTimes || !stat->stateEnterTimes[1+conds[i][j].attr_id.state] )
					break;
				switch ( conds[i][j].op )
				{
				case EDG_WLL_QUERY_OP_EQUAL:
					if ( conds[i][j].value.t.tv_sec == stat->stateEnterTimes[1+conds[i][j].attr_id.state] ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_UNEQUAL:
					if ( conds[i][j].value.t.tv_sec != stat->stateEnterTimes[1+conds[i][j].attr_id.state] ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_LESS:
					if ( conds[i][j].value.t.tv_sec > stat->stateEnterTimes[1+conds[i][j].attr_id.state] ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_GREATER:
					if ( conds[i][j].value.t.tv_sec < stat->stateEnterTimes[1+conds[i][j].attr_id.state] ) goto or_satisfied;
					break;
				case EDG_WLL_QUERY_OP_WITHIN:
					if (   conds[i][j].value.t.tv_sec <= stat->stateEnterTimes[1+conds[i][j].attr_id.state]
						&& conds[i][j].value2.t.tv_sec >= stat->stateEnterTimes[1+conds[i][j].attr_id.state] ) goto or_satisfied;
					break;
				}
				break;
			default:
				break;
			}
		}

			/*
			 *	No one condition in "or clause satisfied
			 */
		return 0;

or_satisfied:
                // just for escaping from nested cycles
		;       /* prevent compiler to complain */
	}

	return 1;
}

static int cmp_string(const char *s1,edg_wll_QueryOp op,const char *s2)
{
	switch (op) {
		case EDG_WLL_QUERY_OP_EQUAL:	return !strcmp(s1,s2);
		case EDG_WLL_QUERY_OP_LESS:	return strcmp(s1,s2)<0;
		case EDG_WLL_QUERY_OP_GREATER:	return strcmp(s1,s2)>0;
		default: return 0;
	}
	return 0;
}


int check_strict_jobid(edg_wll_Context ctx, const edg_wlc_JobId job)
{
	char	*job_host;
	unsigned int	job_port;

	edg_wll_ResetError(ctx);

	/* Allow all jobids when server name is not set. */
	if ( (ctx->srvName == NULL) || (ctx->isProxy)) return edg_wll_Error(ctx,NULL,NULL);

	edg_wlc_JobIdGetServerParts(job,&job_host,&job_port);

	if (strcasecmp(job_host,ctx->srvName) || job_port != ctx->srvPort)
	{
		char	*jobid,msg[300];

		jobid = edg_wlc_JobIdUnparse(job);
		snprintf(msg,sizeof msg,"%s: does not match server address",jobid);
		msg[sizeof msg - 1] = 0;
		edg_wll_SetError(ctx,EINVAL,msg);
		free(jobid);
	}

	free(job_host);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int check_strict_jobid_cond(edg_wll_Context ctx, const edg_wll_QueryRec **cond)
{
	int	i,j,ret;

	if (!cond) return edg_wll_ResetError(ctx);
	for (i=0; cond[i]; i++) for (j=0; cond[i][j].attr; j++) 
		if (cond[i][j].attr == EDG_WLL_QUERY_ATTR_JOBID &&
			(ret = check_strict_jobid(ctx,cond[i][j].value.j)))
				return ret;

	return 0;
}

static int is_all_query(const edg_wll_QueryRec **jc)
{
	if (!jc || !*jc) return 1;

	if (jc[0][0].attr == EDG_WLL_QUERY_ATTR_OWNER && 
	    jc[0][0].op == EDG_WLL_QUERY_OP_EQUAL &&
	    !jc[0][1].attr && !jc[1]) return 1;

	return 0;
}
