#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <globus_config.h>

#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/wmsutils/jobid/cjobid.h"
#include "context-int.h"
#include "glite/lb/producer.h"

static void free_voms_groups(edg_wll_VomsGroups *);

int edg_wll_InitContext(edg_wll_Context *ctx)
{
	int i;
	edg_wll_Context out	= (edg_wll_Context) malloc(sizeof(*out));

	if (!out) return ENOMEM;
	memset(out,0,sizeof(*out));
	assert(out->errDesc == NULL);

	out->allowAnonymous = 1;
	out->notifSock	= -1;

	/* XXX */
	for (i=0; i<EDG_WLL_PARAM__LAST; i++) edg_wll_SetParam(out,i,NULL);

	out->connPool = (edg_wll_ConnPool *) calloc(out->poolSize, sizeof(edg_wll_ConnPool));
	out->connPoolNotif = (edg_wll_ConnPool *) calloc(1, sizeof(edg_wll_ConnPool));
	out->connProxy = (edg_wll_ConnPool *) calloc(1, sizeof(edg_wll_ConnProxy));
	out->connProxy->conn.sock = -1;

	*ctx = out;
	return 0;
}

void edg_wll_FreeContext(edg_wll_Context ctx)
{
	struct timeval close_timeout = {0, 50000};
	OM_uint32 min_stat;

	if (!ctx) return;

	if (ctx->errDesc) free(ctx->errDesc);
	if (ctx->connPool) {
		int i;
		
		for (i=0; i<ctx->poolSize; i++) {
			if (ctx->connPool[i].peerName) free(ctx->connPool[i].peerName);
			edg_wll_gss_close(&ctx->connPool[i].gss,&close_timeout);
			if (ctx->connPool[i].gsiCred)
				gss_release_cred(&min_stat, &ctx->connPool[i].gsiCred);
			if (ctx->connPool[i].buf) free(ctx->connPool[i].buf);
		}	
		free(ctx->connPool);
	}
 	if (ctx->connPoolNotif) {
 		if (ctx->connPoolNotif[0].peerName) free(ctx->connPoolNotif[0].peerName);
 		edg_wll_gss_close(&ctx->connPoolNotif[0].gss,&close_timeout);
 		if (ctx->connPoolNotif[0].gsiCred)
 			gss_release_cred(&min_stat, &ctx->connPoolNotif[0].gsiCred);
 		if (ctx->connPoolNotif[0].buf) free(ctx->connPoolNotif[0].buf);
 		free(ctx->connPoolNotif);
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

        if (ctx->p_jobid) edg_wlc_JobIdFree(ctx->p_jobid);
        if (ctx->p_host) free(ctx->p_host);
        if (ctx->p_instance) free(ctx->p_instance);
        if (ctx->p_destination) free(ctx->p_destination);
        if (ctx->p_user_lbproxy) free(ctx->p_user_lbproxy);
        if (ctx->p_query_server) free(ctx->p_query_server);
        if (ctx->p_notif_server) free(ctx->p_notif_server);
        if (ctx->p_proxy_filename) free(ctx->p_proxy_filename);
        if (ctx->p_cert_filename) free(ctx->p_cert_filename);
        if (ctx->p_key_filename) free(ctx->p_key_filename);

	/* do not free (references only)
	 * ctx->job_index
	 * ctx->job_index_cols
	 * ctx->mysql */

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
	"Database call failed",
	"Bad URL format",
	"MD5 key clash",
	"GSSAPI Error",
	"DNS resolver error",
	"No JobId specified in context",
	"No indexed condition in query",
	"Interlogger protocol error",
	"Interlogger internal error",
	"Interlogger has events pending",
	"Compared events differ",
	"SQL parse error",
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
                        asprintf(&new_desc,"%s;; %s",desc,old_desc);
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

	c = &ctx->p_seqcode.c[0];
	asprintf(&ret, "UI=%06d:NS=%010d:WM=%06d:BH=%010d:JSS=%06d"
				":LM=%06d:LRMS=%06d:APP=%06d",
			c[EDG_WLL_SOURCE_USER_INTERFACE],
			c[EDG_WLL_SOURCE_NETWORK_SERVER],
			c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			c[EDG_WLL_SOURCE_BIG_HELPER],
			c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			c[EDG_WLL_SOURCE_LOG_MONITOR],
			c[EDG_WLL_SOURCE_LRMS],
			c[EDG_WLL_SOURCE_APPLICATION]);
	return ret;
}

int edg_wll_SetSequenceCode(edg_wll_Context ctx,
			const char * seqcode_str, int seq_type)
{
	int res;
	unsigned int *c;
	int duplicate = 0;

	edg_wll_ResetError(ctx);

	if (seq_type == EDG_WLL_SEQ_DUPLICATE) {
		duplicate = 1;
	} else if (seq_type != EDG_WLL_SEQ_NORMAL)
		return edg_wll_SetError(ctx, EINVAL,
			"edg_wll_SetSequenceCode(): unrecognized value of seq_type parameter");
	
	if (!seqcode_str) {
		memset(&ctx->p_seqcode,0,sizeof ctx->p_seqcode);
		return 0;
	}

	c = &ctx->p_seqcode.c[0];
	res =  sscanf(seqcode_str, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION]);

	assert(EDG_WLL_SOURCE__LAST == 9);
	if (res != EDG_WLL_SOURCE__LAST-1)
		return edg_wll_SetError(ctx, EINVAL, "edg_wll_SetSequenceCode(): syntax error in sequence code");

	if (duplicate) {
		if (ctx->p_source <= EDG_WLL_SOURCE_NONE || ctx->p_source >= EDG_WLL_SOURCE__LAST)
			return edg_wll_SetError(ctx,EINVAL,"edg_wll_SetSequenceCode(): context param: source missing");
		c[ctx->p_source] = time(NULL);
	}

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_IncSequenceCode(edg_wll_Context ctx)
{
	edg_wll_ResetError(ctx);
	
	if (ctx->p_source <= EDG_WLL_SOURCE_NONE || ctx->p_source >= EDG_WLL_SOURCE__LAST)
		return edg_wll_SetError(ctx,EINVAL,"edg_wll_IncSequenceCode(): context param: source missing");

	ctx->p_seqcode.c[ctx->p_source]++;

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_GetLoggingJob(const edg_wll_Context ctx,edg_wlc_JobId *out)
{
	return edg_wlc_JobIdDup(ctx->p_jobid,out);
}

int edg_wll_GenerateSubjobIds(
	edg_wll_Context	ctx,
	const edg_wlc_JobId	parent,
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
	if (seed == NULL)
		intseed = edg_wll_GetSequenceCode(ctx);
	else
		intseed = strdup(seed);

	p_unique = edg_wlc_JobIdGetUnique(parent);
	edg_wlc_JobIdGetServerParts(parent, &p_bkserver, &p_port);

	retjobs = calloc(num_subjobs+1, sizeof(edg_wlc_JobId));

	if (p_unique == NULL ||
		intseed == NULL ||
		p_bkserver == NULL ||
		retjobs == NULL)
		return edg_wll_SetError(ctx, ENOMEM, NULL);

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
