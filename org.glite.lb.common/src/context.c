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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "glite/jobid/strmd5.h"
#include "glite/jobid/cjobid.h"
#include "context-int.h"
// #include "producer.h"

static void free_voms_groups(edg_wll_VomsGroups *);

/* uncomment to get the trace
#define CTXTRACE "/tmp/lb-context-trace"
*/

int edg_wll_InitContext(edg_wll_Context *ctx)
{
	int i, ret;
	edg_wll_Context out	= (edg_wll_Context) malloc(sizeof(*out));
	union {
		int i;
		char *s;
		struct timeval *tv;
	} null;

	if (!out) return ENOMEM;
	memset(out,0,sizeof(*out));
	assert(out->errDesc == NULL);

	out->allowAnonymous = 1;
	out->notifSock	= -1;

	out->p_tmp_timeout.tv_sec = out->p_log_timeout.tv_sec;
	out->p_tmp_timeout.tv_usec = out->p_log_timeout.tv_usec;

	out->connNotif = (edg_wll_Connections *) calloc(1, sizeof(edg_wll_Connections));
	out->connProxy = (edg_wll_ConnProxy *) calloc(1, sizeof(edg_wll_ConnProxy));
	if (!out->connNotif || !out->connProxy) goto enomem;
	out->connections = edg_wll_initConnections();
	edg_wll_initConnNotif(out->connNotif);
	out->connProxy->conn.sock = -1;
//	out->connToUse = -1;

	memset(&null, 0, sizeof null);
	for (i=0; i<EDG_WLL_PARAM__LAST; i++) {
		if ((ret = edg_wll_SetParam(out,i,null)) != 0) {
			edg_wll_FreeContext(out);
			return ret;
		}
	}

	*ctx = out;

#ifdef CTXTRACE
{
	int	trc = open(CTXTRACE,O_WRONLY|O_CREAT,0644);
	char	buf[200];
	sprintf(buf,"%p init\n",out);
	lseek(trc,0,SEEK_END);
	write(trc,buf,strlen(buf));
	close(trc);
}
#endif

	return 0;
enomem:
	edg_wll_FreeParams(out);
	free(out->connNotif);
	free(out->connProxy);
	free(out);
	return ENOMEM;
}

