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

#include "glite/wmsutils/jobid/strmd5.h"

#include "glite/lb/events_parse.h"
#include "glite/lb/context-int.h"
#include "glite/lb/log_proto.h"	/* for EDG_WLL_LOG_USER_DEFAULT */
#include "glite/lb/trio.h"

#include "store.h"
#include "get_events.h"
#include "lbs_db.h"
#include "lock.h"
#include "lb_authz.h"
#include "jobstat.h"

static int store_user(edg_wll_Context,const char *,const char *); 
static int store_job(edg_wll_Context,const edg_wlc_JobId,const char *);
#ifdef LB_BUF
static int store_job_block(edg_wll_Context, const edg_wlc_JobId, const char *, edg_wll_bufInsert *);
#endif
static int store_job_grey(edg_wll_Context,const edg_wlc_JobId,time_t);
static int store_flesh(edg_wll_Context,edg_wll_Event *,char *,int);
static int store_seq(edg_wll_Context,edg_wll_Event *,int);
static int check_dup(edg_wll_Context,edg_wll_Event *);
static int check_auth(edg_wll_Context,edg_wll_Event *e); 
#ifndef LB_DAG_EMBRIONIC
static int register_subjobs(edg_wll_Context,const edg_wll_RegJobEvent *);
#endif
static int register_subjobs_embryonic(edg_wll_Context,const edg_wll_RegJobEvent *);

void edg_wll_StoreAnonymous(edg_wll_Context ctx,int anon) {
	ctx->allowAnonymous = anon;
}

int edg_wll_StoreEvent(edg_wll_Context ctx,edg_wll_Event *e,int *seq)
{
	edg_wll_ErrorCode	err = 0;
	char		*userid = NULL,*jobid,*stmt;
	char		*select_max,*ssrc;
	edg_wll_Stmt	sh = NULL;
	int		next = 0xDEAD;
	int		lbproxy_notreg = 0;
	char		*now_s = NULL;

	ssrc = jobid = stmt = select_max = NULL;

	if ( ctx->event_load )
		now_s = strdup(edg_wll_TimeToDB(e->any.arrived.tv_sec));
	else
		now_s = strdup(edg_wll_TimeToDB(time(NULL)));

	edg_wll_ResetError(ctx);
	switch (err = check_auth(ctx,e)) {
		case 0: break;
		case ENOENT: 
			if ( !ctx->isProxy ) {
				if (ctx->greyjobs) {
					edg_wll_ResetError(ctx);
					if (store_job_grey(ctx,e->any.jobId,e->any.timestamp.tv_sec))
						goto clean;
					break;
				} 
		       		else goto clean;
			}

			edg_wll_ResetError(ctx);
			lbproxy_notreg = 1;
			break;
		case EPERM:
			if (!ctx->noAuth) goto clean;
			edg_wll_ResetError(ctx);
			break;
		default: goto clean;
	}

/* FIXME: does not work for grey jobs due to "select from jobs" -- I don't care for the time being */
	if ((err = check_dup(ctx,e))) goto clean;

	userid = strdup(strmd5(e->any.user,NULL));

	if ((e->type == EDG_WLL_EVENT_REGJOB || lbproxy_notreg)) {
		/* Register the job and owner. For LBproxy, contant "lbproxy"
                   is used as the name - it's harmless as the job is already
		   registered with LBserver */
		char *username;

		username = (ctx->isProxy) ? "lbproxy" : ctx->peerName;
		userid = strdup(strmd5(username, NULL));
		if ((err = store_user(ctx,userid, username))) goto clean;
		if ((err = store_job(ctx,e->any.jobId,userid))) goto clean;
	} else {
		/* for other events just make sure user record is there */
		userid = strdup(strmd5(e->any.user,NULL));
		if ((err = store_user(ctx,userid,e->any.user))) goto clean;
	}

	jobid = edg_wlc_JobIdGetUnique(e->any.jobId);

/* obtain next event sequence number */
	trio_asprintf(&select_max,
		"select max(event) from events "
		"where jobid = '%|Ss'",jobid);

	ssrc = edg_wll_SourceToString(e->any.source);

/* try to insert (someone else may be doing the same) */
	while (1) {
		char	*max;

		if (edg_wll_ExecStmt(ctx,select_max,&sh) < 0 ||
		    edg_wll_FetchRow(sh,&max) < 0)
		{
			err = edg_wll_Error(ctx,NULL,NULL);
			goto clean;
		}
		edg_wll_FreeStmt(&sh); 
		
		next = max && *max ? atoi(max)+1 : 0;
		
	/* 
	 * 1) when using transactions:
	 *   Store the whole event right now.
	 *
	 * 2) when not using transaction:
	 *   Store an UNDEF event first in order to prevent race condition
	 *   with readers and update event code later.
	 */
		trio_asprintf(&stmt,
			"insert into events(jobid,event,code,prog,host,time_stamp,usec,arrived,level,userid) "
			"values ('%|Ss',%d,%d,'%|Ss','%|Ss',%s,%d,%s,%d,'%|Ss')",
			jobid,next,
			ctx->use_transactions ? (int) e->any.type : EDG_WLL_EVENT_UNDEF,
			ssrc,e->any.host,
			edg_wll_TimeToDB(e->any.timestamp.tv_sec),e->any.timestamp.tv_usec,
			now_s, e->any.level,userid);

		if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) {
			if ((err = edg_wll_Error(ctx,NULL,NULL)) != EEXIST)
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
	if (!ctx->use_transactions) {
	/* emulate commit, i.e. swith to a real event type to make
	 * the record valid */
		trio_asprintf(&stmt,
			"update events set code=%d "
			"where jobid='%|Ss' and event=%d",
			(int) e->any.type,jobid,next);
		switch (edg_wll_ExecStmt(ctx,stmt,NULL)) {
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
					switch (edg_wll_ExecStmt(ctx,stmt,NULL)) {
						case 1: break;
						case 0: /* purge in progres */
							edg_wll_delete_event(ctx,jobid,next);
							err = edg_wll_SetError(ctx,ENOENT,"job being purged");
							break;
						default: err = edg_wll_SetError(ctx,EDG_WLL_ERROR_DB_CALL,
								"more job records, what is that?");
							break;
					}
				}
				break;
			case -1: err = edg_wll_Error(ctx,NULL,NULL);
				break;

			default: err = edg_wll_SetError(ctx,EDG_WLL_ERROR_DB_CALL,
				"more event records, what is that?");
				break;
		}
	} /* if !ctx->use_transactions */

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
	if (sh) edg_wll_FreeStmt(&sh);
	if (!err && seq) *seq = next;
	return err;
}

