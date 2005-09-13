#ident "$Header: "

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "glite/lb/context.h"
#include "glite/lb/jobstat.h"

#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"

#include "glite/lb/trio.h"

#include "jobstat.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/builtin_plugins.h"
#include "glite/jp/backend.h"
#include "glite/jp/known_attr.h"
#include "jp_job_attrs.h"

#define INITIAL_NUMBER_EVENTS 100
#define LB_PLUGIN_NAMESPACE "urn:org.glite.lb"

typedef struct _lb_buffer_t {
	char *buf;
	size_t pos, size;
	off_t offset;
} lb_buffer_t;

typedef struct _lb_handle {
	edg_wll_Event	**events;
	edg_wll_JobStat	status;
} lb_handle;


extern int processEvent(intJobStat *, edg_wll_Event *, int, int, char **);

static int lb_query(void *fpctx,void *handle,const char * attr,glite_jp_attrval_t **attrval);
static int lb_open(void *,void *, const char *uri, void **);
static int lb_close(void *,void *);
static int lb_status(edg_wll_Event **event, edg_wll_JobStat *status);
static int read_line(glite_jp_context_t  ctx, void *handle, lb_buffer_t *buffer, char **line);



static int lb_dummy(void *fpctx, void *handle, int oper, ...)
{
	puts("lb_dummy() - generic call not used; for testing purposes only...");
	return -1;
}



int init(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data)
{
	data->fpctx = ctx;

	data->uris = calloc(2,sizeof *data->uris);
	data->uris[0] = strdup(GLITE_JP_FILETYPE_LB);

	data->classes = calloc(2,sizeof *data->classes);
	data->classes[0] = strdup("lb");

	data->ops.open 	= lb_open;
	data->ops.close = lb_close;
	data->ops.attr 	= lb_query;
	data->ops.generic = lb_dummy;
	
	printf("lb init OK\n");
	return 0;
}


void done(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data) {
	free(data->uris[0]);
	free(data->classes[0]);
	free(data->uris);
	free(data->classes);
	memset(data, 0, sizeof(*data));
}


static int lb_open(void *fpctx, void *bhandle, const char *uri, void **handle)
{
	lb_handle           *h;
	lb_buffer_t         buffer;
	glite_jp_context_t  ctx = (glite_jp_context_t) fpctx;
	char *line;
	int retval;
	edg_wll_Context     context;
	size_t              nevents, maxnevents, i;
	
	h = calloc(1, sizeof(lb_handle));

	if ((retval = edg_wll_InitContext(&context)) != 0) return retval;

	// read the file given by bhandle
	// parse events into h->events array
	memset(&buffer, 0, sizeof(buffer));
	buffer.buf = malloc(BUFSIZ);
	maxnevents = INITIAL_NUMBER_EVENTS;
	nevents = 0;
	h->events = malloc(maxnevents * sizeof(edg_wll_Event *));

	if ((retval = read_line(ctx, bhandle, &buffer, &line)) != 0) goto fail;
	while (line) {
//		printf("(DEBUG)lb plugin: '%s'\n", line);

		if (line[0]) {
			if (nevents >= maxnevents) {
				maxnevents <<= 1;
				h->events = realloc(h->events, maxnevents * sizeof(edg_wll_Event *));
			}
			if ((retval = edg_wll_ParseEvent(context, line, &h->events[nevents])) != 0) {
				free(line);
				goto fail;
			}
			nevents++;
		}
		free(line);

		if ((retval = read_line(ctx, bhandle, &buffer, &line)) != 0) goto fail;
	}
	free(line);

	free(buffer.buf);
	edg_wll_FreeContext(context);

	if (nevents >= maxnevents) {
		maxnevents <<= 1;
		h->events = realloc(h->events, maxnevents * sizeof(edg_wll_Event *));
	}
	h->events[nevents] = NULL;

	printf("lb open %d events\n", nevents);

	/* count state of job given by loaded events */
	if ( (retval = lb_status(h->events, &(h->status))) ) goto fail;

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
	return retval;
}



static int lb_close(void *fpctx,void *handle)
{
	lb_handle           *h = (lb_handle *) handle;
	int                 i;

	
	// Free allocated stuctures
	if (h->events)
	{
		i = 0;
		while (h->events[i])
		{
			edg_wll_FreeEvent(h->events[i]);
			free(h->events[i]);
			i++;
		}
		free(h->events);
	}

	if (h->status.state != EDG_WLL_JOB_UNDEF) 
		edg_wll_FreeStatus(&h->status);

	free(h);

	return 0;
}