void edg_wll_FreeContext(edg_wll_Context ctx)
{
	struct timeval close_timeout = {0, 50000};
	int	i;

	if (!ctx) return;
#ifdef CTXTRACE
{
	int	trc = open(CTXTRACE,O_WRONLY|O_CREAT,0644);
	char	buf[200];
	sprintf(buf,"%p free\n",ctx);
	lseek(trc,0,SEEK_END);
	write(trc,buf,strlen(buf));
	close(trc);
}
#endif
	if (ctx->errDesc) free(ctx->errDesc);
	if (ctx->connections->connPool) {
#ifdef GLITE_LB_THREADED
		int i;

                /* Since the introduction of a shared connection pool, the pool
                   cannot be freed here. We only need to unlock connections that
                   may have been locked using this context. */

                #ifdef EDG_WLL_CONNPOOL_DEBUG
                     printf("Running edg_wll_FreeContext - checking for connections locked by the current context.\n");
		#endif

                edg_wll_poolLock();

		for (i=0; i<ctx->connections->poolSize; i++) {
	
                        if (ctx->connections->locked_by[i]==ctx) {
                                #ifdef EDG_WLL_CONNPOOL_DEBUG
                                    printf("Unlocking connection No. %d...",i);
                                #endif
				edg_wll_connectionUnlock(ctx, i);

			}
		}

                edg_wll_poolUnlock();
#endif

/*		
		for (i=0; i<ctx->connections->poolSize; i++) {
			if (ctx->connections->connPool[i].peerName) free(ctx->connections->connPool[i].peerName);
			edg_wll_gss_close(&ctx->connections->connPool[i].gss,&close_timeout);
			if (ctx->connections->connPool[i].gsiCred)
				edg_wll_gss_release_cred(&ctx->connections->connPool[i].gsiCred, NULL);
			if (ctx->connections->connPool[i].buf) free(ctx->connections->connPool[i].buf);
		}	
		free(ctx->connections->connPool);*/
	}
 	if (ctx->connNotif) {
		for (i=0; i<ctx->connNotif->poolSize; i++) {
	 		if (ctx->connNotif->connPool[i].peerName) free(ctx->connNotif->connPool[i].peerName);
 			edg_wll_gss_close(&ctx->connNotif->connPool[i].gss,&close_timeout);
 			if (ctx->connNotif->connPool[i].gsiCred)
 				edg_wll_gss_release_cred(&ctx->connNotif->connPool[i].gsiCred, NULL);
	 		if (ctx->connNotif->connPool[i].buf) free(ctx->connNotif->connPool[i].buf);
	 		if (ctx->connNotif->connPool[i].bufOut) free(ctx->connNotif->connPool[i].bufOut);
		}
 		free(ctx->connNotif->connPool);
		free(ctx->connNotif);
 	}
	if ( ctx->connProxy ) {
		if ( ctx->connProxy->buf ) free(ctx->connProxy->buf);
		edg_wll_plain_close(&ctx->connProxy->conn);
		free(ctx->connProxy);
	}
	if (ctx->notifSock >=0) close(ctx->notifSock);
	if (ctx->srvName) free(ctx->srvName);
	if (ctx->peerName) free(ctx->peerName);
	if (ctx->vomsGroups.len) free_voms_groups(&ctx->vomsGroups);
	if (ctx->dumpStorage) free(ctx->dumpStorage);
	if (ctx->purgeStorage) free(ctx->purgeStorage);
	if (ctx->fqans) {
		char **f;
		for (f = ctx->fqans; f && *f; f++)
			free(*f);
		free(ctx->fqans);
		ctx->fqans = NULL;
	}
	if (ctx->authz_policy.actions_num) {
		for (i = 0; i < ctx->authz_policy.actions_num; i++) {
			int j;
			struct _edg_wll_authz_attr *a;
			for (j = 0; j < ctx->authz_policy.actions[i].rules_num; j++) {
				a = ctx->authz_policy.actions[i].rules[j].attrs;
				if (a && a->value)
					free(a->value);
			}
			free(ctx->authz_policy.actions[i].rules);
		}
		free (ctx->authz_policy.actions);
	}
	
	if (ctx->jpreg_dir) free(ctx->jpreg_dir);
	if (ctx->serverIdentity) free(ctx->serverIdentity);
	if (ctx->msg_prefixes) {
		char **fm;
		for (fm = ctx->msg_prefixes; fm && *fm; fm++)
			free(*fm);
		free(ctx->msg_prefixes);
		ctx->msg_prefixes = NULL;
	}
	if (ctx->msg_brokers) {
		char **fm;
		for (fm = ctx->msg_brokers; fm && *fm; fm++)
			free(*fm);
		free(ctx->msg_brokers);
		ctx->msg_brokers = NULL;
	}
	
	if (ctx->authz_policy_file) free(ctx->authz_policy_file);

	edg_wll_FreeParams(ctx);

	free(ctx);
}

static const char* const errTexts[] = {
	/* standard error strings should appear here */
	"Broken ULM",
	"Undefined event",
	"Message incomplete",
	"Duplicate ULM key",
	"Misuse of ULM key",
	"Warning: extra ULM fields",
	"XML Parse error",
	"Server response error",
	"Bad JobId format",
	"Database initialization failed",
	"Database call failed",
	"MD5 key clash",
	"GSSAPI Error",
	"DNS resolver error",
	"No JobId specified in context",
	"No indexed condition in query",
	"LB server (bkserver,lbproxy) store protocol error",
	"Interlogger internal error",
	"Interlogger has events pending",
	"Compared events differ",
	"DB deadlock detected",
	"DB connection lost",
	"Background operation accepted",
};

const char *edg_wll_GetErrorText(int code) {
	return code ? 
		(code <= EDG_WLL_ERROR_BASE ?
			strerror(code) :
			errTexts[code - EDG_WLL_ERROR_BASE - 1]
		) :
		NULL;
}

int edg_wll_Error(edg_wll_Context ctx, char **errText, char **errDesc)
{
	char	*text = NULL,*desc = NULL;
	const char* et = edg_wll_GetErrorText(ctx->errCode);

	if (et) {
		text = strdup(et);
		if (ctx->errDesc) desc = (char *) strdup(ctx->errDesc);
	}

	if (errText) *errText = text; else free(text);
	if (errDesc) *errDesc = desc; else free(desc);
	return ctx->errCode;
}

