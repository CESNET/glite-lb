#ident "$Header$"

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include <cclassad.h>

#include "glite/lb/context.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/producer.h"

#include "glite/lb-utils/trio.h"
#include "glite/lb-utils/cjobid.h"

#include "jobstat.h"
#include "get_events.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/builtin_plugins.h"
#include "glite/jp/backend.h"
#include "glite/jp/attr.h"
#include "glite/jp/known_attr.h"
#include "jp_job_attrs_native.h"
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
			if (nevents == 0) {
				id0 = glite_lbu_JobIdGetUnique(h->events[nevents]->any.jobId );
			} else {
				id = glite_lbu_JobIdGetUnique(h->events[nevents]->any.jobId );
				if (strcmp(id0,id) != 0) {
					char et[BUFSIZ];
					retval = EINVAL;
					err.code = retval;
					snprintf(et,sizeof et,"Attempt to process different jobs. Id '%s' (event n.%d) differs from '%s'",id,nevents+1,id0);
					et[BUFSIZ-1] = 0;
					err.desc = et;
					err.source = "lb_plugin.c:glite_lbu_JobIdGetUnique()";
					glite_jp_stack_error(ctx,&err);
					goto fail;
				}
			}

			if (id) free(id); id = NULL;
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
		err.desc = strdup("There is no job information to query.");
                return glite_jp_stack_error(ctx,&err);
	}

        if (strcmp(attr, GLITE_JP_LB_STAT_user) == 0) {
		if (h->status.owner) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.owner);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_jobId) == 0) {
		if (h->status.jobId) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = glite_lbu_JobIdUnparse(h->status.jobId);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_parent) == 0) {
		if (h->status.parent_job) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = glite_lbu_JobIdUnparse(h->status.parent_job);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_VO) == 0) {
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
        } else if (strcmp(attr, GLITE_JP_LB_STAT_eNodes) == 0) {
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
        } else if (strcmp(attr, GLITE_JP_LB_STAT_eProc) == 0) {
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
	} else if (strcmp(attr, GLITE_JP_LB_STAT_aTag) == 0 ||
                   strcmp(attr, GLITE_JP_LB_STAT_rQType) == 0 ||
                   strcmp(attr, GLITE_JP_LB_STAT_eDuration) == 0) {
		/* have to be retrieved from JDL, but probably obsolete and not needed at all */
		char et[BUFSIZ];
		*attrval = NULL;
		err.code = ENOSYS;
		snprintf(et,sizeof et,"Attribute '%s' not implemented yet.",attr);
		et[BUFSIZ-1] = 0;
		err.desc = et;
		return glite_jp_stack_error(ctx,&err);
	} else if (strcmp(attr, GLITE_JP_LB_STAT_RB) == 0) {
		if (h->status.network_server) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.network_server);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_CE) == 0 ||
		strcmp(attr, GLITE_JP_LB_destination) == 0)
	{
		if (h->status.destination) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.destination);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_host) == 0) {
		if (h->status.ce_node) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.ce_node);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_UIHost) == 0) {
		if (h->status.location) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.location);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_CPUTime) == 0) {
		if (h->status.cpuTime) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			trio_asprintf(&av[0].value,"%d", h->status.cpuTime);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_NProc) == 0) {
		/* currently LB hasn't got the info */
		char et[BUFSIZ];
		*attrval = NULL;
		err.code = ENOSYS;
		snprintf(et,sizeof et,"Attribute '%s' not implemented yet.",attr);
		et[BUFSIZ-1] = 0;
		err.desc = et;
		return glite_jp_stack_error(ctx,&err);
	} else if (strcmp(attr, GLITE_JP_LB_STAT_finalStatus) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].value = edg_wll_StatToString(h->status.state);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_STAT_finalStatusDate) == 0) {
                struct tm *t = NULL;
                if ((t = gmtime(&h->status.lastUpdateTime.tv_sec)) != NULL) {
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
	} else if (strcmp(attr, GLITE_JP_LB_STAT_finalStatusReason) == 0) {
		if (h->status.reason) {
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].name = strdup(attr);
			av[0].value = strdup(h->status.reason);
			av[0].size = -1;
			av[0].timestamp = h->status.lastUpdateTime.tv_sec;
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_LRMSDoneStatus) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		av[0].value = edg_wll_DoneStatus_codeToString(h->status.done_code);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_STAT_LRMSStatusReason) == 0) {
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
	} else if (strcmp(attr, GLITE_JP_LB_STAT_retryCount) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		trio_asprintf(&av[0].value,"%d", h->status.resubmitted);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_STAT_additionalReason) == 0) {
		/* what is it? */
		char et[BUFSIZ];
		*attrval = NULL;
		err.code = ENOSYS;
		snprintf(et,sizeof et,"Attribute '%s' not implemented yet.",attr);
		et[BUFSIZ-1] = 0;
		err.desc = et;
		return glite_jp_stack_error(ctx,&err);
	} else if (strcmp(attr, GLITE_JP_LB_STAT_jobType) == 0) {
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
	} else if (strcmp(attr, GLITE_JP_LB_STAT_nsubjobs) == 0) {
		av = calloc(2, sizeof(glite_jp_attrval_t));
		av[0].name = strdup(attr);
		trio_asprintf(&av[0].value,"%d", h->status.children_num);
		av[0].size = -1;
		av[0].timestamp = h->status.lastUpdateTime.tv_sec;
	} else if (strcmp(attr, GLITE_JP_LB_STAT_subjobs) == 0) {
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
			err.code = 0;
			snprintf(et,sizeof et,"Value unknown for attribute '%s', there are no subjobs.",attr);
			et[BUFSIZ-1] = 0;
			err.desc = et;
			return glite_jp_stack_error(ctx,&err);
		}
	} else if (strcmp(attr, GLITE_JP_LB_STAT_lastStatusHistory) == 0) {
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
			trio_asprintf(&val,"%s\t\t<status name=\"%s\" %s%s/>\n", 
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
				trio_asprintf(&val,"%s\t\t<status name=\"%s\" %s%s/>\n", 
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
	} else if (strcmp(attr, GLITE_JP_LB_STAT_fullStatusHistory) == 0) {
		int i,j;
		char *val, *old_val, *s_str, *t_str, *r_str;
                struct tm *t;

		val = s_str = t_str = r_str = NULL;
		old_val = strdup("");
		t = calloc(1, sizeof(*t));
		i = 0;
		while (h->fullStatusHistory[i]) {
			s_str = edg_wll_StatToString(h->fullStatusHistory[i]->state);
			for (j = 0; s_str[j]; j++) s_str[j] = toupper(s_str[j]);
			if (gmtime_r(&h->fullStatusHistory[i]->timestamp.tv_sec,t) != NULL) {
				/* dateTime format: yyyy-mm-ddThh:mm:ss:uuuuuu */
				trio_asprintf(&t_str,"timestamp=\"%04d-%02d-%02dT%02d:%02d:%02d.%06d\" ",
					1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec,
					h->fullStatusHistory[i]->timestamp.tv_usec);
			} 
			if (h->fullStatusHistory[i]->reason) {
				trio_asprintf(&r_str,"reason=\"%s\" ",h->fullStatusHistory[i]->reason);
			}
			trio_asprintf(&val,"%s\t\t<status name=\"%s\" %s%s/>\n", 
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
	} else if (strncmp(attr, GLITE_JP_LBTAG_NS, sizeof(GLITE_JP_LBTAG_NS)-1) == 0) {
		tag = strrchr(attr, ':');
		if (h->events && tag) {
			tag++;
			i = 0;
			n_tags = 0;

			while (h->events[i]) {
				if ((h->events[i]->type == EDG_WLL_EVENT_USERTAG) &&
/* XXX: LB tag names are case-insensitive */
				    (strcasecmp(h->events[i]->userTag.name, tag) == 0) ) {
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
	} else if (strcmp(attr, GLITE_JP_LB_STAT_JDL) == 0) {
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
@@@{
	selectType $status '_common_';
	for (getFieldsOrdered $status) {
		my $f = selectField $status $_;
		next unless $f->{special}->{JP};
		my $t = getType $f;
		my $n = getName $f;
		my $hn = $n;

		while ($hn =~ /(.*)_(.*)/) { $hn = $1 . ucfirst $2; }
		$hn =~ s/[jJ]dl/JDL/;

		my $ts = toString $f "h->status.$n","attr_s";
		gen qq{
!	\} else if (strcmp(attr,GLITE_JP_LB_$hn) == 0) \{
!		char	*attr_s;
!		$ts
!		if (attr_s) \{
!			av = calloc(2, sizeof(glite_jp_attrval_t));
!			av[0].name = strdup(attr);
!			av[0].value = attr_s;
!			av[0].timestamp = h->status.stateEnterTime.tv_sec;
!			av[0].size = -1;
!		\}
};
	}
@@@}
	} else {
		char et[BUFSIZ];
		*attrval = NULL;
        	err.code = EINVAL;
		snprintf(et,sizeof et,"No such attribute '%s'.",attr);
		et[BUFSIZ-1] = 0;
		err.desc = et;
	        return glite_jp_stack_error(ctx,&err);
	}

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
			if (glite_lbu_JobIdDup(h->events[i]->any.jobId,&js->pub.jobId)) {
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
				h->lastStatusHistory = &(h->fullStatusHistory[nstates]);
			}
			old_state = js->pub.state;
			nstates++;
		}

		i++;
	}
	h->fullStatusHistory[nstates] = NULL;

/* TODO: fill in also subjobs */
	if (js->pub.children_num > 0) {	
		edg_wll_Context context;
		glite_lbu_JobId *subjobs;

		if ((retval = edg_wll_InitContext(&context)) != 0) return retval;
		subjobs = calloc(js->pub.children_num, sizeof (*subjobs));
		if ((retval = edg_wll_GenerateSubjobIds(context, 
			js->pub.jobId, js->pub.children_num, js->pub.seed, &subjobs) ) != 0 ) {
			goto err;
		}
		js->pub.children = calloc(js->pub.children_num + 1, sizeof (*js->pub.children));
		for (i=0; i<js->pub.children_num; i++) {
			js->pub.children[i] = glite_lbu_JobIdUnparse(subjobs[i]);
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

