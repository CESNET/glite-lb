#ident "$Header: "

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <cclassad.h>

#include "glite/lb/context.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/trio.h"

#include "jobstat.h"
#include "get_events.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/builtin_plugins.h"
#include "glite/jp/backend.h"
#include "glite/jp/attr.h"
#include "glite/jp/known_attr.h"
#include "jp_job_attrs.h"

#define INITIAL_NUMBER_EVENTS 100
#define INITIAL_NUMBER_STATES EDG_WLL_NUMBER_OF_STATCODES
#define LB_PLUGIN_NAMESPACE "urn:org.glite.lb"

typedef struct _lb_buffer_t {
	char 			*buf;
	size_t 			pos, size;
	off_t 			offset;
} lb_buffer_t;

typedef struct _lb_historyStatus {
	edg_wll_JobStatCode 	state;
	struct timeval 		timestamp;
	char 			*reason;
} lb_historyStatus;

typedef struct _lb_handle {
	edg_wll_Event		**events;
	edg_wll_JobStat		status;
	lb_historyStatus	**fullStatusHistory, **lastStatusHistory;
} lb_handle;

#define check_strdup(s) ((s) ? strdup(s) : NULL)

extern int processEvent(intJobStat *, edg_wll_Event *, int, int, char **);

static int lb_query(void *fpctx, void *handle, const char *attr, glite_jp_attrval_t **attrval);
static int lb_open(void *fpctx, void *bhandle, const char *uri, void **handle);
static int lb_close(void *fpctx, void *handle);
static int lb_status(void *handle);
static int read_line(glite_jp_context_t ctx, void *handle, lb_buffer_t *buffer, char **line);

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

	data->ops.open 	= lb_open;
	data->ops.close = lb_close;
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
	free(data->uris);
	free(data->classes);
	memset(data, 0, sizeof(*data));
}


static int lb_open(void *fpctx, void *bhandle, const char *uri, void **handle) {

	lb_handle           *h;
	lb_buffer_t         buffer;
	glite_jp_context_t  ctx = (glite_jp_context_t) fpctx;
	char *line;
	int retval;
	edg_wll_Context     context;
	int              nevents, maxnevents, i;
	glite_jp_error_t	err;
	
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

	if ((retval = read_line(ctx, bhandle, &buffer, &line)) != 0) {
		err.code = retval;
		err.desc = "reading LB logline";
		err.source = "lb_plugin.c:read_line()";
		glite_jp_stack_error(ctx,&err);
		goto fail;
	}
	while (line) {
#ifdef PLUGIN_DEBUG
//		fprintf(stderr,"lb_plugin: line read '%s'\n", line);
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
			nevents++;
		}
		free(line);

		if ((retval = read_line(ctx, bhandle, &buffer, &line)) != 0) {
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

	/* count state and status hiftory of the job given by the loaded events */
	if ((retval = lb_status(h)) != 0) goto fail;

	*handle = (void *)h;

	return 0;

fail:
	for (i = 0; i < nevents; i++) {
		edg_wll_FreeEvent(h->events[i]);
		free(h->events[i]);
	}
	free(h->events);
	free(buffer.buf);
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
			if (h->fullStatusHistory[i]->reason) free(h->fullStatusHistory[i]->reason);
			free (h->fullStatusHistory[i]);
			i++;
		}
	}

	free(h);

#ifdef PLUGIN_DEBUG
	fprintf(stderr,"lb_plugin: close OK\n");
#endif
	return 0;
}