int edg_wll_ResetError(edg_wll_Context ctx)
{
	if (ctx->errDesc) free(ctx->errDesc);
	ctx->errDesc = NULL;
	ctx->errCode =  0;

	return ctx->errCode;
}

int edg_wll_SetError(edg_wll_Context ctx,int code,const char *desc)
{
	edg_wll_ResetError(ctx);
	if (code) {
		ctx->errCode = code;
		if (desc) ctx->errDesc = (char *) strdup(desc);
	}

	return ctx->errCode;
}


int edg_wll_UpdateError(edg_wll_Context ctx,int code,const char *desc)
{
        char	*new_desc=NULL;
	char	*old_desc=NULL;
        const char* err_text = edg_wll_GetErrorText(ctx->errCode);

	/* first fill in the old_desc */
        if (ctx->errCode) {
                if (ctx->errDesc) {
                        if ((err_text)) // && (code) && (code <> ctx->errCode))
                                asprintf(&old_desc,"%s;; %s",err_text,ctx->errDesc);
                        else
                                old_desc = (char *) strdup(ctx->errDesc);
                } else {
                        old_desc = (char *) strdup(err_text);
                }
	} else {
                if (ctx->errDesc) old_desc = (char *) strdup(ctx->errDesc);
        }
	if (ctx->errDesc) free(ctx->errDesc);

	/* update errDesc */
        if (old_desc) {
                if (desc) {
                        asprintf(&new_desc,"%s\n%s",desc,old_desc);
                        ctx->errDesc = (char *) strdup(new_desc);
                } else {
                        ctx->errDesc = (char *) strdup(old_desc);
		}
        } else {
                if (desc) ctx->errDesc = (char *) strdup(desc);
        }

	/* update errCode */
        if (code) {
                ctx->errCode = code;
        }

        if (new_desc) free(new_desc);
        if (old_desc) free(old_desc);

        return ctx->errCode;
}

static const char* const srcNames[] = {
	"Undefined",
	"UserInterface",
	"NetworkServer",
	"WorkloadManager",
	"BigHelper",
	"JobController",
	"LogMonitor",
	"LRMS",
	"Application",
	"LBServer",
	"CREAMInterface",
	"CREAMExecutor",
	"PBSClient",
	"PBSServer",
	"PBSMomSuperior",
	"PBSMom",
	"PBSScheduler"
};

edg_wll_Source edg_wll_StringToSource(const char *name)
{
	int	i;
	
        for (i=1; i<sizeof(srcNames)/sizeof(srcNames[0]); i++)
                if (strcasecmp(srcNames[i],name) == 0) return (edg_wll_Source) i;
        return EDG_WLL_SOURCE_NONE;
}

char * edg_wll_SourceToString(edg_wll_Source src)
{
        if (src < EDG_WLL_SOURCE_NONE || src >= EDG_WLL_SOURCE__LAST) return NULL;
        return strdup(srcNames[src]);
}

static const char* const queryResultNames[] = {
	"Undefined",
	"None",
	"All",
	"Limited"
};

edg_wll_QueryResults edg_wll_StringToQResult(const char *name)
{
	int	i;

	for (i=1; i<sizeof(queryResultNames)/sizeof(queryResultNames[0]); i++)
		if (strcasecmp(queryResultNames[i],name) == 0)
			return (edg_wll_QueryResults) i;

	return EDG_WLL_QUERYRES_UNDEF;
}

char * edg_wll_QResultToString(edg_wll_QueryResults res)
{
	if (res < EDG_WLL_QUERYRES_NONE || res >= EDG_WLL_QUERYRES__LAST)
		return NULL;

	return strdup(queryResultNames[res]);
}


