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
#include <ctype.h>

#include "glite/jobid/strmd5.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"

#include "glite/lb/events_parse.h"
#include "glite/lb/context-int.h"
#include "glite/lb/log_proto.h"	/* for EDG_WLL_LOG_USER_DEFAULT */
#include "glite/lb/intjobstat_supp.h"

#include "store.h"
#include "get_events.h"
#include "lb_authz.h"
#include "authz_policy.h"
#include "jobstat.h"
#include "db_calls.h"
#include "db_supp.h"
#include "index.h"

static int store_user(edg_wll_Context,const char *,const char *); 
static int store_job(edg_wll_Context,glite_jobid_const_t,const char *, int, int, int, int);
static int store_flesh(edg_wll_Context,edg_wll_Event *,const char *ulm, char *,int);
/* FIXME: 'check_dup' defined but not used */
static int check_dup(edg_wll_Context,edg_wll_Event *);
static int check_auth(edg_wll_Context,edg_wll_Event *e); 
static void lowercase_usertag(edg_wll_Event *ev);

void edg_wll_StoreAnonymous(edg_wll_Context ctx,int anon) {
	ctx->allowAnonymous = anon;
}


/* !!! to be called from OPEN TRANSACTION only !!! 
 */
int edg_wll_StoreEvent(edg_wll_Context ctx,edg_wll_Event *e,const char *ulm,int *seq)
{
	char			*userid, *jobid, *stmt, *ssrc, *now_s, *stamp, *dummy, *max;
	glite_lbu_Statement	sh = NULL;
	int			next = 0xDEAD, nr;


	userid = ssrc = jobid = stmt = now_s = stamp = dummy = max = NULL;

	lowercase_usertag(e);
	jobid = edg_wlc_JobIdGetUnique(e->any.jobId);
	glite_lbu_TimeToStr(e->any.timestamp.tv_sec, &stamp);
	ssrc = edg_wll_SourceToString(e->any.source);

	if ( ctx->event_load )
		glite_lbu_TimeToStr(e->any.arrived.tv_sec, &now_s);
	else
		glite_lbu_TimeToStr(time(NULL), &now_s);

	edg_wll_ResetError(ctx);
	switch (check_auth(ctx,e)) {
		case 0: break;
		case ENOENT: 
			/* job not registered */
			// should not happen, store_job_server_proxy() miscoded or going thu load?
			goto clean;
			break;
		case EPERM:
			if (!ctx->noAuth) goto clean;
			edg_wll_ResetError(ctx);
			break;
		default: goto clean;
	}

	trio_asprintf(&stmt,"select userid from jobs where jobid='%|Ss'", jobid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	
	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0 || edg_wll_FetchRow(ctx,sh,1,NULL,&userid) < 0) goto clean;
	if (sh) glite_lbu_FreeStmt(&sh);
	free(stmt); stmt = NULL;


/* check duplicity */
	trio_asprintf(&stmt,
		"select arrived from events where jobid='%|Ss' and code='%d'"
		" and prog='%|Ss' and host='%|Ss' and time_stamp=%s and usec='%d'"
		" and level='%d' and userid='%|Ss' and seqcode='%|Ss'",
		jobid, (int) e->any.type,
		ssrc,e->any.host,
		stamp,e->any.timestamp.tv_usec,
		e->any.level,userid, e->any.seqcode);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0) goto clean;
	nr = edg_wll_FetchRow(ctx,sh,1,NULL,&dummy);
	if (sh) glite_lbu_FreeStmt(&sh);
	free(stmt); stmt = NULL;
	free(dummy);

	if (nr < 0) goto clean;
	if (nr > 0) {
		/* possible duplicity (99%) */
		// XXX: check event flesh to be 100% sure
		edg_wll_SetError(ctx,EEXIST,"duplicate event");
		goto clean;
	}
	/* else (nr == 0) -> unique event, continue */
	

/* obtain number of stored events */
	trio_asprintf(&stmt,
		"select nevents from jobs "
		"where jobid = '%|Ss'",jobid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0 ||
	    edg_wll_FetchRow(ctx,sh,1,NULL,&max) < 0) goto clean;
	glite_lbu_FreeStmt(&sh); 
	free(stmt);
	
	next = (max && *max) ? atoi(max) : 0;
	free(max);


