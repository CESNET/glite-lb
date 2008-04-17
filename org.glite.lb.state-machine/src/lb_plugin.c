#ident "$Header$"

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#include <cclassad.h>

#include "glite/lbu/trio.h"

#include "glite/lb/jobstat.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"


#include "intjobstat.h"
#include "seqcode_aux.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/builtin_plugins.h"
#include "glite/jp/backend.h"
#include "glite/jp/attr.h"
#include "glite/jp/known_attr.h"
#include "job_attrs.h"
#include "job_attrs2.h"

#define INITIAL_NUMBER_EVENTS 100
#define INITIAL_NUMBER_STATES EDG_WLL_NUMBER_OF_STATCODES

typedef struct _lb_historyStatus {
	edg_wll_JobStatCode 	state;
	struct timeval 		timestamp;
	char 			*reason;
	char			*destination;
	int			done_code;
} lb_historyStatus;

typedef struct _lb_handle {
	edg_wll_Event			**events;
	edg_wll_JobStat			status;
	lb_historyStatus		**fullStatusHistory, **lastStatusHistory, *finalStatus;
	glite_jpps_fplug_data_t*	classad_plugin;
} lb_handle;

typedef struct _rl_buffer_t {
        char                    *buf;
        size_t                  pos, size;
        off_t                   offset;
} rl_buffer_t;

#define check_strdup(s) ((s) ? strdup(s) : NULL)

extern int processEvent(intJobStat *, edg_wll_Event *, int, int, char **);
static void edg_wll_SortPEvents(edg_wll_Event **);

static int lb_query(void *fpctx, void *handle, const char *attr, glite_jp_attrval_t **attrval);
static int lb_open(void *fpctx, void *bhandle, const char *uri, void **handle);
static int lb_close(void *fpctx, void *handle);
static int lb_filecom(void *fpctx, void *handle);
static int lb_status(void *handle);
static int readline( 
        glite_jp_context_t ctx,
        void *handle,
        rl_buffer_t *buffer,
        char **line
);
char* get_namespace(const char* attr);

static int lb_StringToAttr(const char *name) {
        unsigned int    i;

        for (i=1; i<sizeof(lb_attrNames)/sizeof(lb_attrNames[0]); i++)
                if ( (lb_attrNames[i]) && (strcasecmp(lb_attrNames[i],name) == 0)) return i;
        for (i=1; i<sizeof(lb_attrNames2)/sizeof(lb_attrNames2[0]); i++)
                if ( (lb_attrNames2[i]) && (strcasecmp(lb_attrNames2[i],name) == 0)) return i+ATTRS2_OFFSET;
        return attr_UNDEF;
}

/* XXX: probably not needed 
static char *lb_AttrToString(int attr) {
        if (attr >= 0 && attr < sizeof(lb_attrNames)/sizeof(lb_attrNames[0])) return strdup(lb_attrNames[attr]);
        if (attr >= ATTRS2_OFFSET && attr < ATTRS2_OFFSET + sizeof(lb_attrNames2)/sizeof(lb_attrNames2[0])) return strdup(lb_attrNames2[attr]);
        return NULL;
}
*/

static int lb_dummy(void *fpctx, void *handle, int oper, ...) {
	puts("lb_dummy() - generic call not used; for testing purposes only...");
	return -1;
}

int init(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data) {

	data->fpctx = ctx;

	data->uris = calloc(2,sizeof *data->uris);
	data->uris[0] = strdup(GLITE_JP_FILETYPE_LB);

	data->classes = calloc(2,sizeof *data->classes);
	data->classes[0] = strdup("lb");

	data->namespaces = calloc(4, sizeof *data->namespaces);
	data->namespaces[0] = strdup(GLITE_JP_LB_NS);
	data->namespaces[1] = strdup(GLITE_JP_LB_JDL_NS);
	data->namespaces[2] = strdup(GLITE_JP_LBTAG_NS);

	data->ops.open 	= lb_open;
	data->ops.close = lb_close;
	data->ops.filecom = lb_filecom;
	data->ops.attr 	= lb_query;
	data->ops.generic = lb_dummy;
	
#ifdef PLUGIN_DEBUG
	fprintf(stderr,"lb_plugin: init OK\n");
#endif
	return 0;
}


void done(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data) {
	free(data->uris[0]);
	free(data->classes[0]);
	free(data->namespaces[0]);
	free(data->namespaces[1]);
	free(data->namespaces[2]);
	free(data->uris);
	free(data->classes);
	free(data->namespaces);
	memset(data, 0, sizeof(*data));
}