char *edg_wll_GetSequenceCode(const edg_wll_Context ctx)
{
	unsigned int *c;
	char *ret = NULL;

	switch (ctx->p_seqcode.type) {
		case EDG_WLL_SEQ_CREAM:
			 /* fall through */
		case EDG_WLL_SEQ_DUPLICATE:
			/* fall through */
		case EDG_WLL_SEQ_NORMAL:
			c = &ctx->p_seqcode.c[0];
			asprintf(&ret, EDG_WLL_SEQ_FORMAT_PRINTF,
					c[EDG_WLL_SOURCE_USER_INTERFACE],
					c[EDG_WLL_SOURCE_NETWORK_SERVER],
					c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
					c[EDG_WLL_SOURCE_BIG_HELPER],
					c[EDG_WLL_SOURCE_JOB_SUBMISSION],
					c[EDG_WLL_SOURCE_LOG_MONITOR],
					c[EDG_WLL_SOURCE_LRMS],
					c[EDG_WLL_SOURCE_APPLICATION],
					c[EDG_WLL_SOURCE_LB_SERVER]);
			break;
		case EDG_WLL_SEQ_PBS:
			c = &ctx->p_seqcode.c[0];
			asprintf(&ret, EDG_WLL_SEQ_PBS_FORMAT_PRINTF,
				 c[EDG_WLL_SOURCE_PBS_CLIENT],
				 c[EDG_WLL_SOURCE_PBS_SERVER],
				 c[EDG_WLL_SOURCE_PBS_SCHEDULER],
				 c[EDG_WLL_SOURCE_PBS_SMOM],
				 c[EDG_WLL_SOURCE_PBS_MOM]);
			/* ret = strdup(ctx->p_seqcode.pbs); */
			break;
		case EDG_WLL_SEQ_CONDOR:
			ret = strdup(ctx->p_seqcode.condor);
			break;
		default:
			edg_wll_SetError(ctx,EINVAL,"edg_wll_GetSequenceCode(): unknown sequence code type");
			return NULL;
	}
	
	return ret;
}