static int store_user(edg_wll_Context ctx,const char *userid,const char *subj)
{
	char	*stmt;

	trio_asprintf(&stmt,"insert into users(userid,cert_subj) "
		"values ('%|Ss','%|Ss')",userid,subj);

	if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) {
		if (edg_wll_Error(ctx,NULL,NULL) == EEXIST)
			edg_wll_ResetError(ctx);
	}

	free(stmt);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int store_job(edg_wll_Context ctx,const edg_wlc_JobId job,const char *userid)
{
	char *jobstr = edg_wlc_JobIdUnparse(job);
	char *jobid = edg_wlc_JobIdGetUnique(job);
	char *stmt;

/* debug Duplicate key on index: Duplicate entry '(nil)' for key 1
 */
	if (jobid == NULL || jobstr == NULL) 
		return edg_wll_SetError(ctx,EINVAL,"store_job()");

	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"insert into jobs(jobid,dg_jobid,userid) "
		"values ('%|Ss','%|Ss','%|Ss')",jobid,jobstr,userid);

	if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) {
		if (edg_wll_Error(ctx,NULL,NULL) == EEXIST) 
			edg_wll_ResetError(ctx);
	}
	free(stmt);

	if (ctx->greyjobs) {
		trio_asprintf(&stmt,"delete from grey_jobs where jobid = '%|Ss'",jobid);
		edg_wll_ExecStmt(ctx,stmt,NULL); /* XXX: error propagates */
		free(stmt);
	}

	free(jobstr);
	free(jobid);
	return edg_wll_Error(ctx,NULL,NULL);
}

#ifdef LB_BUF
static int store_job_block(edg_wll_Context ctx,const edg_wlc_JobId job,const char *userid, edg_wll_bufInsert *bi)
{
	char *jobstr = edg_wlc_JobIdUnparse(job);
	char *jobid = edg_wlc_JobIdGetUnique(job);
	char *row;

/* debug Duplicate key on index: Duplicate entry '(nil)' for key 1
 */

	assert(!ctx->greyjobs);	/* XXX: should not happen */

	if (jobid == NULL || jobstr == NULL) 
		return edg_wll_SetError(ctx,EINVAL,"store_jobi_block()");

	edg_wll_ResetError(ctx);

	trio_asprintf(&row, "'%|Ss','%|Ss','%|Ss'", jobid,jobstr,userid);
	edg_wll_bufferedInsert(bi, row); // no need to free row

	free(jobstr);
	free(jobid);
	return edg_wll_Error(ctx,NULL,NULL);
}
#endif