static int lb_open(void *fpctx, void *bhandle, const char *uri, void **handle) {

	lb_handle           *h;
	rl_buffer_t         buffer;
	glite_jp_context_t  ctx = (glite_jp_context_t) fpctx;
	char *line;
	int retval;
	edg_wll_Context     context;
	int                 nevents, maxnevents, i;
	glite_jp_error_t    err;
	char                *id0 = NULL,*id = NULL;
	
	glite_jp_clear_error(ctx);
	h = calloc(1, sizeof(lb_handle));

	if ((retval = edg_wll_InitContext(&context)) != 0) return retval;

	// read the file given by bhandle
	// parse events into h->events array
	memset(&buffer, 0, sizeof(buffer));
	buffer.buf = malloc(BUFSIZ);
	maxnevents = INITIAL_NUMBER_EVENTS;
	nevents = 0;
	h->events = malloc(maxnevents * sizeof(edg_wll_Event *));

	if ((retval = readline(ctx, bhandle, &buffer, &line)) != 0) {
		err.code = retval;
		err.desc = "reading LB logline";
		err.source = "lb_plugin.c:read_line()";
		glite_jp_stack_error(ctx,&err);
		goto fail;
	}
	while (line) {
#ifdef PLUGIN_DEBUG
		//fprintf(stderr,"lb_plugin opened\n", line);
#endif

		if (line[0]) {
			if (nevents >= maxnevents) {
				maxnevents <<= 1;
				h->events = realloc(h->events, maxnevents * sizeof(edg_wll_Event *));
			}
			if ((retval = edg_wll_ParseEvent(context, line, &h->events[nevents])) != 0) {
				char 	*ed;
				free(line);
				err.code = retval;
				edg_wll_Error(context,NULL,&ed);
				err.desc = ed;
				err.source = "edg_wll_ParseEvent()";
				glite_jp_stack_error(ctx,&err);
				free(ed);
				goto fail;
			}
			if (nevents == 0) {
				id0 = edg_wlc_JobIdGetUnique(h->events[nevents]->any.jobId );
			} else {
				id = edg_wlc_JobIdGetUnique(h->events[nevents]->any.jobId );
				if (strcmp(id0,id) != 0) {
					char et[BUFSIZ];
					retval = EINVAL;
					err.code = retval;
					snprintf(et,sizeof et,"Attempt to process different jobs. Id '%s' (event n.%d) differs from '%s'",id,nevents+1,id0);
					et[BUFSIZ-1] = 0;
					err.desc = et;
					err.source = "lb_plugin.c:edg_wlc_JobIdGetUnique()";
					glite_jp_stack_error(ctx,&err);
					goto fail;
				}
			}

			if (id) free(id); id = NULL;
			nevents++;
		}
		free(line);

		if ((retval = readline(ctx, bhandle, &buffer, &line)) != 0) {
			err.code = retval;
			err.desc = "reading LB logline";
			err.source = "lb_plugin.c:read_line()";
			glite_jp_stack_error(ctx,&err);
			goto fail;
		}
	}
	free(line);

	free(buffer.buf);
	edg_wll_FreeContext(context);

	if (nevents >= maxnevents) {
		maxnevents <<= 1;
		h->events = realloc(h->events, maxnevents * sizeof(edg_wll_Event *));
	}
	h->events[nevents] = NULL;

#ifdef PLUGIN_DEBUG
	fprintf(stderr,"lb_plugin: opened %d events\n", nevents);
#endif

	// find classad plugin, if it is loaded
	int j;
        h->classad_plugin = NULL;
        for (i=0; ctx->plugins[i]; i++){
		glite_jpps_fplug_data_t *pd = ctx->plugins[i];
                if (pd->namespaces)
                        for (j=0; pd->classes[j]; j++)
                                if (! strcmp(pd->classes[j], "classad")){
                                        h->classad_plugin = pd;
                                        goto cont;
                                }
	}
cont:

	/* count state and status history of the job given by the loaded events */
	if ((retval = lb_status(h)) != 0) goto fail;

	*handle = (void *)h;

	return 0;

fail:
#ifdef PLUGIN_DEBUG
	fprintf(stderr,"lb_plugin: open ERROR\n");
#endif
	for (i = 0; i < nevents; i++) {
		edg_wll_FreeEvent(h->events[i]);
		free(h->events[i]);
	}
	free(h->events);
	free(buffer.buf);
	if (id0) free(id0);
	if (id) free(id);
	edg_wll_FreeContext(context);
	free(h);
	*handle = NULL;
	err.code = EIO;
	err.desc = NULL;
	err.source = __FUNCTION__;
	glite_jp_stack_error(ctx,&err);

	return retval;
}


static int lb_close(void *fpctx,void *handle) {

	lb_handle           *h = (lb_handle *) handle;
	int                 i;
	
	// Free allocated stuctures
	if (h->events) {
		i = 0;
		while (h->events[i]) {
			edg_wll_FreeEvent(h->events[i]);
			free(h->events[i]);
			i++;
		}
		free(h->events);
	}

	if (h->status.state != EDG_WLL_JOB_UNDEF) 
		edg_wll_FreeStatus(&h->status);

	if (h->fullStatusHistory) {
		i = 0;
		while  (h->fullStatusHistory[i]) {
			free(h->fullStatusHistory[i]->reason);
			free(h->fullStatusHistory[i]->destination);
			free (h->fullStatusHistory[i]);
			i++;
		}
		h->fullStatusHistory = NULL;
		h->lastStatusHistory = NULL;
		h->finalStatus = NULL;
	}

	free(h);

#ifdef PLUGIN_DEBUG
	fprintf(stderr,"lb_plugin: close OK\n");
#endif
	return 0;
}

