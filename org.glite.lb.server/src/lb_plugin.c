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

//#include "jobstat.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/builtin_plugins.h"
#include "glite/jp/backend.h"

#define INITIAL_NUMBER_EVENTS 100

typedef struct _lb_buffer_t {
	char *buf;
	size_t pos, size;
	ssize_t offset;
} lb_buffer_t;

typedef struct _lb_handle {
	edg_wll_Event	**events;
	edg_wll_JobStat	status;
} lb_handle;



static int lb_query(void *fpctx,void *handle,glite_jp_attr_t attr,glite_jp_attrval_t **attrval);
static int lb_open(void *,void *, const char *uri, void **);
static int lb_close(void *,void *);
/*static int lb_status(edg_wll_Event *event, edg_wll_JobStat *status);*/
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
	memset(&buffer, sizeof(buffer), 0);
	buffer.buf = malloc(BUFSIZ);
	maxnevents = INITIAL_NUMBER_EVENTS;
	nevents = 0;
	h->events = malloc(maxnevents * sizeof(edg_wll_Event *));

	if ((retval = read_line(ctx, bhandle, &buffer, &line)) != 0) goto fail;
	while (line) {
		printf("(DEBUG)lb plugin: '%s'\n", line);

		if (nevents >= maxnevents) {
			maxnevents <<= 1;
			h->events = realloc(h->events, maxnevents * sizeof(edg_wll_Event *));
		}
		if ((retval = edg_wll_ParseEvent(context, line, &h->events[nevents])) != 0) goto fail;
		nevents++;
		free(line);

		if ((retval = read_line(ctx, bhandle, &buffer, &line)) != 0) goto fail;
	}

	free(buffer.buf);

	// compute state of the job - still unclear if needed
	// TODO

	handle = (void **) &h;

	return 0;

fail:
	for (i = 0; i < nevents; i++) edg_wll_FreeEvent(h->events[i]);
	free(h->events);
	free(buffer.buf);
	edg_wll_FreeContext(context);
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
			i++;
		}
		free(h->events);
	}

	if (h->status.state != EDG_WLL_JOB_UNDEF) 
		edg_wll_FreeStatus(&h->status);

	free(h);

	return 0;
}



static int lb_query(void *fpctx,void *handle,glite_jp_attr_t attr,glite_jp_attrval_t **attrval)
{
	lb_handle		*h = (lb_handle *) handle;
	glite_jp_context_t	ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t 	err; 
	glite_jp_attrval_t	*av = NULL;
	int			i, n_tags;


        glite_jp_clear_error(ctx); 
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	switch (attr.type) {
		case GLITE_JP_ATTR_OWNER:
			if (h->events)
			{
				i = 0;
				while (h->events[i])
				{
					if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) 
					{
						av = calloc(2, sizeof(glite_jp_attrval_t));
						av[0].attr.type = GLITE_JP_ATTR_OWNER;
						av[0].value.s = strdup(h->events[i]->any.user);

						break;
					}
					i++;
				}
			}
			//av = calloc(2, sizeof(glite_jp_attrval_t));
			//av[0].value.s = strdup(h->status.owner);
			break;
		case GLITE_JP_ATTR_TIME:
			if (edg_wll_StringToStat(attr.name) == EDG_WLL_JOB_SUBMITTED)
			{
				if (h->events)
				{
					i = 0;
					while (h->events[i])
					{
						if (h->events[i]->type == EDG_WLL_EVENT_REGJOB) 
						{
							av = calloc(2, sizeof(glite_jp_attrval_t));
							av[0].attr.type = GLITE_JP_ATTR_TIME;
							av[0].value.time.tv_sec = 
								h->events[i]->any.timestamp.tv_sec;
							av[0].value.time.tv_usec = 
								h->events[i]->any.timestamp.tv_usec;

							break;
						}
						i++;
					}
				}
			} 
			else	// state need to be counted (not impl. yet)
			{
				*attrval = NULL;
	                	err.code = ENOSYS;
		                err.desc = "Not implemented yet.";
        		        return glite_jp_stack_error(ctx,&err);
			}
			// XXX - not clear yet how to care about this
			//av = calloc(2, sizeof(glite_jp_attrval_t));
			//av[0].value.time.tv_sec = h->status.stateEnterTime.tv_sec;
			//av[0].value.time.tv_usec = h->status.stateEnterTime.tv_usec;

			break;
		case GLITE_JP_ATTR_TAG:
			if (h->events)
			{
				i = 0;
				n_tags = 0;

				while (h->events[i])
				{
					if ((h->events[i]->type == EDG_WLL_EVENT_USERTAG) &&
						!(strcmp(h->events[i]->userTag.name, attr.name)) )
					{
						av = realloc(av, (n_tags+2) * sizeof(glite_jp_attrval_t));

						av[i].value.tag.name = strdup(h->events[i]->userTag.name);
						av[i].value.tag.sequence = -1;
						av[i].value.tag.timestamp = 
							h->events[i]->any.timestamp.tv_sec;
						av[i].value.tag.binary = 0;
						av[i].value.tag.size = -1;
						av[i].value.tag.value = strdup(h->events[i]->userTag.value);
						av[i].attr.type	= GLITE_JP_ATTR_TAG;

						av[i+1].attr.type = GLITE_JP_ATTR_UNDEF;

						n_tags++;
					}
					i++;
				}
			}
			break;
		default: 
			*attrval = NULL;
                	err.code = ENOENT;
	                err.desc = "No such attriburte.";
        	        return glite_jp_stack_error(ctx,&err);
			break;
	}

	*attrval = av;
	return 0;
}


/*
 * not finished - not clear if needed 
static int lb_status(edg_wll_Event *event, edg_wll_JobStat *status)
{
        intJobStat	*js;
        int		i, be_strict = 0;
	char		*errstring;
        

        calloc(1, sizeof(intJobStat));
        
	i = 0;
        while (events[i])  
        {
		processEvent(js, events[i], 0, be_strict, &errstring);
		i++;
	}

	memcpy(status, &js->pub, sizeof(edg_wll_JobStat));
	return 0;

err:
	destroy_intJobStat(js);
	return -1;
}
*/


/*
 * realloc the line to double size if needed
 *
 * \return 0 if failed, did nothing
 * \return 1 if success
 */
int check_realloc_line(char **line, size_t *maxlen, size_t len) {
	void *tmp;

	if (len >= *maxlen) {
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
static int read_line(glite_jp_context_t  ctx, void *handle, lb_buffer_t *buffer, char **line) {
	size_t maxlen, len, i;
	ssize_t nbytes;
	int retval;

	maxlen = BUFSIZ;
	i = 0;
	len = 0;
	*line = malloc(maxlen);

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
					buffer->size = (size_t)nbytes;
					buffer->offset += nbytes;
				}
			} else goto fail;
		}

		/* we have buffer->size - buffer->pos bytes */
		i = buffer->pos;
		while (i < buffer->size && buffer->buf[i] != '\n' && buffer->buf[i] != '\0') {
			if (!check_realloc_line(line, &maxlen, len)) {
				retval = ENOMEM;
				goto fail;
			}
			*line[len++] = buffer->buf[i++];
		}
		buffer->pos = i;
	} while (len && *line[len - 1] != '\n' && *line[len - 1] != '\0');

	if (len) *line[len - 1] = '\0';
	else {
		free(*line);
		*line = NULL;
	}

	return 0;

fail:
	free(*line);
	*line = NULL;
	return retval;
}
