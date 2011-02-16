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


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <regex.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"
#include "glite/lb/context-int.h"
#include "glite/lb/intjobstat.h"
#include "glite/lb/seqcode_aux.h"

#include "store.h"
#include "index.h"
#include "jobstat.h"
#include "get_events.h"


/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

static int compare_pevents_by_seq(const void *a, const void *b)
{
        const edg_wll_Event **e = (const edg_wll_Event **) a;
        const edg_wll_Event **f = (const edg_wll_Event **) b;
	return compare_events_by_seq(*e,*f);
}

void edg_wll_SortEvents(edg_wll_Event *e)
{
	int	n;

	if (!e) return;
	for (n=0; e[n].type; n++);
	qsort(e,n,sizeof(*e),compare_events_by_seq);
}

void edg_wll_SortPEvents(edg_wll_Event **e)
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