static int get_classad_attr(const char* attr, glite_jp_context_t ctx, lb_handle *h, glite_jp_attrval_t **av){
/*	printf("attr = %s\n", attr); */
	glite_jp_error_t err;
	glite_jp_clear_error(ctx);
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if (! h->classad_plugin){
        	err.code = ENOENT;
        	err.desc = strdup("Classad plugin has not been loaded.");
                return glite_jp_stack_error(ctx,&err);
	}
        // Get the attribute from JDL
        int i = 0;
        while (h->events[i]){
        	if (h->events[i]->type == EDG_WLL_EVENT_REGJOB
			&& h->events[i]->regJob.jdl 
			&& h->events[i]->regJob.jdl[0])
		{
                	void *beh;
                        if (! h->classad_plugin->ops.open_str(h->classad_plugin->fpctx, h->events[i]->regJob.jdl, "", "", &beh)){
                        	if (! h->classad_plugin->ops.attr(h->classad_plugin->fpctx, beh, attr, av))
                                	(*av)[0].timestamp = h->events[i]->any.timestamp.tv_sec;
                                else{
                                	h->classad_plugin->ops.close(h->classad_plugin->fpctx, beh);
                                        err.code = ENOENT;
                                        err.desc = strdup("Classad attribute not found.");
                                        return glite_jp_stack_error(ctx,&err);
                                }
                        h->classad_plugin->ops.close(h->classad_plugin->fpctx, beh);
                        }
                }
                i++;
        }
	return 0;
}