static int store_job_grey(edg_wll_Context ctx,const edg_wlc_JobId job,time_t etime)
{
	char *jobstr = edg_wlc_JobIdUnparse(job);
	char *jobid = edg_wlc_JobIdGetUnique(job);
	char *stmt;

	if (jobid == NULL || jobstr == NULL)
		return edg_wll_SetError(ctx,EINVAL,"store_job_grey()");

	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"insert into grey_jobs(jobid,dg_jobid,time_stamp) "
			"values ('%|Ss','%|Ss',%s)",
			jobid,jobstr,edg_wll_TimeToDB(etime));

	if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) {
		if (edg_wll_Error(ctx,NULL,NULL) == EEXIST)
			edg_wll_ResetError(ctx);
	}

	free(stmt);
	free(jobstr);
	free(jobid);
	return edg_wll_Error(ctx,NULL,NULL);
}

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
	char	*jobid = edg_wlc_JobIdGetUnique(e->any.jobId);

	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"insert into short_fields(jobid,event,name,value) "
			"values ('%|Ss',%d,'SEQCODE','%|Ss')",
			jobid,no,e->any.seqcode);

	ret = edg_wll_ExecStmt(ctx,stmt,NULL);
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
		if ($flctype =~ m/^pBS/) { $flctype = ucfirst $flctype; }
		if ($flctype =~ m/^condor/) { $flctype = ucfirst $flctype; }
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

		if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) err = edg_wll_Error(ctx,NULL,NULL);
		free(stmt);
	}
	for (i=0; i<sizeof(f)/sizeof(f[0]); i++) free(f[i].val);

/* XXX: hardcoded, no other suitable place to store it */
	if (!err) {
		trio_asprintf(&stmt,"insert into short_fields(jobid,event,name,value) "
			"values ('%|Ss',%d,'SRC_INSTANCE','%|Ss')",
			jobid,no,e->any.src_instance);
		if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) err = edg_wll_Error(ctx,NULL,NULL);
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
	char	*jobid = edg_wlc_JobIdGetUnique(e->any.jobId);
	char	*q = NULL,*owner = NULL;
	edg_wll_Stmt	stmt = NULL;
	char	*user;

	edg_wll_ResetError(ctx);

	if (e->type == EDG_WLL_EVENT_REGJOB) 
		return strcmp(e->any.user,EDG_WLL_LOG_USER_DEFAULT) ?
			0 : edg_wll_SetError(ctx,EPERM,"can't register jobs anonymously");

	trio_asprintf(&q,"select userid from jobs where jobid='%|Ss'",jobid);

	if (edg_wll_ExecStmt(ctx,q,&stmt) < 0
		|| edg_wll_FetchRow(stmt,&owner) < 0
	) goto clean;

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
			user = strmd5(e->any.user,NULL);
			if (strcmp(owner,user)) edg_wll_SetError(ctx,EPERM,"check_auth()");
			break;
		default:
			/* XXX: just don't allow anonymous */
			if (!strcmp(e->any.user,EDG_WLL_LOG_USER_DEFAULT))
				edg_wll_SetError(ctx,EPERM,"check_auth()");
			break;
	}


clean:
	if (stmt) edg_wll_FreeStmt(&stmt);
	free(q);
	free(owner);
	return edg_wll_Error(ctx,NULL,NULL);
}

