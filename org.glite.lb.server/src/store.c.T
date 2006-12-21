#ident "$Header$"

/*

@@@AUTO

 * XXX: still lots of hardcoded stuff
 *	there's mapping db.column <-> event struct field
 */

@@@LANG: C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <syslog.h>

#include <globus_config.h>

#include "glite/lb-utils/strmd5.h"

#include "glite/lb-utils/db.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/context-int.h"
#include "glite/lb/producer.h"
#include "glite/lb/log_proto.h"	/* for EDG_WLL_LOG_USER_DEFAULT */
#include "glite/lb-utils/trio.h"

#include "db_supp.h"
#include "store.h"
#include "get_events.h"
#include "lock.h"
#include "lb_authz.h"
#include "jobstat.h"

static int store_user(edg_wll_Context,const char *,const char *); 
static int store_job(edg_wll_Context,const glite_lbu_JobId,const char *);
#ifdef LB_BUF
static int store_job_block(edg_wll_Context, const glite_lbu_JobId, const char *, glite_lbu_bufInsert);
#endif
static int store_flesh(edg_wll_Context,edg_wll_Event *,char *,int);
static int store_seq(edg_wll_Context,edg_wll_Event *,int);
static int check_dup(edg_wll_Context,edg_wll_Event *);
static int check_auth(edg_wll_Context,edg_wll_Event *e); 
#ifdef LB_DAG_EMBRIONIC
static int register_subjobs_embryonic(edg_wll_Context,const edg_wll_RegJobEvent *);
#else
static int register_subjobs(edg_wll_Context,const edg_wll_RegJobEvent *);
#endif

void edg_wll_StoreAnonymous(edg_wll_Context ctx,int anon) {
	ctx->allowAnonymous = anon;
}