/* store event */		
	trio_asprintf(&stmt,
		"insert into events(jobid,event,code,prog,host,time_stamp,usec,arrived,level,userid,seqcode) "
		"values ('%|Ss',%d,%d,'%|Ss','%|Ss',%s,%d,%s,%d,'%|Ss','%|Ss')",
		jobid,next,
		(int) e->any.type,
		ssrc,e->any.host,
		stamp,e->any.timestamp.tv_usec,
		now_s, e->any.level,userid, e->any.seqcode);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto clean;
	free(stmt); stmt = NULL;

/* increase number of stored events */
	trio_asprintf(&stmt,
		"update jobs set nevents='%d'"
		"where jobid = '%|Ss'", next+1, jobid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto clean;
	free(stmt); stmt = NULL;
	

/* store event record */
	if (store_flesh(ctx,e,ulm,jobid,next)) goto clean;

clean:
	free(now_s);
	free(userid);
	free(jobid);
	free(stmt);
	free(ssrc);
	if (sh) glite_lbu_FreeStmt(&sh);
	if (!edg_wll_Error(ctx,NULL,NULL) && seq) *seq = next;
	free(stamp);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int store_user(edg_wll_Context ctx,const char *userid,const char *subj)
{
	char	*stmt;

	trio_asprintf(&stmt,"insert into users(userid,cert_subj) "
		"values ('%|Ss','%|Ss')",userid,subj);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) {
		if (edg_wll_Error(ctx,NULL,NULL) == EEXIST)
			edg_wll_ResetError(ctx);
	}

	free(stmt);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int store_job(edg_wll_Context ctx,glite_jobid_const_t job,const char *userid, int proxy, int server,int grey, int update)
{
	char *jobstr = edg_wlc_JobIdUnparse(job);
	char *jobid = edg_wlc_JobIdGetUnique(job);
	char *stmt;
	char *srvName;
	unsigned int	srvPort;

/* debug Duplicate key on index: Duplicate entry '(nil)' for key 1
 */
	if (jobid == NULL || jobstr == NULL) 
		return edg_wll_SetError(ctx,EINVAL,"store_job()");

	edg_wll_ResetError(ctx);

	if (ctx->isProxy) {
		proxy = 1;

		/* If host&port in jobId match bkserver hostname and port and bkserver
		 * runs server service, mark the proxy job to belong to bkserver too	
		 */
		edg_wlc_JobIdGetServerParts(job, &srvName, &srvPort);
		if ( (ctx->serverRunning) && (ctx->srvPort == srvPort) && 
				!strcmp(ctx->srvName, srvName)) {
			server=1;
		}
	}
	else {
		server = 1;
	}

	if (update) {
		trio_asprintf(&stmt,"update jobs set userid='%|Ss', proxy='%|Sd', server='%|Sd', grey='%|Sd' where jobid='%|Ss'", 
			userid,proxy,server,grey,jobid);
	}
	else {
		trio_asprintf(&stmt,"insert into jobs(jobid,dg_jobid,userid,proxy,server,grey) "
			"values ('%|Ss','%|Ss','%|Ss', '%|Sd', '%|Sd', '%|Sd')",jobid,jobstr,userid,proxy,server,grey);
	}
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) {
		if (edg_wll_Error(ctx,NULL,NULL) == EEXIST && !update) 
			edg_wll_ResetError(ctx);
		else
			goto err;
	}
	free(stmt); stmt = NULL;

err:
	free(stmt);
	free(jobstr);
	free(jobid);
	return edg_wll_Error(ctx,NULL,NULL);
}


/* test whether job shares LB proxy and server DB or not */
int is_job_local(edg_wll_Context ctx, glite_jobid_const_t jobId) 
{
	char		*srvName = NULL;
	unsigned int	srvPort; 
	int		ret;	

	if (!ctx->serverRunning) return 0;	/* XXX */

	edg_wlc_JobIdGetServerParts(jobId, &srvName, &srvPort);
	ret = ((ctx->srvPort == srvPort) && ctx->srvName && !strcmp(srvName,ctx->srvName));
	free(srvName);

	return(ret);
}