#ifndef LB_DAG_EMBRIONIC
static int register_subjobs(edg_wll_Context ctx,const edg_wll_RegJobEvent *e)
{
	int	i,err;
	edg_wlc_JobId	*subjobs;
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
		edg_wlc_JobIdDup(e->jobId,&e2.regJob.parent);
		e2.regJob.jobtype = EDG_WLL_REGJOB_SIMPLE;
		e2.regJob.jdl = strdup("");

		switch (edg_wll_StoreEvent(ctx,&e2,&seq)) {

			case 0: break;
			/* maybe some non-ignorable errors should be handled here */

			default: 
				edg_wll_Error(ctx,&et,&ed);
				job_s = edg_wlc_JobIdUnparse(e2.any.jobId);
				fprintf(stderr,"register subjob %s: %s (%s)\n",job_s,et,ed);
				syslog(LOG_ERR,"register subjob %s: %s (%s)",job_s,et,ed);
				free(job_s); free(et); free(ed);
				edg_wll_FreeEvent(&e2);
				edg_wll_ResetError(ctx);
				continue;
		}

		if (edg_wll_LockJob(ctx,e2.any.jobId)) {
			job_s = edg_wlc_JobIdUnparse(e2.any.jobId);
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
			job_s = edg_wlc_JobIdUnparse(e2.any.jobId);
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
	edg_wlc_JobId jobid,
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
	if (edg_wlc_JobIdDup(jobid, &stat->pub.jobId) ||
		edg_wlc_JobIdDup(e->jobId, &stat->pub.parent_job)) goto err;
	stat->pub.state = EDG_WLL_JOB_SUBMITTED;
	stat->pub.owner = strdup(e->user);
	stat->pub.jobtype = EDG_WLL_STAT_SIMPLE;
	stat->pub.stateEnterTimes[1 + EDG_WLL_JOB_SUBMITTED] = (int)e->timestamp.tv_sec;

	jobid_md5 = edg_wlc_JobIdGetUnique(jobid);
	parent_md5 = edg_wlc_JobIdGetUnique(e->jobId);
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
	edg_wlc_JobId		*subjobs;
	struct timeval		now;
	char			*userid = strdup(strmd5(e->user,NULL));
	char			*jobid_md5, *jobid_md5_old;
	size_t			jobid_len;
#ifdef LB_BUF
	edg_wll_bufInsert	bi_j;
	edg_wll_bufInsert	*bi_jobs = &bi_j;
       	char			*states_cols;
#endif
	edg_wll_bufInsert	bi_s, *bi_states = &bi_s;
	char			*icnames, *values;


	edg_wll_ResetError(ctx);

	if (e->nsubjobs == 0) return 0;
	if (e->nsubjobs < 0) return edg_wll_SetError(ctx,EINVAL,"negative number of subjobs");
	if ((err = edg_wll_GenerateSubjobIds(ctx,e->jobId,e->nsubjobs,e->seed,&subjobs)))
		return err;

	/* find out icnames and values once, then only change jobids */ 
	if (states_values_embryonic(ctx, subjobs[0], e, &icnames, &values))
		edg_wll_Error(ctx, NULL, NULL);
	jobid_md5_old = edg_wlc_JobIdGetUnique(subjobs[0]);
	jobid_len = strlen(jobid_md5_old);
		

#ifdef LB_BUF
	/* init multirows insert mechanism for tables used here */
	if (edg_wll_bufferedInsertInit(ctx, bi_jobs, NULL, "jobs", 4000, 1000,
		"jobid, dg_jobid, userid"))
	{
		return edg_wll_SetError(ctx, EINVAL, "edg_wll_bufferedInsertInit()");
	}

	asprintf(&states_cols,"jobid, status, seq,int_status, version, parent_job%s", icnames);
	if (edg_wll_bufferedInsertInit(ctx, bi_states, NULL, "states", 4000, 1000, states_cols))
	{
                return edg_wll_SetError(ctx, EINVAL, "edg_wll_bufferedInsertInit()");
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
		jobid_md5 = edg_wlc_JobIdGetUnique(subjobs[i]);
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

//job_s = edg_wlc_JobIdUnparse(subjobs[i]);
//printf("%s\n", job_s);
//free(job_s);

		if (err) {
			job_s = edg_wlc_JobIdUnparse(subjobs[i]);
			fprintf(stderr,"%s: %s (%s)\n",job_s,et,ed);
			syslog(LOG_ERR,"%s: %s (%s)",job_s,et,ed);
			free(job_s); free(et); free(ed);
			edg_wll_ResetError(ctx);
		}
		edg_wlc_JobIdFree(subjobs[i]);
	}

	free(jobid_md5_old);	//free the last one
	free(icnames);
	free(values);
	free(subjobs);

#ifdef LB_BUF
	/* commit the rest of multirows insert and clean structures */
	edg_wll_bufferedInsertClose(bi_jobs);
	edg_wll_bufferedInsertClose(bi_states);
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
	edg_wll_ExecStmt(ctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from long_fields where jobid='%|Ss' and event=%d",
		jobid,event);
	edg_wll_ExecStmt(ctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from events where jobid='%|Ss' and event=%d",
		jobid,event);
	edg_wll_ExecStmt(ctx,stmt,NULL);
	free (stmt);

	return edg_wll_Error(ctx,NULL,NULL);
}
