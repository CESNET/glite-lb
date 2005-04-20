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

//#include "jobstat.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/builtin_plugins.h"

typedef struct _lb_handle {
	edg_wll_Event	*events;
	edg_wll_JobStat	status;
} lb_handle;



static int lb_query(void *fpctx,void *handle,glite_jp_attr_t attr,glite_jp_attrval_t **attrval);
static int lb_open(void *,void *,void **);
static int lb_close(void *,void *);
static int lb_status(edg_wll_Event *event, edg_wll_JobStat *status);



static int lb_dummy()
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



static int lb_open(void *fpctx,void *bhandle,void **handle)
{
	lb_handle	*h;

	
	h = calloc(1, sizeof(lb_handle));


	// read the file given by bhandle
	// parse events into h->events array

	// compute state of the job - still unclear if needed

	handle = (void **) &h;

	return 0;
}



static int lb_close(void *fpctx,void *handle)
{
	lb_handle	*h = (lb_handle *) handle;
	int		i;

	
	// Free allocated stuctures
	if (h->events)
	{
		i = 0;
		while (h->events[i].type != EDG_WLL_EVENT_UNDEF)
		{
			edg_wll_FreeEvent(&h->events[i]); 
			i++;
		}
		free(h->events);
	}

	if (h->status.state != EDG_WLL_JOB_UNDEF) 
		edg_wll_FreeStatus(&h->status);

	return 0;
}



static int lb_query(void *fpctx,void *handle,glite_jp_attr_t attr,glite_jp_attrval_t **attrval)
{
	lb_handle		*h = (lb_handle *) handle;
	glite_jp_context_t	ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t 	err; 
	glite_jp_attrval_t	*av;
	int			i, n_tags;


        glite_jp_clear_error(ctx); 
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	switch (attr.type) {
		case GLITE_JP_ATTR_OWNER:
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].value.s = strdup(h->status.owner);
			break;
		case GLITE_JP_ATTR_TIME:
			// XXX - not clear yet how to care about this
			av = calloc(2, sizeof(glite_jp_attrval_t));
			av[0].value.time.tv_sec = h->status.stateEnterTime.tv_sec;
			av[0].value.time.tv_usec = h->status.stateEnterTime.tv_usec;
			break;
		case GLITE_JP_ATTR_TAG:
			if (h->events)
			{
				i = 0;
				n_tags = 0;

				while (h->events[i].type != EDG_WLL_EVENT_UNDEF)
				{
					if (h->events[i].type == EDG_WLL_EVENT_USERTAG)
					{
						av = realloc(av, (n_tags+2) * sizeof(glite_jp_attrval_t));

						av[i].value.tag.name = strdup(h->events[i].userTag.name);
						av[i].value.tag.sequence = -1;
						av[i].value.tag.timestamp = 
							h->events[i].any.timestamp.tv_sec;  
						av[i].value.tag.binary = 0;
						av[i].value.tag.size = -1;
						av[i].value.tag.value = strdup(h->events[i].userTag.value);
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
        while (events[i].any.type != EDG_WLL_EVENT_UNDEF)  
        {
		processEvent(js, &(events[i]), 0, be_strict, &errstring);
		i++;
	}

	memcpy(status, &js->pub, sizeof(edg_wll_JobStat));
	return 0;

err:
	destroy_intJobStat(js);
	return -1;
}
*/