int edg_wll_StoreEvent(edg_wll_Context ctx,edg_wll_Event *e,int *seq)
{
	edg_wll_ErrorCode	err = 0;
	char		*userid = NULL,*jobid,*stmt;
	char		*select_max,*ssrc;
	glite_lbu_Statement	sh;
	int		next = 0xDEAD;
	int		lbproxy_notreg = 0;
	char		*now_s = NULL;

	ssrc = jobid = stmt = select_max = NULL;

	if ( ctx->event_load )
		glite_lbu_TimeToDB(e->any.arrived.tv_sec, &now_s);
	else
		glite_lbu_TimeToDB(time(NULL), &now_s);
	edg_wll_ResetError(ctx);
	switch (err = check_auth(ctx,e)) {
		case 0: break;
		case ENOENT: 
			if ( !ctx->isProxy ) goto clean;
			edg_wll_ResetError(ctx);
			lbproxy_notreg = 1;
			break;
		case EPERM:
			if (!ctx->noAuth) goto clean;
			edg_wll_ResetError(ctx);
			break;
		default: goto clean;
	}
	if ((err = check_dup(ctx,e))) goto clean;

	userid = str2md5(e->any.user);

/* make sure user record is there */
	if ((err = store_user(ctx,userid,e->any.user))) goto clean;

	jobid = glite_lbu_JobIdGetUnique(e->any.jobId);

/* only REGJOB events determine job owner now */
	if (   (e->type == EDG_WLL_EVENT_REGJOB || lbproxy_notreg)
		&& (err = store_job(ctx,e->any.jobId,userid))) goto clean;


/* obtain next event sequence number */
	trio_asprintf(&select_max,
		"select max(event) from events "
		"where jobid = '%|Ss'",jobid);

	ssrc = edg_wll_SourceToString(e->any.source);

/* try to insert (someone else may be doing the same) */
	while (1) {
		char	*max, *stamp;

		if (glite_lbu_ExecSQL(ctx->dbctx,select_max,&sh) < 0 ||
		    glite_lbu_FetchRow(sh,1,NULL,&max) < 0)
		{
			err = edg_wll_SetErrorDB(ctx);
			goto clean;
		}
		glite_lbu_FreeStmt(&sh);
		
		next = max && *max ? atoi(max)+1 : 0;
		
	/* 
	 * 1) when using transactions:
	 *   Store the whole event right now.
	 *
	 * 2) when not using transactions:
	 *   Store an UNDEF event first in order to prevent race condition
	 *   with readers and update event code later.
	 */
		glite_lbu_TimeToDB(e->any.timestamp.tv_sec, &stamp);
		trio_asprintf(&stmt,
			"insert into events(jobid,event,code,prog,host,time_stamp,usec,arrived,level,userid) "
			"values ('%|Ss',%d,%d,'%|Ss','%|Ss',%s,%d,%s,%d,'%|Ss')",
			jobid,next,
			glite_lbu_DBQueryCaps(ctx->dbctx) & GLITE_LBU_DB_CAP_TRANSACTIONS ? (int) e->any.type : EDG_WLL_EVENT_UNDEF,
			ssrc,e->any.host,
			stamp,e->any.timestamp.tv_usec,
			now_s, e->any.level,userid);
		free(stamp);

		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) {
			if ((err = edg_wll_SetErrorDB(ctx)) != EEXIST)
				goto clean;
		} else break; /* successful insert */

	/* we were late -- try once again */
		free(stmt);
	}

	free(stmt); stmt = NULL;
	if ((err = store_seq(ctx,e,next)) ||
		(err = store_flesh(ctx,e,jobid,next))) {
	/* attempt to cleanup, ignore new errors */
		char		*desc;
		edg_wll_ErrorCode	oerr = edg_wll_Error(ctx,NULL,&desc);

		edg_wll_delete_event(ctx,jobid,next);
		edg_wll_SetError(ctx,oerr,desc);
		free(desc);
	} else
	if ((glite_lbu_DBQueryCaps(ctx->dbctx) & GLITE_LBU_DB_CAP_TRANSACTIONS) == 0) {
	/* emulate commit, i.e. swith to a real event type to make
	 * the record valid */
		trio_asprintf(&stmt,
			"update events set code=%d "
			"where jobid='%|Ss' and event=%d",
			(int) e->any.type,jobid,next);
		switch (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL)) {
			case 0: if (ctx->strict_locking)
					err = edg_wll_SetError(ctx,ENOENT,"event disappeared on store while strict locking");
			/* purge in progres: drop the garbage, ignore errors */
				else {
					edg_wll_delete_event(ctx,jobid,next);
					err = edg_wll_SetError(ctx,ENOENT,"job being purged");
				}
				break;
			case 1: if (ctx->strict_locking) err = 0;
				else {
			/* check whether the job is still there to prevent garbage
			 * left while there is a concurrent purge 
			 */
					free(stmt);
					trio_asprintf(&stmt,
						"select 'x' from jobs where jobid='%|Ss'",
						jobid);
					switch (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL)) {
						case 1: break;
						case 0: /* purge in progres */
							edg_wll_delete_event(ctx,jobid,next);
							err = edg_wll_SetError(ctx,ENOENT,"job being purged");
							break;
						case -1: err = edg_wll_SetErrorDB(ctx); break;
						default: err = edg_wll_SetError(ctx,EDG_WLL_ERROR_DB_CALL,
								"more job records, what is that?");
							break;
					}
				}
				break;
			case -1: err = edg_wll_SetErrorDB(ctx);
				break;
			default: err = edg_wll_SetError(ctx,EDG_WLL_ERROR_DB_CALL,
				"more event records, what is that?");
				break;
		}
	} /* !transactions */

	if (err == 0 && 
		e->any.type == EDG_WLL_EVENT_REGJOB &&
		(e->regJob.jobtype == EDG_WLL_REGJOB_DAG ||
		 e->regJob.jobtype == EDG_WLL_REGJOB_PARTITIONED ||
		 e->regJob.jobtype == EDG_WLL_REGJOB_COLLECTION) &&
		e->regJob.nsubjobs > 0)  

#ifdef LB_DAG_EMBRIONIC
			err = register_subjobs_embryonic(ctx,&e->regJob);
#else
			err = register_subjobs(ctx,&e->regJob);
#endif


clean:
	free(now_s);
	free(userid);
	free(jobid);
	free(stmt);
	free(ssrc);
	free(select_max);
	if (!err && seq) *seq = next;
	return err;
}

