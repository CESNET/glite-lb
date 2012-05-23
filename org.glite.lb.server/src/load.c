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


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"

#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/events.h"

#include "store.h"
#include "purge.h"
#include "query.h"
#include "get_events.h"
#include "server_state.h"
#include "db_supp.h"
#include "lb_proto.h"

static int read_line(char **buff, size_t *maxsize, int fd);

int edg_wll_LoadEventsServer(edg_wll_Context ctx,const edg_wll_LoadRequest *req,edg_wll_LoadResult *result)
{
	int					fd,
						reject_fd = -1,
						readret, i, ret;
	size_t					maxsize;
	char			   *line = NULL, *errdesc,
						buff[30];


	edg_wll_ResetError(ctx);
	
	if ( !req->server_file )
		return edg_wll_SetError(ctx, EINVAL, "Server file is not specified for load");

	if ( (fd = open(req->server_file, O_RDONLY)) == -1 )
		return edg_wll_SetError(ctx, errno, "Server can not open the file");

	memset(result,0,sizeof(*result));
	i = 0;
	while ( 1 )
	{
		/*	Read one line
		 */
		if ( (readret = read_line(&line, &maxsize, fd)) == -1 ) {
			return edg_wll_SetError(ctx, errno, "reading dump file");
		}

		if ( readret == 0 )
			break;

		ctx->event_load = 1;
		i++;
		if (db_store(ctx, line))
		{
			char errs[100];
			sprintf(errs, "Error parsing event at line %d", i);
			if ( !edg_wll_Error(ctx,NULL,NULL) )
				edg_wll_SetError(ctx, EINVAL, errs);
			fprintf(stderr, "%s", errs);
			continue;
		}

		if ((ret = edg_wll_Error(ctx, NULL, &errdesc)) != 0) {
			int		len = strlen(line),
					total = 0,
					written;

			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Can't store event: %s", errdesc);
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
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "New reject file %s created", result->server_file);
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

cycle_clean:
		ctx->event_load = 0;
	}

	if ( reject_fd != -1 )
		close(reject_fd);

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