static int lb_query(void *fpctx,void *handle, const char *attr,glite_jp_attrval_t **attrval) {
	lb_handle		*h = (lb_handle *) handle;
	glite_jp_context_t	ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t 	err; 
	glite_jp_attrval_t	*av = NULL;
	int			i, j, n_tags;
	char 			*ns = get_namespace(attr); 
	char			*tag;
	int			aa = lb_StringToAttr(attr);

        glite_jp_clear_error(ctx); 
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if ((h->events == NULL) || 
	    (h->status.state == EDG_WLL_JOB_UNDEF) ||
	    (h->fullStatusHistory == NULL) ) {
                *attrval = NULL;
                err.code = ENOENT;
		err.desc = strdup("There is no job information to query.");
                return glite_jp_stack_error(ctx,&err);
	}


        if (strcmp(ns, GLITE_JP_LB_JDL_NS) == 0) {
		if (get_classad_attr(attr, ctx, h, &av)){
			*attrval = NULL;
                        err.code = ENOENT;
                        err.desc = strdup("Cannot get attribute from classad.");
			free(ns);
                        return glite_jp_stack_error(ctx,&err);
		}
	} else if (strcmp(ns, GLITE_JP_LBTAG_NS) == 0) {
		tag = strrchr(attr, ':');
                if (h->events && tag) {
                	tag++;
			i = 0;
			n_tags = 0;

			while (h->events[i]) {
				if ((h->events[i]->type == EDG_WLL_EVENT_USERTAG) &&
				(strcasecmp(h->events[i]->userTag.name, tag) == 0) ) {
/* XXX: LB tag names are case-insensitive */
					av = realloc(av, (n_tags+2) * sizeof(glite_jp_attrval_t));
					memset(&av[n_tags], 0, 2 * sizeof(glite_jp_attrval_t));

					av[n_tags].name = strdup(attr);
					av[n_tags].value = check_strdup(h->events[i]->userTag.value);
					av[n_tags].timestamp = 
						h->events[i]->any.timestamp.tv_sec;
					av[n_tags].size = -1;

					n_tags++;
				}
				i++;
			}
		}
	} else if (strcmp(ns, GLITE_JP_LB_NS) == 0) {

	switch (aa) {

	case attr_user :
	case attr2_owner :
		if (h->status.owner) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.owner);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_jobId :
	case attr2_jobId :
		if (h->status.jobId) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = edg_wlc_JobIdUnparse(h->status.jobId);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_parent :
	case attr2_parentJob :
		if (h->status.parent_job) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = edg_wlc_JobIdUnparse(h->status.parent_job);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_VO :
		if (get_classad_attr(":VirtualOrganisation", ctx, h, &av)){
			printf("error");
                        *attrval = NULL;
                        err.code = ENOENT;
                        err.desc = strdup("Cannot get attribute from classad.");
			free(ns);
                        return glite_jp_stack_error(ctx,&err);
                }
	   break;

        case attr_eNodes :
		if (get_classad_attr(":max_nodes_running", ctx, h, &av)){
                        printf("error");
                        *attrval = NULL;
                        err.code = ENOENT;
                        err.desc = strdup("Cannot get attribute from classad.");
			free(ns);
                        return glite_jp_stack_error(ctx,&err);
                }
	   break;

        case attr_eProc :
		if (get_classad_attr(":NodeNumber", ctx, h, &av)){
                        printf("error");
                        *attrval = NULL;
                        err.code = ENOENT;
                        err.desc = strdup("Cannot get attribute from classad.");
			free(ns);
                        return glite_jp_stack_error(ctx,&err);
                }
	   break;

	case attr_RB :
	case attr2_networkServer :
		if (h->status.network_server) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.network_server);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_CE :
		if (h->status.destination) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.destination);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_host :
	case attr2_ceNode :
		if (h->status.ce_node) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.ce_node);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_UIHost :
                i = 0;
                while (h->events[i]) {
                        if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) {
                                if (h->events[i]->any.host) {
                                        av = calloc(2, sizeof(glite_jp_attrval_t));
                                        av[0].name = strdup(attr);
                                        av[0].value = strdup(h->events[i]->any.host);
                                        av[0].size = -1;
                                        av[0].timestamp = h->events[i]->any.timestamp.tv_sec;
                                }       
                                break;
                        }       
                        i++;    
                }       
	   break;

	case attr_CPUTime :
	case attr2_cpuTime :
		if (h->status.cpuTime) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			trio_asprintf(&av[0].value,"%d", h->status.cpuTime);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_finalStatus :
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		if (h->finalStatus) {
			av[0].value = edg_wll_StatToString(h->finalStatus->state);
			av[0].timestamp = h->finalStatus->timestamp.tv_sec;
		} else {
			av[0].value = edg_wll_StatToString(h->status.state);
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
		av[0].size = -1;
	   break;

	case attr_finalDoneStatus :
	case attr2_doneCode :
		/* XXX: should be a string */
		if (h->finalStatus && h->finalStatus->state == EDG_WLL_JOB_DONE) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);

			trio_asprintf(&av[0].value,"%d",h->status.done_code);
			av[0].timestamp = h->finalStatus->timestamp.tv_sec;
		}
		else {
			*attrval = NULL;
                        err.code = EINVAL;
                        err.desc = strdup("Final status is not Done");
                        return glite_jp_stack_error(ctx,&err);
		}
	   break;


	case attr_finalStatusDate : 
	case attr2_stateEnterTime :
	   {
                struct tm *t = NULL;
                if ( (h->finalStatus) &&
		     ((t = gmtime(&h->finalStatus->timestamp.tv_sec)) != NULL) ) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			/* dateTime format: yyyy-mm-ddThh:mm:ss.uuuuuu */
                        trio_asprintf(&av[0].value,"%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                                1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec,
				h->finalStatus->timestamp.tv_usec);
			av[0].size = -1;
			av[0].timestamp = h->finalStatus->timestamp.tv_sec;
                } else if ((t = gmtime(&h->status.lastUpdateTime.tv_sec)) != NULL) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			/* dateTime format: yyyy-mm-ddThh:mm:ss.uuuuuu */
                        trio_asprintf(&av[0].value,"%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                                1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec,
				h->status.lastUpdateTime.tv_usec);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
                }
	   }
	   break;

	case attr_finalStatusReason :
		if (h->finalStatus && h->finalStatus->reason) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->finalStatus->reason);
			av[0].size = -1;
			av[0].timestamp = h->finalStatus->timestamp.tv_sec;
		} else if (h->status.reason) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.reason);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_LRMSDoneStatus :
		i = 0;
		j = -1;
		while (h->events[i]) {
			if ( (h->events[i]->type == EDG_WLL_EVENT_DONE) && 
			     (h->events[i]->any.source == EDG_WLL_SOURCE_LRMS) )
				j = i;
			i++;
		}
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].size = -1;
		if ( j != -1) {
			av[0].value = edg_wll_DoneStatus_codeToString(h->events[j]->done.status_code);
			av[0].timestamp = h->events[j]->any.timestamp.tv_sec;
		} else {
			av[0].value = edg_wll_DoneStatus_codeToString(h->status.done_code);
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_LRMSStatusReason :
		i = 0;
		j = -1;
		while (h->events[i]) {
			if ( (h->events[i]->type == EDG_WLL_EVENT_DONE) && 
			     (h->events[i]->any.source == EDG_WLL_SOURCE_LRMS) )
				j = i;
			i++;
		}
		if ( ( j != -1) && (h->events[j]->done.reason) ) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->events[j]->done.reason);
			av[0].size = -1;
			av[0].timestamp = h->events[j]->any.timestamp.tv_sec;
		}
	   break;

	case attr_retryCount :
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		trio_asprintf(&av[0].value,"%d", h->status.resubmitted);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	   break;

	case attr_jobType :
	case attr2_jobtype :
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		switch (h->status.jobtype) {
			case EDG_WLL_STAT_SIMPLE:
				av[0].value = strdup("SIMPLE"); break;
			case EDG_WLL_STAT_DAG:
				av[0].value = strdup("DAG"); break;
			default:
				av[0].value = strdup("UNKNOWN"); break;
		}
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	   break;

	case attr_nsubjobs :
	case attr2_childrenNum :
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		trio_asprintf(&av[0].value,"%d", h->status.children_num);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	   break;

	case attr_subjobs :
	case attr2_children :
		if (h->status.children_num > 0) {
			char *val = NULL, *old_val;

			old_val = strdup ("");			
			for (i=0; i<h->status.children_num; i++) {
				trio_asprintf(&val,"%s\t\t<jobId>%s</jobId>\n",
					old_val, h->status.children[i] ? h->status.children[i] : "");
				if (old_val) free(old_val);
				old_val = val; val = NULL; 
			}
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = check_strdup(old_val);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		} else {
			char et[BUFSIZ];
			*attrval = NULL;
			err.code = ENOENT;
			snprintf(et,sizeof et,"Value unknown for attribute '%s', there are no subjobs.",attr);
			et[BUFSIZ-1] = 0;
			err.desc = et;
			return glite_jp_stack_error(ctx,&err);
		}
	   break;

	case attr_lastStatusHistory :
	   {
		int i,j;
		char *val, *old_val, *s_str, *t_str, *r_str;
                struct tm *t;

		val = s_str = t_str = r_str = NULL;
		old_val = strdup("");
		t = calloc(1, sizeof(*t));
		/* first record is Submitted - hopefully in fullStatusHistory[0] */
		if ((h->fullStatusHistory[0] && 
			(h->fullStatusHistory[0]->state == EDG_WLL_JOB_SUBMITTED)) ) {

			s_str = edg_wll_StatToString(h->fullStatusHistory[0]->state);
			for (j = 0; s_str[j]; j++) s_str[j] = toupper(s_str[j]);
			if (gmtime_r(&h->fullStatusHistory[0]->timestamp.tv_sec,t) != NULL) {
				/* dateTime format: yyyy-mm-ddThh:mm:ss.uuuuuu */
				trio_asprintf(&t_str,"timestamp=\"%04d-%02d-%02dT%02d:%02d:%02d.%06d\" ",
					1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec,
					h->fullStatusHistory[0]->timestamp.tv_usec);
			} 
			if (h->fullStatusHistory[0]->reason) {
				trio_asprintf(&r_str,"reason=\"%s\" ",h->fullStatusHistory[0]->reason);
			}
			trio_asprintf(&val,"%s\t\t<status xmlns=\"" GLITE_JP_LB_NS "\" name=\"%s\" %s%s/>\n", 
				old_val, s_str ? s_str : "", t_str ? t_str : "", r_str ? r_str : "");
			if (s_str) free(s_str);
			if (t_str) free(t_str);
			if (r_str) free(t_str);
			if (old_val) free(old_val);
			old_val = val; val = NULL;
		}
		/* and the rest is from last Waiting to the end - i.e. all lastStatusHistory[] */
		if (h->lastStatusHistory) {
			i = 0;
			while (h->lastStatusHistory[i]) {
				s_str = edg_wll_StatToString(h->lastStatusHistory[i]->state);
				for (j = 0; s_str[j]; j++) s_str[j] = toupper(s_str[j]);
				if (gmtime_r(&h->lastStatusHistory[i]->timestamp.tv_sec,t) != NULL) {
					/* dateTime format: yyyy-mm-ddThh:mm:ss.uuuuuu */
					trio_asprintf(&t_str,"timestamp=\"%04d-%02d-%02dT%02d:%02d:%02d.%06d\" ",
						1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
						t->tm_hour, t->tm_min, t->tm_sec,
						h->lastStatusHistory[i]->timestamp.tv_usec);
				} 
				if (h->lastStatusHistory[i]->reason) {
					trio_asprintf(&r_str,"reason=\"%s\" ",h->lastStatusHistory[i]->reason);
				}
				trio_asprintf(&val,"%s\t\t<status xmlns=\"" GLITE_JP_LB_NS "\" name=\"%s\" %s%s/>\n", 
					old_val, s_str ? s_str : "", t_str ? t_str : "", r_str ? r_str : "");
				if (s_str) free(s_str); s_str = NULL;
				if (t_str) free(t_str); t_str = NULL;
				if (r_str) free(r_str); r_str = NULL;
				if (old_val) free(old_val);
				old_val = val; val = NULL;
				i++;
			}
		}
		val = old_val; old_val = NULL;
		if (val) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(val);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
			free(val);
		}
	   }
	   break;

	case attr_fullStatusHistory :
	   {
		int i,j;
		char *val, *old_val, *s_str, *t_str, *r_str;
                struct tm *t;

		val = s_str = t_str = r_str = NULL;
		old_val = strdup("");
		t = calloc(1, sizeof(*t));
		i = 0;
		while (h->fullStatusHistory[i]) {
			int	state;
			
			s_str = edg_wll_StatToString(state = h->fullStatusHistory[i]->state);
			for (j = 0; s_str[j]; j++) s_str[j] = toupper(s_str[j]);
			if (gmtime_r(&h->fullStatusHistory[i]->timestamp.tv_sec,t) != NULL) {
				/* dateTime format: yyyy-mm-ddThh:mm:ss:uuuuuu */
				trio_asprintf(&t_str,"timestamp=\"%04d-%02d-%02dT%02d:%02d:%02d.%06d\" ",
					1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec,
					h->fullStatusHistory[i]->timestamp.tv_usec);
			} 
			if (h->fullStatusHistory[i]->reason) {
				trio_asprintf(&r_str,"reason=\"%|Xs\" ",h->fullStatusHistory[i]->reason);
			}

			if (h->fullStatusHistory[i]->destination && 
				state >= EDG_WLL_JOB_READY && 
				state <= EDG_WLL_JOB_DONE
			) {
				char	*aux;
				trio_asprintf(&aux,"%s destination=\"%|Xs\"",
						r_str ? r_str : "",
						h->fullStatusHistory[i]->destination);
				r_str = aux;
			}

			if (state == EDG_WLL_JOB_DONE) {
				char	*aux;
				trio_asprintf(&aux,"%s doneCode=\"%d\"",
						r_str ? r_str : "",
						h->fullStatusHistory[i]->done_code);
				r_str = aux;
			}

			trio_asprintf(&val,"%s\t\t<status xmlns=\"" GLITE_JP_LB_NS "\" name=\"%s\" %s%s/>\n", 
				old_val, s_str ? s_str : "", t_str ? t_str : "", r_str ? r_str : "");
			if (s_str) free(s_str); s_str = NULL;
			if (t_str) free(t_str); t_str = NULL;
			if (r_str) free(r_str); r_str = NULL;
			if (old_val) free(old_val);
			old_val = val; val = NULL;
			i++;
		}
		val = old_val; old_val = NULL;
		if (val) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(val);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
			free(val);
		}
	   }
	   break;

	case attr_JDL :
	case attr2_jdl :
		if (h->status.jdl) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.jdl);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	   break;

	case attr_aTag : /* have to be retrieved from JDL, but probably obsolete and not needed at all */
	case attr_rQType : /* have to be retrieved from JDL, but probably obsolete and not needed at all */
	case attr_eDuration : /* have to be retrieved from JDL, but probably obsolete and not needed at all */
	case attr_NProc : /* currently LB hasn't got the info */
	case attr_additionalReason : /* what is it? */

	case attr2_status :
	case attr2_seed :
	case attr2_childrenHist :
	case attr2_childrenStates :
	case attr2_condorId :
	case attr2_globusId :
	case attr2_localId :
	case attr2_matchedJdl :
	case attr2_destination :
	case attr2_condorJdl :
	case attr2_rsl :
	case attr2_reason :
	case attr2_location :
	case attr2_subjobFailed :
	case attr2_exitCode :
	case attr2_resubmitted :
	case attr2_cancelling :
	case attr2_cancelReason :
	case attr2_userTags :
	case attr2_lastUpdateTime :
	case attr2_stateEnterTimes :
	case attr2_expectUpdate :
	case attr2_expectFrom :
	case attr2_acl :
	case attr2_payloadRunning :
	case attr2_possibleDestinations :
	case attr2_possibleCeNodes :
	case attr2_suspended :
	case attr2_suspendReason :
	case attr2_failureReasons :
	case attr2_removeFromProxy :
	case attr2_pbsState :
	case attr2_pbsQueue :
	case attr2_pbsOwner :
	case attr2_pbsName :
	case attr2_pbsReason :
	case attr2_pbsScheduler :
	case attr2_pbsDestHost :
	case attr2_pbsPid :
	case attr2_pbsResourceUsage :
	case attr2_pbsExitStatus :
	case attr2_pbsErrorDesc :
	case attr2_condorStatus :
	case attr2_condorUniverse :
	case attr2_condorOwner :
	case attr2_condorPreempting :
	case attr2_condorShadowPid :
	case attr2_condorShadowExitStatus :
	case attr2_condorStarterPid :
	case attr2_condorStarterExitStatus :
	case attr2_condorJobPid :
	case attr2_condorJobExitStatus :
	case attr2_condorDestHost :
	case attr2_condorReason :
	case attr2_condorErrorDesc :
	   {
		char et[BUFSIZ];
		*attrval = NULL;
		err.code = ENOSYS;
		snprintf(et,sizeof et,"Attribute '%s' not implemented yet.",attr);
		et[BUFSIZ-1] = 0;
		err.desc = et;
		return glite_jp_stack_error(ctx,&err);
	   }
	   break;

	case attr_UNDEF :
	case attr2_UNDEF :
	default:
	   {
		char et[BUFSIZ];
		*attrval = NULL;
		err.code = ENOSYS;
		snprintf(et,sizeof et,"Unknown attribute '%s' (in the '%s' namespace).",attr,ns);
		et[BUFSIZ-1] = 0;
		err.desc = et;
		return glite_jp_stack_error(ctx,&err);
	   }

	   break;
	} /* switch */

	} else {
		char et[BUFSIZ];
		*attrval = NULL;
        	err.code = EINVAL;
		snprintf(et,sizeof et,"No such attribute '%s' or namespace '%s'.",attr,ns);
		et[BUFSIZ-1] = 0;
		err.desc = et;
	        return glite_jp_stack_error(ctx,&err);
	}

	free(ns);

	if (av && av[0].value) {
		for (i=0; av[i].name; i++) av[i].origin = GLITE_JP_ATTR_ORIG_FILE;
		*attrval = av;
		return 0;
	} else {
		char et[BUFSIZ];
		*attrval = NULL;
		err.code = ENOENT;
		snprintf(et,sizeof et,"Value unknown for attribute '%s'.",attr);
		et[BUFSIZ-1] = 0;
		err.desc = et;
		if (av) glite_jp_attrval_free(av,1); // XXX: probably not needed
		return glite_jp_stack_error(ctx,&err);
	}
}