int store_job_server_proxy(edg_wll_Context ctx, edg_wll_Event *event, int *register_to_JP)
{
	char 		*unique = edg_wlc_JobIdGetUnique(event->any.jobId);
	char            *q = NULL, *userid = NULL, *subj = NULL, *owner = NULL;
	glite_lbu_Statement    stmt = NULL, stmt_zomb = NULL;
	int             nar, grey = 0, async = 0;
	char		*can_peername = NULL;
	int             local_job = is_job_local(ctx, event->any.jobId);
	char 		*res[3] = {NULL, NULL, NULL};
	

	/* check auth */
	if (!ctx->isProxy && !ctx->peerName)
		return edg_wll_SetError(ctx,EPERM,"LB server can't store using unauthenticated connection");
	if (ctx->isProxy && (!event->any.user || !strcmp(event->any.user,EDG_WLL_LOG_USER_DEFAULT)) )
		return edg_wll_SetError(ctx,EPERM,"LB proxy can't store using unauthenticated connection");


	trio_asprintf(&q,"select proxy,server,grey from jobs where jobid='%|Ss' for update", unique);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);

	nar = edg_wll_ExecSQL(ctx,q,&stmt);
	free(q); q = NULL;

/* XXX: greyjobs semantics is overloaded here:
 * we trust the registration event content to specify job owner unconditionally.
 * It's fine for the time being but should be solved with new authz.
 * */

	if (nar < 0) goto err;
	else if (nar == 0) {
		/* Job not stored yet */
		if (event->any.type == EDG_WLL_EVENT_REGJOB) {

		/* XXX: directness is checked by any.user == peerName. Not perfect but better than event flags. */

			if (!ctx->isProxy && !edg_wll_gss_equal_subj(event->any.user, ctx->peerName) && !ctx->greyjobs) {
				if(ctx->async_registrations) {
					/* check authorization to register job as different user */
					if(check_authz_policy_ctx(ctx, REGISTER_FOREIGN)) {
						edg_wll_SetError(ctx, EPERM, "not allowed to register jobs as different user");
						goto err;
					}
				} else {
					edg_wll_SetError(ctx,EPERM,"job registration must go directly");
					goto err;
				}
			}
			if ((event->any.priority & EDG_WLL_LOGLFLAG_EXCL) && ctx->exclusive_zombies) {
				trio_asprintf(&q,"select jobid from zombie_jobs "
					"where zombie_jobs.jobid='%|Ss'",unique);

				if (edg_wll_ExecSQL(ctx,q,&stmt_zomb) > 0) {
					edg_wll_SetError(ctx,EEXIST,"Zombie job with given jobid already exists. Rejecting event.");
					goto err;
				}
				if (stmt_zomb) { glite_lbu_FreeStmt(&stmt_zomb); stmt_zomb = NULL; }
				free (q); q = NULL;
			}

			/* else OK */
		}
		else {
			if (ctx->greyjobs) grey = 1;
			else if(ctx->async_registrations) { 
				/* TODO: possibly add another permission */
				if(check_authz_policy_ctx(ctx, REGISTER_FOREIGN)) {
					edg_wll_SetError(ctx, EPERM, "not allowed to log events to unregistered job");
					goto err;
				}
				async = 1;
			}
			else {
				edg_wll_SetError(ctx, ENOENT, "job not registered");
				goto err;
			}
		}

		/* first event is not registration:
		 *   - when grey jobs are allowed, job is owned by "GREY JOB"
		 *   - when asynchronous registrations are allowed, job is owned by the logging component
		 * when none of the above applies, we should not get here at all.
		 */
		can_peername = grey ?  strdup("GREY JOB") : ( async ? edg_wll_gss_normalize_subj(ctx->peerName, 0) : edg_wll_gss_normalize_subj(event->any.user, 0) );
		userid = strdup(strmd5(can_peername, NULL));
		if (store_user(ctx,userid,can_peername)) goto err;
		/* store job as unregistered (grey) when either grey jobs or async registrations are allowed */
		if (store_job(ctx,(glite_jobid_const_t) event->any.jobId,
			userid, ctx->isProxy, local_job, grey | async, 0 )) goto err;
		if (event->any.type == EDG_WLL_EVENT_REGJOB) {
			if(event->regJob.wms_dn) {
				char *names = strdup(event->regJob.wms_dn);
				char *p, *name;
				int ret;
				
				name = names;
				while ((p = strchr(name, '\n'))) {
					*p = '\0';
					ret = edg_wll_UpdateACL(ctx, event->any.jobId, name,
								EDG_WLL_CHANGEACL_DN,
								EDG_WLL_CHANGEACL_READ,
								EDG_WLL_CHANGEACL_ALLOW,
								EDG_WLL_CHANGEACL_ADD);
					if (ret) { 
						free(names);
						goto err;
					}
					name = p+1;
				}
				free(names);
			} else if(async) {
				int ret;
				
				/* allow logging component to modify job */
				ret = edg_wll_UpdateACL(ctx, event->any.jobId, 
							strdup(edg_wll_gss_normalize_subj(ctx->peerName, 0)),
							EDG_WLL_CHANGEACL_DN,
							EDG_WLL_CHANGEACL_READ,
							EDG_WLL_CHANGEACL_ALLOW,
							EDG_WLL_CHANGEACL_ADD);
				if (ret) { 
					goto err;
				}
			}
		}

		*register_to_JP = (local_job) ? REG_JOB_TO_JP : 0;
	}
	else {
		/* Job already stored */

		if (edg_wll_FetchRow(ctx,stmt,sizeof(res)/sizeof(res[0]),NULL,res) < 0) goto err;
		if (stmt) { glite_lbu_FreeStmt(&stmt); stmt = NULL; }


		if ((ctx->greyjobs || ctx->async_registrations) && !strcmp(res[2],"1") && 
			(event->any.type == EDG_WLL_EVENT_REGJOB)) 
		{
			/* late registration event for already existing job */

			/* check permission to change owner if not doing greyjobs (ie. in async registration) */
			if(!ctx->greyjobs) {
				
			}

			/* set proper job owner */
			can_peername = edg_wll_gss_normalize_subj(event->any.user, 0);
			userid = strdup(strmd5(can_peername, NULL));
			if (store_user(ctx,userid,can_peername)) goto err;
			if (store_job(ctx,(glite_jobid_const_t) event->any.jobId,
				userid, (ctx->isProxy || !strcmp(res[0],"1")), 
				!strcmp(res[1],"1") || (local_job ? ctx->serverRunning : 0), 0, 1)) goto err;
		}
		else {
			/* check possible server vs. proxy registration ownership clash */
			if (event->any.type == EDG_WLL_EVENT_REGJOB) 
			{
				trio_asprintf(&q,"select u.cert_subj from jobs j, users u "
							"where j.jobid='%|Ss' and u.userid=j.userid",unique);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);

				if (edg_wll_ExecSQL(ctx,q,&stmt) < 0
					|| edg_wll_FetchRow(ctx,stmt,1,NULL,&owner) < 0
				) goto err;

				if (!edg_wll_gss_equal_subj(event->any.user, owner)) {
					edg_wll_SetError(ctx,EPERM,"Job already registered with different owner. Rejecting event.");
					goto err;
				}

				if (event->any.priority & EDG_WLL_LOGLFLAG_EXCL) {
					edg_wll_SetError(ctx,EEXIST,"Job with given jobid already exists. Rejecting event.");
					goto err;
				}
			}

			/* update job membership if needed */
			// if (!strcmp(res[0],"1") && !strcmp(res[1],"1") ) /*nothing to do */;
			if ( (!strcmp(res[0],"0") && ctx->isProxy) || (!strcmp(res[1],"0") && !ctx->isProxy) ) {
				trio_asprintf(&q,"update jobs set server='1', proxy='1' where jobid='%|Ss'",
					unique);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
				if (edg_wll_ExecSQL(ctx,q,NULL) < 0) goto err;
				free(q); q = NULL;
			}
		}
	}