static int store_user(edg_wll_Context ctx,const char *userid,const char *subj)
{
	char	*stmt;

	trio_asprintf(&stmt,"insert into users(userid,cert_subj) "
		"values ('%|Ss','%|Ss')",userid,subj);

	if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) {
		if (edg_wll_SetErrorDB(ctx) == EEXIST)
			edg_wll_ResetError(ctx);
	}

	free(stmt);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int store_job(edg_wll_Context ctx,const glite_lbu_JobId job,const char *userid)
{
	char *jobstr = glite_lbu_JobIdUnparse(job);
	char *jobid = glite_lbu_JobIdGetUnique(job);
	char *stmt;

/* debug Duplicate key on index: Duplicate entry '(nil)' for key 1
 */
	if (jobid == NULL || jobstr == NULL) 
		return edg_wll_SetError(ctx,EINVAL,"store_job()");

	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"insert into jobs(jobid,dg_jobid,userid) "
		"values ('%|Ss','%|Ss','%|Ss')",jobid,jobstr,userid);

	if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) {
		if (edg_wll_SetErrorDB(ctx) == EEXIST) 
			edg_wll_ResetError(ctx);
	}
	free(stmt);
	free(jobstr);
	free(jobid);
	return edg_wll_Error(ctx,NULL,NULL);
}

#ifdef LB_BUF
static int store_job_block(edg_wll_Context ctx,const glite_lbu_JobId job,const char *userid, glite_lbu_bufInsert bi)
{
	char *jobstr = glite_lbu_JobIdUnparse(job);
	char *jobid = glite_lbu_JobIdGetUnique(job);
	char *row;

/* debug Duplicate key on index: Duplicate entry '(nil)' for key 1
 */
	if (jobid == NULL || jobstr == NULL) 
		return edg_wll_SetError(ctx,EINVAL,"store_jobi_block()");

	edg_wll_ResetError(ctx);

	trio_asprintf(&row, "'%|Ss','%|Ss','%|Ss'", jobid,jobstr,userid);
	glite_lbu_bufferedInsert(bi, row); // no need to free row
	edg_wll_SetErrorDB(ctx);

	free(jobstr);
	free(jobid);
	return edg_wll_Error(ctx,NULL,NULL);
}
#endif

/*
 * XXX: store it in SHORT_FIELDS for now despite it should go to dedicated
 * 	column in EVENTS.
 *
 * 	don't want to change the database structure now, will be done anyway
 * 	soon
 */
static int store_seq(edg_wll_Context ctx,edg_wll_Event *e,int no)
{
	int	ret;
	char	*stmt;
	char	*jobid = glite_lbu_JobIdGetUnique(e->any.jobId);

	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"insert into short_fields(jobid,event,name,value) "
			"values ('%|Ss',%d,'SEQCODE','%|Ss')",
			jobid,no,e->any.seqcode);

	ret = glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL);
	edg_wll_SetErrorDB(ctx);

	free(stmt);
	free(jobid);

	return ret>=0 ? 0 : edg_wll_Error(ctx,NULL,NULL);
}

#define SHORT_LEN	255	/* short_fiels.value db column lenght */