static int lb_filecom(void *fpctx, void *handle){
	glite_jp_context_t      ctx = (glite_jp_context_t) fpctx;
	lb_handle               *h = (lb_handle *) handle;
	glite_jp_attrval_t      attr[2];
	memset(attr, 0, 2 * sizeof(glite_jp_attrval_t));

	if (h->events) {
        	int i = 0;
                while (h->events[i]) {
                	if (h->events[i]->type == EDG_WLL_EVENT_USERTAG &&
                        strchr(h->events[i]->userTag.name,':')) 
                        {
				//printf("%s, %s\n", edg_wlc_JobIdUnparse(h->status.jobId), h->status.jobId);
				attr[0].name = h->events[i]->userTag.name;
                		attr[0].value = h->events[i]->userTag.value;
                		attr[0].binary = 0;
        			attr[0].origin = GLITE_JP_ATTR_ORIG_USER;
        			attr[0].timestamp = time(NULL);
        			attr[0].origin_detail = NULL;   /* XXX */
        			attr[1].name = NULL;
				glite_jppsbe_append_tag(ctx, edg_wlc_JobIdUnparse(h->status.jobId), attr);
                        }
                        i++;
                }
	}

	return 0;
}

static int lb_status(void *handle) {

	lb_handle       *h = (lb_handle *) handle;
        intJobStat	*js;
        int		maxnstates, nstates, i, be_strict = 0, retval;
	char		*errstring;
	edg_wll_JobStatCode old_state = EDG_WLL_JOB_UNDEF;
        
        js = calloc(1, sizeof(intJobStat));
	init_intJobStat(js);

	edg_wll_SortPEvents(h->events);

	maxnstates = INITIAL_NUMBER_STATES;
	nstates = 0;
	h->fullStatusHistory = calloc(maxnstates, sizeof(lb_historyStatus *));
	h->lastStatusHistory = NULL;
	h->finalStatus = NULL;
	i = 0;
        while (h->events[i])  
        {
		/* realloc the fullStatusHistory if needed */
		if (nstates >= maxnstates) {
			maxnstates <<= 1;
			h->fullStatusHistory = realloc(h->fullStatusHistory, maxnstates * sizeof(lb_historyStatus *));
		}

		/* job owner and jobId not filled from events normally */
		if (h->events[i]->any.type == EDG_WLL_EVENT_REGJOB) {
			js->pub.owner = check_strdup(h->events[i]->any.user);
			if (edg_wlc_JobIdDup(h->events[i]->any.jobId,&js->pub.jobId)) {
				goto err;
			}
		}
		/* Process Event and update the state */
		if (processEvent(js, h->events[i], 0, be_strict, &errstring) == RET_FATAL) {
			goto err;
		}

		/* if the state has changed, update the status history */
		if (js->pub.state != old_state) {
			h->fullStatusHistory[nstates] = calloc(1,sizeof(lb_historyStatus));
			h->fullStatusHistory[nstates]->state = js->pub.state;
			h->fullStatusHistory[nstates]->timestamp.tv_sec = js->pub.stateEnterTime.tv_sec;
			h->fullStatusHistory[nstates]->timestamp.tv_usec = js->pub.stateEnterTime.tv_usec;
			h->fullStatusHistory[nstates]->reason = check_strdup(js->pub.reason);		
			/* lastStatusHistory starts from the last WAITING state */
			if (js->pub.state == EDG_WLL_JOB_WAITING) {
				h->lastStatusHistory = &(h->fullStatusHistory[nstates]);
			}
			/* finalStatus is the one preceeding the CLEARED state */
			if ( (js->pub.state == EDG_WLL_JOB_CLEARED) && (nstates > 0) ) {
				h->finalStatus = h->fullStatusHistory[nstates-1];
			}

			h->fullStatusHistory[nstates]->destination = check_strdup(js->pub.destination);
			h->fullStatusHistory[nstates]->done_code = js->pub.done_code;

			old_state = js->pub.state;
			nstates++;
		}

		i++;
	}
	h->fullStatusHistory[nstates] = NULL;
	/* if there is no CLEARED state, finalStatus is just the last status 
	   and if there is no such thing, leave h->finalStatus NULL and for the attribute 
           try to read something from the h->status */
	if ( (h->finalStatus == NULL) && (nstates > 0) ) {
		h->finalStatus = h->fullStatusHistory[nstates-1];
	}

	/* fill in also subjobs */
	if (js->pub.children_num > 0) {	
		edg_wll_Context context;
		edg_wlc_JobId *subjobs;

		if ((retval = edg_wll_InitContext(&context)) != 0) return retval;
		subjobs = calloc(js->pub.children_num, sizeof (*subjobs));
		if ((retval = edg_wll_GenerateSubjobIds(context, 
			js->pub.jobId, js->pub.children_num, js->pub.seed, &subjobs) ) != 0 ) {
			goto err;
		}
		js->pub.children = calloc(js->pub.children_num + 1, sizeof (*js->pub.children));
		for (i=0; i<js->pub.children_num; i++) {
			js->pub.children[i] = edg_wlc_JobIdUnparse(subjobs[i]);
		}
		edg_wll_FreeContext(context);
		free(subjobs);
	}

	memcpy(&h->status, &js->pub, sizeof(edg_wll_JobStat));

	// not very clean, but working
	memset(&js->pub, 0, sizeof(edg_wll_JobStat));
	destroy_intJobStat(js);

	return 0;

err:
	destroy_intJobStat(js);
	return -1;
}