err:
	free(res[0]); free(res[1]); free(res[2]);
	if (stmt) glite_lbu_FreeStmt(&stmt);
	free(subj);
	free(userid);
	free(unique);
	if (q) free(q);
	if (owner) free(owner);

	return edg_wll_Error(ctx,NULL,NULL);

}



#define SHORT_LEN	255	/* short_fiels.value db column lenght */

static int store_flesh(edg_wll_Context ctx,edg_wll_Event *e,const char *ulm,char *jobid,int no)
{
#ifndef LB_EVENTS_BLOB
	struct {
		char	*key;
		char	*val;
	} f[20];
	unsigned int	i;
#endif

	char	*stmt;
	int	err = 0;

	assert(ulm || e);
	edg_wll_ResetError(ctx);
	if (!ulm) ulm = edg_wll_UnparseEvent(ctx, e);
	if (!ulm) return edg_wll_Error(ctx, NULL, NULL);

#ifndef LB_EVENTS_BLOB
	// in both ways if some preparsed flesh fields in events too
	if (!e) {
		if (edg_wll_ParseEvent(ctx, (char *)ulm, &e) != 0) return edg_wll_Error(ctx, NULL, NULL);
	}

	memset(f,0,sizeof(f)); assert(f[0].key == NULL);

	switch (e->type) {
@@@{
	for my $type (getTypesOrdered $event) {
		next if $type eq '_common_';
		selectType $event $type;
		my $uctype = uc $type;
		my $flctype = lcfirst $type;
		if ($flctype =~ m/^pBS/) { $flctype = ucfirst $flctype; }
		if ($flctype =~ m/^cREAM/) { $flctype = ucfirst $flctype; }
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
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

		if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) err = edg_wll_Error(ctx,NULL,NULL);
		free(stmt);
	}
	for (i=0; i<sizeof(f)/sizeof(f[0]); i++) free(f[i].val);

/* XXX: hardcoded, no other suitable place to store it */
	if (!err) {
		trio_asprintf(&stmt,"insert into short_fields(jobid,event,name,value) "
			"values ('%|Ss',%d,'SRC_INSTANCE','%|Ss')",
			jobid,no,e->any.src_instance);
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

		if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) err = edg_wll_Error(ctx,NULL,NULL);
		free(stmt);
	}
#else
	if (!ulm) ulm = edg_wll_UnparseEvent(ctx, e);
	trio_asprintf(&stmt, "insert into events_flesh (jobid, event, ulm) values ('%|Ss', %d, '%|Ss')", jobid, no, ulm);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) err = edg_wll_Error(ctx,NULL,NULL);