static int lb_query(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval)
{
	lb_handle		*h = (lb_handle *) handle;
	glite_jp_context_t	ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t 	err; 
	glite_jp_attrval_t	*av = NULL;
	int			i, n_tags;
	const char             *tag;


        glite_jp_clear_error(ctx); 
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if (strncmp(attr, GLITE_JP_ATTR_OWNER, strlen(GLITE_JP_ATTR_OWNER)) == 0 ||
            strncmp(attr, GLITE_JP_LB_user, strlen(GLITE_JP_LB_user)) == 0) {
		if (h->events) {
			i = 0;
			while (h->events[i]) {
				if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) {
					av = calloc(2, sizeof(glite_jp_attrval_t));
					av[0].name = strdup(GLITE_JP_ATTR_OWNER);
					av[0].value = strdup(h->events[i]->any.user);
					av[0].size = -1;
					av[0].timestamp = 
						h->events[i]->any.timestamp.tv_sec;
					break;
				}
				i++;
			}
		}
	} else if (strncmp(attr, GLITE_JP_LB_SUBMITTED, strlen(GLITE_JP_LB_SUBMITTED)) == 0) {
		if (h->events) {
			i = 0;
			while (h->events[i]) {
				if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) {
					av = calloc(2, sizeof(glite_jp_attrval_t));
					av[0].name = strdup(GLITE_JP_LB_SUBMITTED);
					trio_asprintf(&av[0].value,"%ld.%06ld",
						h->events[i]->any.timestamp.tv_sec,
						h->events[i]->any.timestamp.tv_usec);
					av[0].timestamp = 
						h->events[i]->any.timestamp.tv_sec;
					break;
				}
				i++;
			}
		}
	} else if (strncmp(attr, GLITE_JP_LB_TERMINATED, strlen(GLITE_JP_LB_TERMINATED)) == 0 || 
                   strncmp(attr, GLITE_JP_LB_VO, strlen(GLITE_JP_LB_VO)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_aTag, strlen(GLITE_JP_LB_aTag)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_rQType, strlen(GLITE_JP_LB_rQType)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_eDuration, strlen(GLITE_JP_LB_eDuration)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_eNodes, strlen(GLITE_JP_LB_eNodes)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_eProc, strlen(GLITE_JP_LB_eProc)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_RB, strlen(GLITE_JP_LB_RB)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_CE, strlen(GLITE_JP_LB_CE)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_host, strlen(GLITE_JP_LB_host)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_UIHost, strlen(GLITE_JP_LB_UIHost)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_CPUTime, strlen(GLITE_JP_LB_CPUTime)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_NProc, strlen(GLITE_JP_LB_NProc)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_finalStatus, strlen(GLITE_JP_LB_finalStatus)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_finalStatusDate, strlen(GLITE_JP_LB_finalStatusDate)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_finalStatusReason, strlen(GLITE_JP_LB_finalStatusReason)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_LRMSDoneStatus, strlen(GLITE_JP_LB_LRMSDoneStatus)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_LRMSStatusReason, strlen(GLITE_JP_LB_LRMSStatusReason)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_retryCount, strlen(GLITE_JP_LB_retryCount)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_additionalReason, strlen(GLITE_JP_LB_additionalReason)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_jobType, strlen(GLITE_JP_LB_jobType)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_nsubjobs, strlen(GLITE_JP_LB_nsubjobs)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_lastStatusHistory, strlen(GLITE_JP_LB_lastStatusHistory)) == 0 ||
                   strncmp(attr, GLITE_JP_LB_fullStatusHistory, strlen(GLITE_JP_LB_fullStatusHistory)) == 0 ) {
		{
			*attrval = NULL;
                	err.code = ENOSYS;
	                err.desc = "Not implemented yet.";
       		        return glite_jp_stack_error(ctx,&err);
		}
	} else if (strncmp(attr, GLITE_JP_LBTAG_NS, strlen(GLITE_JP_LBTAG_NS)) == 0) {
		tag = strrchr(attr, ':');
		if (h->events && tag) {
			tag++;
			i = 0;
			n_tags = 0;

			while (h->events[i]) {
				if ((h->events[i]->type == EDG_WLL_EVENT_USERTAG) &&
					(strcmp(h->events[i]->userTag.name, tag) == 0) )
				{
					av = realloc(av, (n_tags+2) * sizeof(glite_jp_attrval_t));
					memset(&av[n_tags], 0, 2 * sizeof(glite_jp_attrval_t));

					av[n_tags].name = strdup(attr);
					av[n_tags].value = strdup(h->events[i]->userTag.value);
					av[n_tags].timestamp = 
						h->events[i]->any.timestamp.tv_sec;
					av[n_tags].size = -1;

					n_tags++;
				}
				i++;
			}
		}
	} else {
		*attrval = NULL;
        	err.code = ENOENT;
                err.desc = "No such attribute.";
	        return glite_jp_stack_error(ctx,&err);
	}

	*attrval = av;
	return 0;
}


static int lb_status(edg_wll_Event **events, edg_wll_JobStat *status)
{
        intJobStat	*js;
        int		i, be_strict = 0;
	char		*errstring;
        

        js = calloc(1, sizeof(intJobStat));
        
	i = 0;
        while (events[i])  
        {
		if (!processEvent(js, events[i], 0, be_strict, &errstring)) {
			// XXX: better way would be to export definitions
			//      of intermal job_status error codes and test 
			//      it againts RET_FATAL
			goto err;
		}
		i++;
	}

	memcpy(status, &js->pub, sizeof(edg_wll_JobStat));

	// XXX: awful, hopefully working
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
 * /return error code
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