/*
 * realloc the line to double size if needed
 *
 * \return 0 if failed, did nothing
 * \return 1 if success
 */
/*int check_realloc_line(char **line, size_t *maxlen, size_t len) {
	void *tmp;

	if (len > *maxlen) {
		*maxlen <<= 1;
		tmp = realloc(*line, *maxlen);
		if (!tmp) return 0;
		*line = tmp;
	}

	return 1;
}
*/

/*
 * read next line from stream
 *
 * \return error code
 */
/*static int read_line(glite_jp_context_t ctx, void *handle, lb_buffer_t *buffer, char **line) {

	size_t maxlen, len, i;
	ssize_t nbytes;
	int retval, z, end;

	maxlen = BUFSIZ;
	i = 0;
	len = 0;
	*line = malloc(maxlen);
	end = 0;

	do {
		// read next portion 
		if (buffer->pos >= buffer->size) {
			buffer->pos = 0;
			buffer->size = 0;
			if ((retval = glite_jppsbe_pread(ctx, handle, buffer->buf, BUFSIZ, buffer->offset, &nbytes)) == 0) {
				if (nbytes < 0) {
					retval = EINVAL;
					goto fail;
				} else {
					if (nbytes) {
						buffer->size = (size_t)nbytes;
						buffer->offset += nbytes;
					} else end = 1;
				}
			} else goto fail;
		}

		// we have buffer->size - buffer->pos bytes 
		i = buffer->pos;
		do {
			if (i >= buffer->size) z = '\0';
			else {
				z = buffer->buf[i];
				if (z == '\n') z = '\0';
			}
			len++;
			
			if (!check_realloc_line(line, &maxlen, len)) {
				retval = ENOMEM;
				goto fail;
			}
			(*line)[len - 1] = z;
			i++;
		} while (z && i < buffer->size);
		buffer->pos = i;
	} while (len && (*line)[len - 1] != '\0');

	if ((!len || !(*line)[0]) && end) {
		free(*line);
		*line = NULL;
	}

	return 0;

fail:
	free(*line);
	*line = NULL;
	return retval;
}
*/