#endif

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

static int
check_acl(edg_wll_Context ctx,edg_wll_Event *e, enum edg_wll_ChangeACLPermission p)
{
	int ret;
	edg_wll_Acl acl = NULL;
	struct _edg_wll_GssPrincipal_data princ;

	memset(&princ, 0, sizeof(princ));

	ret = edg_wll_GetACL(ctx, e->any.jobId, &acl);
	if (ret)
		return EPERM;

	/* XXX don't have VOMS attributes here */
	princ.name = e->any.user;
	ret = edg_wll_CheckACL_princ(ctx, acl, p, &princ);
	edg_wll_FreeAcl(acl);
	return ret;
}

static int check_auth(edg_wll_Context ctx,edg_wll_Event *e)
{
	char	*jobid = edg_wlc_JobIdGetUnique(e->any.jobId);
	char	*q = NULL,*owner = NULL;
	glite_lbu_Statement	stmt = NULL;

	edg_wll_ResetError(ctx);

	if (!ctx->isProxy && !ctx->peerName) 
		return edg_wll_SetError(ctx,EPERM,"can't store using unauthenticated connection");

	trio_asprintf(&q,"select u.cert_subj from jobs j, users u "
				"where j.jobid='%|Ss' and u.userid=j.userid",jobid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);

	if (edg_wll_ExecSQL(ctx,q,&stmt) < 0
		|| edg_wll_FetchRow(ctx,stmt,1,NULL,&owner) < 0
	) goto clean;

	if (!owner) {
		edg_wll_SetError(ctx, ENOENT, "job not registered");
		goto clean;
	}

	switch (e->any.source) {
		case EDG_WLL_SOURCE_USER_INTERFACE:
		case EDG_WLL_SOURCE_LRMS:
		case EDG_WLL_SOURCE_APPLICATION:
			if (edg_wll_gss_equal_subj(owner,e->any.user))
				break;
			if (e->any.type == EDG_WLL_EVENT_USERTAG &&
			    check_acl(ctx, e, EDG_WLL_CHANGEACL_TAG) == 0)
				break;
			if (e->any.type == EDG_WLL_EVENT_TAKEPAYLOADOWNERSHIP &&
			    strcmp(e->any.user,EDG_WLL_LOG_USER_DEFAULT) != 0)
				break;
		
			edg_wll_SetError(ctx,EPERM,"check_auth()");
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


edg_wll_ErrorCode intJobStat_embryonic(
	edg_wll_Context ctx,
	glite_jobid_const_t jobid,
	const edg_wll_RegJobEvent *e,
	intJobStat *stat)
{
	if (edg_wlc_JobIdDup(jobid, &intJobStat_to_JobStat(stat)->jobId) ||
		edg_wlc_JobIdDup(e->jobId, &intJobStat_to_JobStat(stat)->parent_job)) goto err;
	intJobStat_to_JobStat(stat)->state = EDG_WLL_JOB_SUBMITTED;
	intJobStat_to_JobStat(stat)->owner = strdup(e->user);
	if (e->jobtype == EDG_WLL_REGJOB_FILE_TRANSFER_COLLECTION)
		intJobStat_to_JobStat(stat)->jobtype = EDG_WLL_STAT_FILE_TRANSFER;
	else /* be defensive here */
		intJobStat_to_JobStat(stat)->jobtype = EDG_WLL_STAT_SIMPLE;
	intJobStat_to_JobStat(stat)->stateEnterTimes[1 + EDG_WLL_JOB_SUBMITTED] = (int)e->timestamp.tv_sec;
	intJobStat_to_JobStat(stat)->lastUpdateTime = e->timestamp;
	intJobStat_to_JobStat(stat)->stateEnterTime = e->timestamp;

err:
	return edg_wll_Error(ctx,NULL,NULL);
}

/*
 * Returns encoded SQL table states record for embryonic DAG subjob
 */

static edg_wll_ErrorCode states_values_embryonic(
	edg_wll_Context ctx,
	glite_jobid_const_t jobid,
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

	if (intJobStat_embryonic(ctx, jobid, e, stat)) goto err;
	
	jobid_md5 = edg_wlc_JobIdGetUnique(jobid);
	parent_md5 = edg_wlc_JobIdGetUnique(e->jobId);
	stat_enc = enc_intJobStat(strdup(""), stat);
	if (jobid_md5 == NULL || parent_md5 == NULL || stat_enc == NULL) goto err;


	if (edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, intJobStat_to_JobStat(stat), 1, icnames, &icvalues)) goto err;
	trio_asprintf(&stmt,
		"'%|Ss',%d,%d,'%|Ss','%|Ss','%|Ss'%s",
		jobid_md5, intJobStat_to_JobStat(stat)->state, 1, stat_enc,
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


int register_subjobs_embryonic(edg_wll_Context ctx,const edg_wll_RegJobEvent *e)
{
	int			i, j,  err = 0;
	edg_wlc_JobId		*subjobs = NULL;
	char			*jobid = NULL, *jobid_md5 = NULL, *jobid_md5_old = NULL;
	size_t			jobid_len;
	char			*icnames = NULL, *values = NULL, *userid = NULL, *stmt = NULL;
	int			server, proxy, membership = 0;
	glite_lbu_Statement     sh = NULL;


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
		

	/* increase the overall request timeout. */
	ctx->p_tmp_timeout.tv_sec += e->nsubjobs/10;
	if (ctx->p_tmp_timeout.tv_sec > 86400) ctx->p_tmp_timeout.tv_sec = 86400;

	if ((membership = edg_wll_jobMembership(ctx, e->jobId)) < 0) goto err;

	proxy = membership & DB_PROXY_JOB;
	server = membership & DB_SERVER_JOB;

	/* get userid of parent job */
	jobid = edg_wlc_JobIdGetUnique(e->jobId);
	trio_asprintf(&stmt,"select userid from jobs where jobid='%|Ss'", jobid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0 || edg_wll_FetchRow(ctx,sh,1,NULL,&userid) < 0) goto err;

	for (i=0; i<e->nsubjobs; i++) {
		char		*et,*ed,*job_s,*p,*p1;

		/* save jobid-userid relation into jobs table */
	        if ((err = store_job(ctx, subjobs[i], userid, proxy, server, 0, 0)))
			if (edg_wll_Error(ctx,&et,&ed) == EDEADLK) goto err;

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

		if (!err && (err = edg_wll_StoreIntStateEmbryonic(ctx, icnames, values)))
			edg_wll_Error(ctx,&et,&ed);

		if (err) {
			job_s = edg_wlc_JobIdUnparse(subjobs[i]);
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "%s: %s (%s)", job_s, et, ed);
			free(job_s); free(et); free(ed);
			edg_wll_ResetError(ctx);
		}
		edg_wlc_JobIdFree(subjobs[i]);
	}

err:
	free(jobid_md5_old);	//free the last one
	free(icnames);
	free(values);
	/* free the rest of subjobs if DEADLOCK occurs */
	for (j=i; j<e->nsubjobs; j++) edg_wlc_JobIdFree(subjobs[j]);
	free(subjobs);
	if (sh) glite_lbu_FreeStmt(&sh);
	free(stmt);

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
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	edg_wll_ExecSQL(ctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from long_fields where jobid='%|Ss' and event=%d",
		jobid,event);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	edg_wll_ExecSQL(ctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from events_flesh where jobid='%|Ss' and event=%d",
		jobid,event);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	edg_wll_ExecSQL(ctx,stmt,NULL);
	free(stmt);

	trio_asprintf(&stmt,
		"delete from events where jobid='%|Ss' and event=%d",
		jobid,event);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	edg_wll_ExecSQL(ctx,stmt,NULL);
	free (stmt);

	return edg_wll_Error(ctx,NULL,NULL);
}


/* XXX: if event type is user tag, convert the tag name to lowercase!
 * 	  (not sure whether to convert a value too is reasonable
 * 	  or keep it 'case sensitive')
 */
static void lowercase_usertag(edg_wll_Event *ev)
{
	int i;

	if ( ev->any.type == EDG_WLL_EVENT_USERTAG ) {

		for ( i = 0; ev->userTag.name[i] != '\0'; i++ )
		ev->userTag.name[i] = tolower(ev->userTag.name[i]);
	}
}

int edg_wll_jobsconnection_create(edg_wll_Context ctx, glite_jobid_const_t jobid_from, glite_jobid_const_t jobid_to, enum edg_wll_StatJobtype jobtype, edg_wll_JobConnectionType connectiontype){
        char *stmt;

	char *jobid_from_u, *jobid_to_u;
	jobid_from_u = edg_wlc_JobIdGetUnique(jobid_from);
	jobid_to_u = edg_wlc_JobIdGetUnique(jobid_to);

	// clear previous connection of same jobs (e.g. resubmition on same machine)
	trio_asprintf(&stmt,
		"delete from job_connections where jobid_from='%s' and jobid_to='%s'",
		jobid_from_u, jobid_to_u
	);

	edg_wll_ExecSQL(ctx,stmt,NULL);

	free(stmt);

        trio_asprintf(&stmt,
                "insert into job_connections (jobid_from, jobid_to, jobtype, connection) values ('%s', '%s', %i, %i)", 
		jobid_from_u, jobid_to_u, jobtype, connectiontype
        );
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	free(jobid_from_u);
	free(jobid_to_u);

        edg_wll_ExecSQL(ctx,stmt,NULL);

        free(stmt);

        return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_jobsconnection_modify(edg_wll_Context ctx, glite_jobid_const_t jobid_from, glite_jobid_const_t jobid_to, edg_wll_JobConnectionType connectiontype){
        char *stmt;

        char *jobid_from_u, *jobid_to_u;
        jobid_from_u = edg_wlc_JobIdGetUnique(jobid_from);
        jobid_to_u = edg_wlc_JobIdGetUnique(jobid_to);

        trio_asprintf(&stmt,
                "update job_connections set connection=%i where jobid_from='%s' and jobid_to='%s'",
                connectiontype, jobid_from_u, jobid_to_u
        );
        glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
        free(jobid_from_u);
        free(jobid_to_u);

        edg_wll_ExecSQL(ctx,stmt,NULL);

        free(stmt);

        return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_jobsconnection_modifyall(edg_wll_Context ctx, glite_jobid_const_t jobid, edg_wll_JobConnectionType oldtype, edg_wll_JobConnectionType newtype){
        char *stmt;

        char *jobid_u;
        jobid_u = edg_wlc_JobIdGetUnique(jobid);

        trio_asprintf(&stmt,
                "update job_connections set connection=%i where connection=%i and (jobid_from='%s' or jobid_to='%s')",
                newtype, oldtype, jobid_u, jobid_u
        );
        glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
        free(jobid_u);

        edg_wll_ExecSQL(ctx,stmt,NULL);

        free(stmt);

        return edg_wll_Error(ctx, NULL, NULL);
}


int edg_wll_jobsconnection_purgeall(edg_wll_Context ctx, glite_jobid_const_t jobid){
	char *stmt;

	char *jobid_u;
	jobid_u = edg_wlc_JobIdGetUnique(jobid);

	trio_asprintf(&stmt,
		"delete from job_connections where jobid_from='%s' or jobid_to='%s'",
		jobid_u, jobid_u
	);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	free(jobid_u);

	edg_wll_ExecSQL(ctx,stmt,NULL);

	free(stmt);

	return edg_wll_Error(ctx, NULL, NULL);
}