static int lb_query(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval) {

	lb_handle		*h = (lb_handle *) handle;
	glite_jp_context_t	ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t 	err; 
	glite_jp_attrval_t	*av = NULL;
	int			i, n_tags;
	const char             *tag;

        glite_jp_clear_error(ctx); 
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if ((h->events == NULL) || 
	    (h->status.state == EDG_WLL_JOB_UNDEF) ||
	    (h->fullStatusHistory == NULL) ) {
                *attrval = NULL;
                err.code = ENOENT;
                trio_asprintf(&err.desc,"There is no job information to query.");
                return glite_jp_stack_error(ctx,&err);
	}

        if (strcmp(attr, GLITE_JP_LB_user) == 0) {
		if (h->status.owner) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.owner);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_jobId) == 0) {
		if (h->status.jobId) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = edg_wlc_JobIdUnparse(h->status.jobId);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_VO) == 0) {
		i = 0;
		while (h->events[i]) {
			if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) {
				struct cclassad *ad;
				char *string_vo = NULL; 

				ad = cclassad_create(h->events[i]->regJob.jdl);
				if (ad) {
					if (cclassad_evaluate_to_string(ad, "VirtualOrganisation", &string_vo)) {
						av = calloc(2, sizeof(glite_jp_attrval_t));
						av[0].name = strdup(attr);
						av[0].value = check_strdup(string_vo);
						av[0].timestamp = h->events[i]->any.timestamp.tv_sec;
					}
					cclassad_delete(ad);
					if (string_vo) free(string_vo);
				}
				break;
			}
			i++;
		}
        } else if (strcmp(attr, GLITE_JP_LB_eNodes) == 0) {
		i = 0;
		while (h->events[i]) {
			if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) {
				struct cclassad *ad;
				char *string_nodes = NULL; 

				ad = cclassad_create(h->events[i]->regJob.jdl);
				if (ad) {
					if (cclassad_evaluate_to_string(ad, "max_nodes_running", &string_nodes)) {
						av = calloc(2, sizeof(glite_jp_attrval_t));
						av[0].name = strdup(attr);
						av[0].value = check_strdup(string_nodes);
						av[0].timestamp = h->events[i]->any.timestamp.tv_sec;
					}
					cclassad_delete(ad);
					if (string_nodes) free(string_nodes);
				}
				break;
			}
			i++;
		}
        } else if (strcmp(attr, GLITE_JP_LB_eProc) == 0) {
		i = 0;
		while (h->events[i]) {
			if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) {
				struct cclassad *ad;
				char *string_nodes = NULL; 

				ad = cclassad_create(h->events[i]->regJob.jdl);
				if (ad) {
					if (cclassad_evaluate_to_string(ad, "NodeNumber", &string_nodes)) {
						av = calloc(2, sizeof(glite_jp_attrval_t));
						av[0].name = strdup(attr);
						av[0].value = check_strdup(string_nodes);
						av[0].timestamp = h->events[i]->any.timestamp.tv_sec;
					}
					cclassad_delete(ad);
					if (string_nodes) free(string_nodes);
				}
				break;
			}
			i++;
		}
	} else if (strcmp(attr, GLITE_JP_LB_aTag) == 0 ||
                   strcmp(attr, GLITE_JP_LB_rQType) == 0 ||
                   strcmp(attr, GLITE_JP_LB_eDuration) == 0) {
		/* have to be retrieved from JDL, but probably obsolete and not needed at all */
		*attrval = NULL;
		err.code = ENOSYS;
		trio_asprintf(&err.desc,"Attribute '%s' not implemented yet.",attr);
		return glite_jp_stack_error(ctx,&err);
	} else if (strcmp(attr, GLITE_JP_LB_RB) == 0) {
		if (h->status.network_server) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.network_server);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_CE) == 0) {
		if (h->status.destination) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.destination);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_host) == 0) {
		if (h->status.ce_node) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.ce_node);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_UIHost) == 0) {
		if (h->status.location) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.location);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_CPUTime) == 0) {
		if (h->status.cpuTime) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			trio_asprintf(&av[0].value,"%d", h->status.cpuTime);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_NProc) == 0) {
		/* currently LB hasn't got the info */
		*attrval = NULL;
		err.code = ENOSYS;
		trio_asprintf(&err.desc,"Attribute '%s' not implemented yet.",attr);
		return glite_jp_stack_error(ctx,&err);
	} else if (strcmp(attr, GLITE_JP_LB_finalStatus) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].value = edg_wll_StatToString(h->status.state);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_finalStatusDate) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
                trio_asprintf(&av[0].value,"%ld.%06ld",  
			h->status.lastUpdateTime.tv_sec,
			h->status.lastUpdateTime.tv_usec);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_finalStatusReason) == 0) {
		if (h->status.reason) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.reason);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_LRMSDoneStatus) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].value = edg_wll_DoneStatus_codeToString(h->status.done_code);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_LRMSStatusReason) == 0) {
		i = 0;
		while (h->events[i]) {
			if (h->events[i]->type == EDG_WLL_EVENT_DONE) {
				if (h->events[i]->done.reason) {
					av = calloc(2, sizeof(glite_jp_attrval_t));
					av[0].name = strdup(attr);
					av[0].value = strdup(h->events[i]->done.reason);
					av[0].size = -1;
					av[0].timestamp = h->events[i]->any.timestamp.tv_sec;
				}
				break;
			}
			i++;
		}
	} else if (strcmp(attr, GLITE_JP_LB_retryCount) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		trio_asprintf(&av[0].value,"%d", h->status.resubmitted);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_additionalReason) == 0) {
		/* what is it? */
		*attrval = NULL;
		err.code = ENOSYS;
		trio_asprintf(&err.desc,"Attribute '%s' not implemented yet.",attr);
		return glite_jp_stack_error(ctx,&err);
	} else if (strcmp(attr, GLITE_JP_LB_jobType) == 0) {
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
	} else if (strcmp(attr, GLITE_JP_LB_nsubjobs) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		trio_asprintf(&av[0].value,"%d", h->status.children_num);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_lastStatusHistory) == 0) {
/*
		av = calloc(1 + EDG_WLL_NUMBER_OF_STATCODES, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].value = check_strdup(h->status.reason);
		av[0].timestamp = h->status.stateEnterTime.tv_sec;
		av[0].size = -1;
		for (i=1; i<EDG_WLL_NUMBER_OF_STATCODES; i++) {
			av[i].name = strdup(attr);
			if (i == h->status.state) av[i].value = check_strdup(h->status.reason);
			av[i].timestamp = h->status.stateEnterTimes[i+1];
			av[i].size = -1;
		}
*/
                i = 0; while (h->lastStatusHistory[i]) i++;
                av = calloc(i+2, sizeof(glite_jp_attrval_t));
                av[0].name = strdup(attr);
                av[0].value = check_strdup(h->status.reason);
                av[0].timestamp = h->status.stateEnterTime.tv_sec;
                av[0].size = -1;
		if (h->fullStatusHistory[0]) {
			av[1].name = edg_wll_StatToString(h->fullStatusHistory[0]->state);
			av[1].value = check_strdup(h->fullStatusHistory[0]->reason);
			av[1].timestamp = h->fullStatusHistory[0]->timestamp.tv_sec;
			av[1].size = -1;	
		}
                i = 0;
                while (h->lastStatusHistory[i]) {
                        av[i+2].name = edg_wll_StatToString(h->lastStatusHistory[i]->state);
                        av[i+2].value = check_strdup(h->lastStatusHistory[i]->reason);
                        av[i+2].timestamp = h->lastStatusHistory[i]->timestamp.tv_sec;
                        av[i+2].size = -1;
                        i++;
                }
	} else if (strcmp(attr, GLITE_JP_LB_fullStatusHistory) == 0) {
		i = 0; while (h->fullStatusHistory[i]) i++;
		av = calloc(i+1, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].value = check_strdup(h->status.reason);
		av[0].timestamp = h->status.stateEnterTime.tv_sec;
		av[0].size = -1;
		i = 0;
		while (h->fullStatusHistory[i]) {
			av[i+1].name = edg_wll_StatToString(h->fullStatusHistory[i]->state);
			av[i+1].value = check_strdup(h->fullStatusHistory[i]->reason);
			av[i+1].timestamp = h->fullStatusHistory[i]->timestamp.tv_sec;
			av[i+1].size = -1;	
			i++;
		}
	} else if (strncmp(attr, GLITE_JP_LBTAG_NS, sizeof(GLITE_JP_LBTAG_NS)-1) == 0) {
		tag = strrchr(attr, ':');
		if (h->events && tag) {
			tag++;
			i = 0;
			n_tags = 0;

			while (h->events[i]) {
				if ((h->events[i]->type == EDG_WLL_EVENT_USERTAG) &&
				    (strcmp(h->events[i]->userTag.name, tag) == 0) ) {
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
	} else if (strcmp(attr, GLITE_JP_LB_JDL) == 0) {
                i = 0;
                while (h->events[i]) {
                        if ((h->events[i]->type == EDG_WLL_EVENT_REGJOB) &&
			    (h->events[i]->regJob.jdl) ) {
				av = calloc(2, sizeof(glite_jp_attrval_t));
				av[0].name = strdup(attr);
				av[0].value = strdup(h->events[i]->regJob.jdl);
				av[0].timestamp = h->events[i]->any.timestamp.tv_sec;
				av[0].size = -1;
				break;
			}
			i++;
		}
	} else {
		*attrval = NULL;
        	err.code = EINVAL;
		trio_asprintf(&err.desc,"No such attribute '%s'.",attr);
	        return glite_jp_stack_error(ctx,&err);
	}

	if (av && av[0].value) {
		for (i=0; av[i].name; i++) av[i].origin = GLITE_JP_ATTR_ORIG_FILE;
		*attrval = av;
		return 0;
	} else {
		*attrval = NULL;
		err.code = ENOENT;
		trio_asprintf(&err.desc,"Value unknown for attribute '%s'.",attr);
		if (av) glite_jp_attrval_free(av,1); // probably not needed
		return glite_jp_stack_error(ctx,&err);
	}
}


static int lb_status(void *handle) {

	lb_handle       *h = (lb_handle *) handle;
        intJobStat	*js;
        int		maxnstates, nstates, i, be_strict = 0;
	char		*errstring;
	edg_wll_JobStatCode old_state = EDG_WLL_JOB_UNDEF;
        
        js = calloc(1, sizeof(intJobStat));
	init_intJobStat(js);

	edg_wll_SortPEvents(h->events);

	maxnstates = INITIAL_NUMBER_STATES;
	nstates = 0;
	h->fullStatusHistory = calloc(maxnstates, sizeof(lb_historyStatus *));
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
			if (js->pub.state == EDG_WLL_JOB_WAITING) {
				h->lastStatusHistory = &h->fullStatusHistory[nstates];
			}
			old_state = js->pub.state;
			nstates++;
		}

		i++;
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


/*
 * read next line from stream
 *
 * \return error code
 */
static int read_line(glite_jp_context_t ctx, void *handle, lb_buffer_t *buffer, char **line) {

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