static int compare_pevents_by_seq(const void *a, const void *b)
{
        const edg_wll_Event **e = (const edg_wll_Event **) a;
        const edg_wll_Event **f = (const edg_wll_Event **) b;
	return compare_events_by_seq(*e,*f);
}


static void edg_wll_SortPEvents(edg_wll_Event **e)
{
	edg_wll_Event **p;
	int	n;

	if (!e) return;
	p = e;
	for (n=0; *p; n++) {
		p++;
	}
	qsort(e,n,sizeof(*e),compare_pevents_by_seq);
}

int check_realloc_line(char **line, size_t *maxlen, size_t len) {
        void *tmp;

        if (len > *maxlen) {
                *maxlen <<= 1;
                tmp = realloc(*line, *maxlen);
                if (!tmp) return 0;
                *line = tmp;
        }

        return 1;
}

int readline(
        glite_jp_context_t ctx,
        void *handle,
        rl_buffer_t *buffer,
        char **line
)
{
        size_t maxlen, len, i;
        ssize_t nbytes;
        int retval, z, end;

        maxlen = BUFSIZ;
        i = 0;
        len = 0;
        *line = malloc(maxlen);
        end = 0;

        do {
                /* read next portion */
                if (buffer->pos >= buffer->size) {
                        buffer->pos = 0;
                        buffer->size = 0;
                        if ((retval = glite_jppsbe_pread(ctx, handle, buffer->buf, BUFSIZ, buffer->offset, &nbytes)) == 0) {
                                if (nbytes < 0) {
                                        retval = EINVAL;
                                        goto fail;
                                } else {
                                        if (nbytes) {
                                                buffer->size = (size_t)nbytes;
                                                buffer->offset += nbytes;
                                        } else end = 1;
                                }
                        } else goto fail;
                }

                /* we have buffer->size - buffer->pos bytes */
                i = buffer->pos;
                do {
                        if (i >= buffer->size) z = '\0';
                        else {
                                z = buffer->buf[i];
                                if (z == '\n') z = '\0';
                        }
                        len++;

                        if (!check_realloc_line(line, &maxlen, len)) {
                                retval = ENOMEM;
                                goto fail;
                        }
                        (*line)[len - 1] = z;
                        i++;
                } while (z && i < buffer->size);
                buffer->pos = i;
        } while (len && (*line)[len - 1] != '\0');

        if ((!len || !(*line)[0]) && end) {
                free(*line);
                *line = NULL;
        }

        return 0;

fail:
        free(*line);
        *line = NULL;
        return retval;
}

char* get_namespace(const char* attr){
        char* namespace = strdup(attr);
        char* colon = strrchr(namespace, ':');
        if (colon)
                namespace[strrchr(namespace, ':') - namespace] = 0;
        else
                namespace[0] = 0;
        return namespace;
}

