#ident "$Header$"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "glite/lb-utils/db.h"
#include "glite/lb-utils/trio.h"

#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/purge.h"
#include "glite/lb/purge.h"
#include "glite/lb/events.h"
#include "glite/lb/dump.h"
#include "glite/lb/load.h"

#include "db_supp.h"
#include "store.h"
#include "purge.h"
#include "query.h"
#include "get_events.h"
#include "server_state.h"

static int read_line(char **buff, size_t *maxsize, int fd);

int edg_wll_LoadEvents(edg_wll_Context ctx,const edg_wll_LoadRequest *req,edg_wll_LoadResult *result)
{
	int					fd,
						reject_fd = -1,
						readret, i;
	size_t					maxsize;
	char			   *line = NULL,
						buff[30];
	edg_wll_Event	   *event;
	glite_lbu_JobId		jobid = NULL;


	edg_wll_ResetError(ctx);
	
	if ( !req->server_file )
		return edg_wll_SetError(ctx, EINVAL, "Server file is not specified for load");

	if ( (fd = open(req->server_file, O_RDONLY)) == -1 )
		return edg_wll_SetError(ctx, errno, "Server can not open the file");

	if (glite_lbu_Transaction(ctx->dbctx) != 0) 
		return edg_wll_SetErrorDB(ctx);

	memset(result,0,sizeof(*result));
	i = 0;
	while ( 1 )
	{
		/*	Read one line
		 */
		if ( (readret = read_line(&line, &maxsize, fd)) == -1 ) {
			edg_wll_SetError(ctx, errno, "reading dump file");
			glite_lbu_Rollback(ctx->dbctx);
			return edg_wll_Error(ctx, NULL, NULL);
		}

		if ( readret == 0 )
			break;

		i++;
		if (   sscanf(line, "DG.ARRIVED=%s %*s", buff) != 1
			|| edg_wll_ParseEvent(ctx, line, &event) )
		{
			char errs[100];
			sprintf(errs, "Error parsing event at line %d", i);
			if ( !edg_wll_Error(ctx,NULL,NULL) )
				edg_wll_SetError(ctx, EINVAL, errs);
			fprintf(stderr, errs);
			continue;
		}
		edg_wll_ULMDateToTimeval(buff, &(event->any.arrived));

		if ( i == 1 )
		{
			result->from = event->any.arrived.tv_sec;
			result->to = event->any.arrived.tv_sec;
		}
		ctx->event_load = 1;
		if ( edg_wll_StoreEvent(ctx, event, NULL) )
		{
			char		*errdesc;
			int		len = strlen(line),
					total = 0,
					written;

			edg_wll_Error(ctx, NULL, &errdesc);
			fprintf(stderr, "Can't store event: %s\n", errdesc);
			if ( reject_fd == -1 )
			{
				char   *s, *s1;

				if ( result->server_file != NULL )
					goto cycle_clean;

				s1 = strdup(req->server_file);
				s = strrchr(s1, '/');
				if ( s )
				{
					*s = '\0';
					reject_fd = edg_wll_CreateFileStorage(ctx,FILE_TYPE_LOAD,s1,&(result->server_file));
				}
				else
					reject_fd = edg_wll_CreateFileStorage(ctx,FILE_TYPE_LOAD,NULL,&(result->server_file));
				if ( reject_fd == -1 )
					goto cycle_clean;
				printf("New reject file %s created\n", result->server_file);
				free(s1);
			}
			/*	Save the line into "reject_file"
			 */
			while (total != len) {
				written = write(reject_fd, line+total, len-total);
				if (written < 0 && errno != EAGAIN) {
					edg_wll_SetError(ctx, errno, "writing load reject file");
					free(line);
					break;
				}
				total += written;
			}
			write(reject_fd,"\n",1);
		}
		else
		{
			result->to = event->any.arrived.tv_sec;
			if ( jobid )
			{
				char *md5_jobid = glite_lbu_JobIdGetUnique(jobid);
				
				if ( strcmp(md5_jobid, glite_lbu_JobIdGetUnique(event->any.jobId)) )
				{
					edg_wll_JobStat st;

					edg_wll_JobStatus(ctx, jobid, 0, &st);
					edg_wll_FreeStatus(&st);

					glite_lbu_JobIdFree(jobid);
					glite_lbu_JobIdDup(event->any.jobId, &jobid);
				}
				free(md5_jobid);
			}
			else
				glite_lbu_JobIdDup(event->any.jobId, &jobid);
		}


cycle_clean:
		ctx->event_load = 0;
		edg_wll_FreeEvent(event);
	}

	if ( jobid )
	{
		edg_wll_JobStat st;

		edg_wll_JobStatus(ctx, jobid, 0, &st);
		edg_wll_FreeStatus(&st);
		glite_lbu_JobIdFree(jobid);
	}

	if ( reject_fd != -1 )
		close(reject_fd);

	if (glite_lbu_Commit(ctx->dbctx) != 0)
		return edg_wll_SetErrorDB(ctx);

	return edg_wll_Error(ctx,NULL,NULL);
}

#define BUFFSZ			1024

static int read_line(char **buff, size_t *maxsize, int fd)
{
	int		ct, i;
	void		*tmp;


	if ( *buff == NULL )
	{
		*buff = malloc(BUFFSZ);
		if ( *buff == NULL )
			return -1;
		*maxsize = BUFFSZ;
	}

	i = 0;
	while ( 1 )
	{
		if (i >= *maxsize) {
			(*maxsize) *= 2;
			if ((tmp = realloc(*buff, *maxsize)) == NULL) return -1;
			*buff = (char *)tmp;
		}
		if ( (ct = read(fd, (*buff)+i, 1)) == -1 )
			return -1;

		if ( ct == 0 )
			return 0;

		if ( (*buff)[i] == '\n' )
		{
			(*buff)[i--] = '\0';
			while ( (i != -1) && isspace((*buff)[i]) ) i--;
			if ( i == -1 )
			{
				/**	empty line
				 */
				i = 0;
				continue;
			}
			else
				return 1;
		}

		i++;
	}
}