static int store_flesh(edg_wll_Context ctx,edg_wll_Event *e,char *jobid,int no)
{
	struct {
		char	*key;
		char	*val;
	} f[20];

	char	*stmt;
	unsigned int	i;
	int	err = 0;

	edg_wll_ResetError(ctx);
	memset(f,0,sizeof(f)); assert(f[0].key == NULL);

	switch (e->type) {
@@@{
	for my $type (getTypesOrdered $event) {
		next if $type eq '_common_';
		selectType $event $type;
		my $uctype = uc $type;
		my $flctype = lcfirst $type;
		gen qq{
!		case EDG_WLL_EVENT_$uctype:
};
		my $idx = 0;
		for (getFieldsOrdered $event) {
			my $f = selectField $event $_;
			my $name = getName $f;
			my $ucname = uc $name;
			my $fucname = ucfirst $name;
			my $tos = $f->{codes} ?
				"f[$idx].val = edg_wll\_$type${fucname}ToString(e->$flctype.$name);" :
				toString $f "e->$flctype.$name","f[$idx].val";
			gen qq{
!			f[$idx].key = "$ucname";
!			$tos
};
			$idx++;
		}
		gen qq{
!			assert($idx<sizeof f/sizeof f[0]);
!			break;
};
	}
@@@}
		default:
			break;
	}

	for (i=0; i<sizeof(f)/sizeof(f[0]) && !err; i++) if (f[i].key && f[i].val) {
		trio_asprintf(&stmt,"insert into %s(jobid,event,name,value) "
			"values ('%|Ss',%d,'%|Ss','%|Ss')",
			strlen(f[i].val) <= SHORT_LEN ? "short_fields" : "long_fields",
			jobid,no,f[i].key,f[i].val);

		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) err = edg_wll_SetErrorDB(ctx);
		free(stmt);
	}
	for (i=0; i<sizeof(f)/sizeof(f[0]); i++) free(f[i].val);

/* XXX: hardcoded, no other suitable place to store it */
	if (!err) {
		trio_asprintf(&stmt,"insert into short_fields(jobid,event,name,value) "
			"values ('%|Ss',%d,'SRC_INSTANCE','%|Ss')",
			jobid,no,e->any.src_instance);
		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) err = edg_wll_SetErrorDB(ctx);
		free(stmt);
	}

	return err;
}