int edg_wll_SetSequenceCode(edg_wll_Context ctx,
			const char * seqcode_str, int seq_type)
{
	int res;
	unsigned int *c;


	edg_wll_ResetError(ctx);

	ctx->p_seqcode.type = seq_type;

	switch (seq_type) {
		case EDG_WLL_SEQ_CREAM:
			/* fall through */
		case EDG_WLL_SEQ_DUPLICATE:
			/* fall through */
		case EDG_WLL_SEQ_NORMAL:
			if (!seqcode_str) {
				memset(&ctx->p_seqcode.c, 0, sizeof ctx->p_seqcode.c);
				return 0;
			}

			c = &ctx->p_seqcode.c[0];
			res =  sscanf(seqcode_str, EDG_WLL_SEQ_FORMAT_SCANF,
					&c[EDG_WLL_SOURCE_USER_INTERFACE],
					&c[EDG_WLL_SOURCE_NETWORK_SERVER],
					&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
					&c[EDG_WLL_SOURCE_BIG_HELPER],
					&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
					&c[EDG_WLL_SOURCE_LOG_MONITOR],
					&c[EDG_WLL_SOURCE_LRMS],
					&c[EDG_WLL_SOURCE_APPLICATION],
					&c[EDG_WLL_SOURCE_LB_SERVER]);

			/* XXX: can't be true anymore. What was the reason for assert()? 
			assert(EDG_WLL_SOURCE__LAST == 10); */
			if (res == EDG_WLL_SOURCE_LB_SERVER-1) {
				/* pre-collections compatibility */
				c[EDG_WLL_SOURCE_LB_SERVER] = 0;
			} else if (res != EDG_WLL_SEQ_FORMAT_NUMBER) {
				if (seq_type == EDG_WLL_SEQ_CREAM)
					return 0;
				else
					return edg_wll_SetError(ctx, EINVAL,
						"edg_wll_SetSequenceCode(): syntax error in sequence code");
			}

			if (seq_type == EDG_WLL_SEQ_DUPLICATE) {
				if (ctx->p_source <= EDG_WLL_SOURCE_NONE || 
						ctx->p_source >= EDG_WLL_SOURCE__LAST)
				{
					return edg_wll_SetError(ctx,EINVAL,
						"edg_wll_SetSequenceCode(): context param: source missing");
				}
				c[ctx->p_source] = time(NULL);
			}
			break;
		case EDG_WLL_SEQ_PBS:
			/* original version
			if (!seqcode_str) 
				memset(&ctx->p_seqcode.pbs, 0, sizeof ctx->p_seqcode.pbs);
			else
				strncpy(ctx->p_seqcode.pbs, seqcode_str, sizeof(ctx->p_seqcode.pbs));
			*/
			if (!seqcode_str) {
				memset(&ctx->p_seqcode.c, 0, sizeof(ctx->p_seqcode.c));
				return 0;
			}
			c = ctx->p_seqcode.c;
			res = sscanf(seqcode_str, EDG_WLL_SEQ_PBS_FORMAT_SCANF,
				     &c[EDG_WLL_SOURCE_PBS_CLIENT],
				     &c[EDG_WLL_SOURCE_PBS_SERVER],
				     &c[EDG_WLL_SOURCE_PBS_SCHEDULER],
				     &c[EDG_WLL_SOURCE_PBS_SMOM],
				     &c[EDG_WLL_SOURCE_PBS_MOM]);
			if(res != EDG_WLL_SEQ_PBS_FORMAT_NUMBER) {
				return edg_wll_SetError(ctx, EINVAL,
					"edg_wll_SetSequenceCode(): syntax error in sequence code");
			}
			break;
		case EDG_WLL_SEQ_CONDOR:
			if (!seqcode_str) 
				memset(&ctx->p_seqcode.condor, 0, sizeof ctx->p_seqcode.condor);
			else
				strncpy(ctx->p_seqcode.condor, seqcode_str, sizeof(ctx->p_seqcode.condor));
			break;
		default:
			return edg_wll_SetError(ctx, EINVAL,
				"edg_wll_SetSequenceCode(): unrecognized value of seq_type parameter");
	}

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_IncSequenceCode(edg_wll_Context ctx)
{
	edg_wll_ResetError(ctx);

	switch (ctx->p_seqcode.type) {
		case EDG_WLL_SEQ_CREAM:
			/* fall through */
		case EDG_WLL_SEQ_DUPLICATE:
			/* fall through */
		case EDG_WLL_SEQ_NORMAL:
			if (ctx->p_source <= EDG_WLL_SOURCE_NONE || 
					ctx->p_source >= EDG_WLL_SOURCE__LAST) 
			{
				return edg_wll_SetError(ctx,EINVAL,
					"edg_wll_IncSequenceCode(): context param: source missing");
			}

			ctx->p_seqcode.c[ctx->p_source]++;
			break;
		case EDG_WLL_SEQ_PBS:
			/* no action */
			break;
		default:
			break;
	}

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_GetLoggingJob(const edg_wll_Context ctx,edg_wlc_JobId *out)
{
	return edg_wlc_JobIdDup(ctx->p_jobid,out);
}

int edg_wll_GenerateSubjobIds(
	edg_wll_Context	ctx,
	glite_jobid_const_t	parent,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs)
{
	int subjob, ret;
	char *p_unique, *p_bkserver, *intseed;
	char *unhashed, *hashed;
	unsigned int p_port;
	edg_wlc_JobId *retjobs;


	if (num_subjobs < 1)
		return edg_wll_SetError(ctx, EINVAL,
				"edg_wll_GenerateSubjobIds(): num_subjobs < 1");

	p_unique = edg_wlc_JobIdGetUnique(parent);
	edg_wlc_JobIdGetServerParts(parent, &p_bkserver, &p_port);

	retjobs = calloc(num_subjobs+1, sizeof(edg_wlc_JobId));

	if (p_unique == NULL ||
		p_bkserver == NULL ||
		retjobs == NULL)
		return edg_wll_SetError(ctx, ENOMEM, NULL);

	if ( !seed ) {
		intseed = strdup("edg_wll_GenerateSubjobIds()");
	} else {
		intseed = strdup(seed);
	}

	for (subjob = 0; subjob < num_subjobs; subjob++) {

		asprintf(&unhashed, "%s,%s,%d", p_unique, intseed, subjob);
		if (unhashed == NULL) {
			edg_wll_SetError(ctx, ENOMEM, "edg_wll_GenerateSubjobIds(): asprintf() error");
			goto handle_error;
		}

		hashed = str2md5base64(unhashed);
		free(unhashed);
		if (hashed == NULL) {
			edg_wll_SetError(ctx, ENOMEM, "edg_wll_GenerateSubjobIds(): str2md5base64() error");
			goto handle_error;
		}

		ret = edg_wlc_JobIdRecreate(p_bkserver, p_port, hashed, &retjobs[subjob]);
		free(hashed);
		if (ret != 0) {
			edg_wll_SetError(ctx, ret, "edg_wll_GenerateSubjobIds(): edg_wlc_JobIdRecreate() error");
			goto handle_error;
		}
	}
	
	free(intseed);
	free(p_unique);
	free(p_bkserver);

	*subjobs = retjobs;
	return 0;

    handle_error:
	free(intseed);
	free(p_unique);
	free(p_bkserver);
	for ( subjob-- ;subjob >= 0; subjob--)
		edg_wlc_JobIdFree(retjobs[subjob]);
	return edg_wll_Error(ctx, NULL, NULL);
}

static void
free_voms_groups(edg_wll_VomsGroups *groups)
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

int edg_wll_SetErrorGss(edg_wll_Context ctx, const char *desc, edg_wll_GssStatus *gss_code)
{
   char *err_msg;

   edg_wll_gss_get_error(gss_code, desc, &err_msg);
   edg_wll_SetError(ctx,EDG_WLL_ERROR_GSS, err_msg);
   free(err_msg);
   return ctx->errCode;
}

int
edg_wll_add_authz_attr(edg_wll_Context ctx,
                       struct _edg_wll_authz_rule *rule,
                       int id,
                       char *value)
{
    struct _edg_wll_authz_attr *attrs = rule->attrs;
    int num = rule->attrs_num;

    attrs = realloc(rule->attrs, (num + 1) * sizeof(*attrs));
    if (attrs == NULL)
	return edg_wll_SetError(ctx, ENOMEM, NULL);

    attrs[num].id = id;
    attrs[num].value = strdup(value);
    rule->attrs = attrs;
    rule->attrs_num++;

    return 0;
}

int
edg_wll_add_authz_rule(edg_wll_Context ctx,
		       edg_wll_authz_policy policy,
		       int action,
		       struct _edg_wll_authz_rule *rule)
{
    struct _edg_wll_authz_rule *rules;
    struct _edg_wll_authz_action *a = policy->actions;
    int idx;
    int num, i;

    num = policy->actions_num;
    for (idx = 0; idx < num; idx++)
	if (policy->actions[idx].id == action)
	    break;

    if (idx == num) {
	a = realloc(policy->actions, (num + 1) * sizeof(*a));
	if (a == NULL)
	    return edg_wll_SetError(ctx, ENOMEM, NULL);
	a[num].id = action;
	a[num].rules = NULL;
	a[num].rules_num = 0;
	policy->actions = a;
	policy->actions_num++;
    }

    num = policy->actions[idx].rules_num;
    rules = policy->actions[idx].rules;
    rules = realloc(rules, (num + 1) * sizeof(*rules));
    if (rules == NULL)
        return edg_wll_SetError(ctx, ENOMEM, NULL);
    rules[num].attrs = NULL;
    rules[num].attrs_num = 0;
    for (i=0; i < rule->attrs_num; i++)
	edg_wll_add_authz_attr(ctx, &rules[num],
			       rule->attrs[i].id, rule->attrs[i].value);
    policy->actions[idx].rules = rules;
    policy->actions[idx].rules_num++;

    return 0;
}

int
edg_wll_ParseMSGConf(char *msg_conf, char ***brokers, char ***prefixes) {
	FILE *conf;
	char l[512];
	char *data, *d_to_parse;
	int inmsg = 0, ntoks;
	char *tok_r = NULL;
	char *token;
	char **tokens;
	

	conf = fopen (msg_conf, "r");
	if (conf == NULL) return -1; //Cannot open file

	while( 1 ) {
		fgets(l, 512, conf);
		if ( feof(conf) ) break;

		if (l[0] == '[') { // Detect section [msg]
			if (!strncasecmp(l, "[msg]", 5)) inmsg = 1;
			else inmsg = 0;
		}
		else if (inmsg) {
			if ((!strncasecmp(l, "prefix", 6)) || (!strncasecmp(l, "broker", 6))) {
				data=strchr(l, '=');
				if (!data) return -2; // No '='
//				data = data[1];
				if (strlen(data) < 1) return -2; // No text after '='

				tokens = NULL; ntoks = 0;
				for (d_to_parse = data+1; ; d_to_parse = NULL) {
					token = strtok_r(d_to_parse, " \t\n", &tok_r);
					if (token == NULL) break;

					tokens = (char**) realloc (tokens, sizeof(char**) * (ntoks + 2));
					asprintf(&(tokens[ntoks]), "%s", token);
					tokens[++ntoks] = NULL;	
				} 

				if (!strncasecmp(l, "prefix", 6)) *prefixes=tokens; 
				else *brokers=tokens;
			}

		}

	}
	
	return 0;
}