static int check_dup(edg_wll_Context ctx,edg_wll_Event *e)
{
	int		i,dup_detected = 0;
	int		err;
	char		*es,*es2;
	edg_wll_QueryRec	jc[2],ec[2];
	edg_wll_QueryRec        **jca, **eca;
	edg_wll_Event	*e2;

	edg_wll_ResetError(ctx);

	jc[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	jc[0].value.j = e->any.jobId;
	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	ec[0].attr = EDG_WLL_QUERY_ATTR_TIME;
	memcpy(&ec[0].value.t,&e->any.timestamp,sizeof(struct timeval));
	ec[0].op = EDG_WLL_QUERY_OP_EQUAL;
	ec[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

        jca = (edg_wll_QueryRec **) malloc (2 * sizeof(edg_wll_QueryRec **));
        eca = (edg_wll_QueryRec **) malloc (2 * sizeof(edg_wll_QueryRec **));
        jca[0] = jc;
        jca[1] = NULL;
        eca[0] = ec;
        eca[1] = NULL;

	err = edg_wll_QueryEventsServer(ctx,1,(const edg_wll_QueryRec **)jca, 
					(const edg_wll_QueryRec **)eca,&e2);
	switch (err) {
		case 0: /* continue normally */
			break;
		case ENOENT:
			free(jca);
			free(eca);
			return edg_wll_ResetError(ctx);
			break;
		default:
			free(jca);
			free(eca);
			return edg_wll_Error(ctx,NULL,NULL);
			break;
	}

	es = edg_wll_UnparseEvent(ctx,e);
	assert(es);

	for (i=0;e2[i].type && !dup_detected;i++) {
		/* Ignore priority */
		e2[i].any.priority = e->any.priority;
		es2 = edg_wll_UnparseEvent(ctx,e2+i);
		assert(es2);
		if (!strcmp(es,es2)) {
			dup_detected = 1;
			edg_wll_SetError(ctx,EEXIST,"duplicate event");
		}
		free(es2);
	}

	free(jca);
	free(eca);
	free(es);
	for (i=0; e2[i].type; i++) edg_wll_FreeEvent(e2+i);
	free(e2);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int check_auth(edg_wll_Context ctx,edg_wll_Event *e)
{
	char	*jobid = glite_lbu_JobIdGetUnique(e->any.jobId);
	char	*q = NULL,*owner = NULL;
	glite_lbu_Statement	stmt = NULL;
	char	*user;

	edg_wll_ResetError(ctx);

	if (e->type == EDG_WLL_EVENT_REGJOB) 
		return strcmp(e->any.user,EDG_WLL_LOG_USER_DEFAULT) ?
			0 : edg_wll_SetError(ctx,EPERM,"can't register jobs anonymously");

	trio_asprintf(&q,"select userid from jobs where jobid='%|Ss'",jobid);

	if (glite_lbu_ExecSQL(ctx->dbctx,q,&stmt) < 0
		|| glite_lbu_FetchRow(stmt,1,NULL,&owner) < 0
	) {
		edg_wll_SetErrorDB(ctx);
		goto clean;
	}

	if (!owner) {
		if ( ctx->isProxy && !e->any.seqcode )
			edg_wll_SetError(ctx, EINVAL, "Job not registered - sequence code needed");
		else
			/* We have to let the calling function know what happened here
			 * even if it hapens inside the LB Proxy which shouldn't consider
			 * this as an error
			 */
			edg_wll_SetError(ctx, ENOENT, "job not registered");
		goto clean;
	}

	switch (e->any.source) {
		case EDG_WLL_SOURCE_USER_INTERFACE:
		case EDG_WLL_SOURCE_LRMS:
		case EDG_WLL_SOURCE_APPLICATION:
			user = str2md5(e->any.user);
			if (strcmp(owner,user)) edg_wll_SetError(ctx,EPERM,"check_auth()");
			free(user);
			break;
		default:
			/* XXX: just don't allow anonymous */
			if (!strcmp(e->any.user,EDG_WLL_LOG_USER_DEFAULT))
				edg_wll_SetError(ctx,EPERM,"check_auth()");
			break;
	}


clean:
	if (stmt) glite_lbu_FreeStmt(&stmt);
	free(q);
	free(owner);
	return edg_wll_Error(ctx,NULL,NULL);
}

#ifndef LB_DAG_EMBRIONIC
static int register_subjobs(edg_wll_Context ctx,const edg_wll_RegJobEvent *e)
{
	int	i,err;
	glite_lbu_JobId	*subjobs;
	struct timeval	now;

	edg_wll_ResetError(ctx);
	if (e->nsubjobs == 0) return 0;
	if (e->nsubjobs < 0) return edg_wll_SetError(ctx,EINVAL,"negative number of subjobs");

	if ((err = edg_wll_GenerateSubjobIds(ctx,e->jobId,e->nsubjobs,e->seed,&subjobs)))
		return err;

	gettimeofday(&now,NULL);

/* XXX: increase the overall request timeout. */
	ctx->p_tmp_timeout.tv_sec += e->nsubjobs;
	if (ctx->p_tmp_timeout.tv_sec > 86400) ctx->p_tmp_timeout.tv_sec = 86400;

	for (i=0; i<e->nsubjobs; i++) {
		edg_wll_Event	e2;
		int		seq;
		char		*et,*ed,*job_s;

		memset(&e2,0,sizeof e2);
		e2.type = EDG_WLL_EVENT_REGJOB;
		e2.any.jobId = subjobs[i]; subjobs[i] = NULL;
		memcpy(&e2.regJob.timestamp,&now,sizeof now);
		e2.any.host = strdup(ctx->srvName);
		e2.any.level = e->level;
		e2.any.priority = e->priority;
		e2.any.seqcode = strdup(EDG_WLL_SEQ_NULL);
		e2.any.user = strdup(e->user);
		e2.any.source = e->source;
		e2.any.src_instance = strdup(ctx->isProxy ? 
			"L&B proxy" : "L&B server");
		e2.regJob.ns = strdup(e->ns);
		glite_lbu_JobIdDup(e->jobId,&e2.regJob.parent);
		e2.regJob.jobtype = EDG_WLL_REGJOB_SIMPLE;
		e2.regJob.jdl = strdup("");

		switch (edg_wll_StoreEvent(ctx,&e2,&seq)) {

			case 0: break;
			/* maybe some non-ignorable errors should be handled here */

			default: 
				edg_wll_Error(ctx,&et,&ed);
				job_s = glite_lbu_JobIdUnparse(e2.any.jobId);
				fprintf(stderr,"register subjob %s: %s (%s)\n",job_s,et,ed);
				syslog(LOG_ERR,"register subjob %s: %s (%s)",job_s,et,ed);
				free(job_s); free(et); free(ed);
				edg_wll_FreeEvent(&e2);
				edg_wll_ResetError(ctx);
				continue;
		}

		if (edg_wll_LockJob(ctx,e2.any.jobId)) {
			job_s = glite_lbu_JobIdUnparse(e2.any.jobId);
			fprintf(stderr,"lock job %s: %s (%s)\n",job_s,et,ed);
			syslog(LOG_ERR,"lock job %s: %s (%s)",job_s,et,ed);
			free(job_s); free(et); free(ed);
			edg_wll_FreeEvent(&e2);
			edg_wll_ResetError(ctx);
			continue;
		}

		if ((err = edg_wll_StepIntState(ctx,e2.any.jobId,&e2,seq,NULL)))
			edg_wll_Error(ctx,&et,&ed);

		edg_wll_UnlockJob(ctx,e2.any.jobId);
		edg_wll_ResetError(ctx);

		if (err) {
			job_s = glite_lbu_JobIdUnparse(e2.any.jobId);
			fprintf(stderr,"%s: %s (%s)\n",job_s,et,ed);
			syslog(LOG_ERR,"%s: %s (%s)",job_s,et,ed);
			free(job_s); free(et); free(ed);
			edg_wll_ResetError(ctx);
		}

		edg_wll_FreeEvent(&e2);
	}

	free(subjobs);
	return edg_wll_Error(ctx,NULL,NULL);
}
#endif


/*
 * Returns encoded SQL table states record for embryonic DAG subjob
 */

static edg_wll_ErrorCode states_values_embryonic(
	edg_wll_Context ctx,
	glite_lbu_JobId jobid,
	const edg_wll_RegJobEvent *e,
	char **icnames,
	char **values)
{
	char *jobid_md5, *stat_enc, *parent_md5;
	char *stmt = NULL;
	char *icvalues;
	intJobStat stat_rec;
	intJobStat *stat = &stat_rec;

	init_intJobStat(stat);
	if (glite_lbu_JobIdDup(jobid, &stat->pub.jobId) ||
		glite_lbu_JobIdDup(e->jobId, &stat->pub.parent_job)) goto err;
	stat->pub.state = EDG_WLL_JOB_SUBMITTED;
	stat->pub.owner = strdup(e->user);
	stat->pub.stateEnterTimes[1 + EDG_WLL_JOB_SUBMITTED] = (int)e->timestamp.tv_sec;

	jobid_md5 = glite_lbu_JobIdGetUnique(jobid);
	parent_md5 = glite_lbu_JobIdGetUnique(e->jobId);
	stat_enc = enc_intJobStat(strdup(""), stat);
	if (jobid_md5 == NULL || parent_md5 == NULL || stat_enc == NULL) goto err;


	if (edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, stat, 1, icnames, &icvalues)) goto err;
	trio_asprintf(&stmt,
		"'%|Ss',%d,%d,'%|Ss','%|Ss','%|Ss'%s",
		jobid_md5, stat->pub.state, 1, stat_enc,
		INTSTAT_VERSION, parent_md5, icvalues);
	free(icvalues);

err:
	destroy_intJobStat(stat);
	free(jobid_md5);
	free(stat_enc);
	free(parent_md5);
	*values = stmt;
	return edg_wll_Error(ctx,NULL,NULL);
}

static int register_subjobs_embryonic(edg_wll_Context ctx,const edg_wll_RegJobEvent *e)
{
	int			i, err = 0;
	glite_lbu_JobId		*subjobs;
	struct timeval		now;
	char			*userid = str2md5(e->user);
	char			*jobid_md5, *jobid_md5_old;
	size_t			jobid_len;
	char			*icnames, *values;
	glite_lbu_bufInsert     bi_states;
#ifdef LB_BUF
	char                   *states_cols;
	glite_lbu_bufInsert	bi_jobs;
#endif

	edg_wll_ResetError(ctx);

	if (e->nsubjobs == 0) return 0;
	if (e->nsubjobs < 0) return edg_wll_SetError(ctx,EINVAL,"negative number of subjobs");
	if ((err = edg_wll_GenerateSubjobIds(ctx,e->jobId,e->nsubjobs,e->seed,&subjobs)))
		return err;

	/* find out icnames and values once, then only change jobids */ 
	if (states_values_embryonic(ctx, subjobs[0], e, &icnames, &values))
		edg_wll_Error(ctx, NULL, NULL);
	jobid_md5_old = glite_lbu_JobIdGetUnique(subjobs[0]);
	jobid_len = strlen(jobid_md5_old);
		

#ifdef LB_BUF
	/* init multirows insert mechanism for tables used here */
	if (glite_lbu_bufferedInsertInit(ctx->dbctx, &bi_jobs, NULL, "jobs", 4000, 1000,
		"jobid, dg_jobid, userid"))
	{
		return edg_wll_SetErrorDB(ctx);
	}

	asprintf(&states_cols,"jobid, status, seq,int_status, version, parent_job%s", icnames);
	if (glite_lbu_bufferedInsertInit(ctx->dbctx, &bi_states, NULL, "states", 4000, 1000, states_cols))
	{
                return edg_wll_SetErrorDB(ctx);
	}
	free(states_cols);
#endif		


	gettimeofday(&now,NULL);

	/* increase the overall request timeout. */
	ctx->p_tmp_timeout.tv_sec += e->nsubjobs/10;
	if (ctx->p_tmp_timeout.tv_sec > 86400) ctx->p_tmp_timeout.tv_sec = 86400;

	for (i=0; i<e->nsubjobs; i++) {
		char		*et,*ed,*job_s,*p,*p1;

		/* save jobid-userid relation into jobs table */
#ifdef LB_BUF
	        if ((err = store_job_block(ctx, subjobs[i], userid, bi_jobs)))
#else
	        if ((err = store_job(ctx, subjobs[i], userid)))
#endif
			edg_wll_Error(ctx,&et,&ed);

		/* interchange variable parts (jobids) in values */
		/* there are only two occurences of subjob jobid */
		jobid_md5 = glite_lbu_JobIdGetUnique(subjobs[i]);
		if (i) {
			p = strstr(values, jobid_md5_old);		
			assert(p);
			memcpy(p, jobid_md5, jobid_len);

			p1 = strstr(p + jobid_len, jobid_md5_old);
			assert(p1);
			memcpy(p1, jobid_md5, jobid_len);
		}
		free(jobid_md5_old);
		jobid_md5_old = jobid_md5;

		if (!err && (err = edg_wll_StoreIntStateEmbryonic(ctx, subjobs[i], icnames, values, bi_states)))
			edg_wll_Error(ctx,&et,&ed);

//job_s = glite_lbu_JobIdUnparse(subjobs[i]);
//printf("%s\n", job_s);
//free(job_s);

		if (err) {
			job_s = glite_lbu_JobIdUnparse(subjobs[i]);
			fprintf(stderr,"%s: %s (%s)\n",job_s,et,ed);
			syslog(LOG_ERR,"%s: %s (%s)",job_s,et,ed);
			free(job_s); free(et); free(ed);
			edg_wll_ResetError(ctx);
		}
		glite_lbu_JobIdFree(subjobs[i]);
	}

	free(jobid_md5_old);	//free the last one
	free(icnames);
	free(values);
	free(subjobs);

#ifdef LB_BUF
	/* commit the rest of multirows insert and clean structures */
	glite_lbu_bufferedInsertClose(bi_jobs);
	glite_lbu_bufferedInsertClose(bi_states);
#endif

	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_delete_event(edg_wll_Context ctx,const char *jobid,int event)
{
	char	*stmt;

/* The order of tables is important to prevent another process calling
 * StoreEvent() to get our event number and mess up the fields together.
 *
 * XXX: best effort: more or less ignore errors
 *
 */

	trio_asprintf(&stmt,
		"delete from short_fields where jobid='%|Ss' and event=%d",
		jobid,event);
	glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from long_fields where jobid='%|Ss' and event=%d",
		jobid,event);
	glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from events where jobid='%|Ss' and event=%d",
		jobid,event);
	glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL);
	free (stmt);

	return edg_wll_SetErrorDB(ctx);
}
