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


/*
@@@AUTO
*/
@@@LANG: C

/* Helper functions for getting events from the LB database *
 * XXX: lots of stuff still hadcoded:
 * 		there's mapping db.columns <-> union event fields
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"
#include "get_events.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/ulm_parse.h"
#include "db_supp.h"

static void edg_wll_set_event_field(edg_wll_Event *,char *,char *);
static void edg_wll_set_event_field_warn(edg_wll_Event *,char *,char *);

int edg_wll_get_event_flesh(edg_wll_Context ctx,int n,edg_wll_Event *e)
{
	char	*jobid = edg_wlc_JobIdGetUnique(e->any.jobId),
		*q = NULL,*nameval[2];
	glite_lbu_Statement	sh;
	int	ret,t;
	const char	*tables[] = { "short_fields","long_fields" };
	edg_wll_Event *f;

	edg_wll_ResetError(ctx);

	trio_asprintf(&q, "select ulm from events_flesh where jobid = '%|Ss' and event = %d", jobid, n);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);

	if ((ret=edg_wll_ExecSQL(ctx,q,&sh)) < 0) goto cleanup;
	if (edg_wll_FetchRow(ctx,sh,1,NULL,&nameval[0]) == 1) {
		//fprintf(stderr, "got ulm: '%s'\n", nameval[0]);
		// nasty ;-)
		edg_wll_ParseEvent(ctx,nameval[0],&f);
		free(nameval[0]);

		if(f) {
			f->any.arrived = e->any.arrived;
			edg_wll_FreeEvent(e);

			memcpy(e, f, sizeof *e);
			free(f);
			ret=edg_wll_CheckEvent(ctx,e);
		}
		else {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "Unknown event type encountered during dump; jobid %s, event %d", jobid, n);
			ret = EINVAL;
		}
	} else ret = ENOENT;
	free(q); q = NULL;
	glite_lbu_FreeStmt(&sh);

	// old way keeped for compatibility/slow migration
	if (ret != 0) {
	for (t=0; t<=1; t++) {
		trio_asprintf(&q,"select name,value from %s "
				"where jobid = '%|Ss' and event = %d ",
				tables[t],jobid,n);
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB,
			LOG_PRIORITY_DEBUG, q);

		if ((ret=edg_wll_ExecSQL(ctx,q,&sh)) < 0) goto cleanup;

		while ((ret=edg_wll_FetchRow(ctx,sh,sizeof(nameval)/sizeof(nameval[0]),NULL,nameval)) > 0) {
			assert(ret == 2);
			edg_wll_set_event_field(e,nameval[0],nameval[1]);
			free(nameval[0]);
			/* XXX: nameval[1] freed in edg_wll_set_event_field
			 * if necessary
			 */
		}

		if (ret<0) goto cleanup;
		glite_lbu_FreeStmt(&sh);
		free(q); q=NULL;
	}
	ret=edg_wll_CheckEvent(ctx,e);
	}

cleanup:
	if (sh) glite_lbu_FreeStmt(&sh);
	free(jobid);
	free(q);

	if (ret) { edg_wll_FreeEvent(e); memset(e,0,sizeof *e); }
	return ret;
}


/* print/log  warning for database inconsistency */

static void edg_wll_set_event_field_warn(
		edg_wll_Event	*event,
		char		*name,
		char		*value)
{
	char *e = edg_wll_EventToString(event->any.type);

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, 
		"edg_wll_set_event_field: bad field:"
		"code=\"%s\" name=\"%s\" value=\"%s\"",
		e, name, value);
	free(e);
	/* XXX edg_wll_Log */
}

/* set event structure field */

static void edg_wll_set_event_field(
		edg_wll_Event	*event,
		char		*name,
		char		*value)
{
/* XXX: where's the best place to hande it? */
	if (!strcasecmp(name,"SRC_INSTANCE")) {
		event->any.src_instance = value;
		return;
	}

/* XXX: handled separately, should go to event_head one day */
	if (!strcasecmp(name,"SEQCODE")) {
		event->any.seqcode = value;
		return;
	}

@@@{
	for my $n (getAllFieldsOrdered $event) {
		my @occ = getFieldOccurence $event $n;
		next if $#occ == 0 && $occ[0] eq '_common_';
		selectType $event $occ[0];
		my $f = selectField $event $n;
		my $name = getName $f;
		my $lcname = lc $name;
		gen qq{
!	if (!strcasecmp(name,"$lcname")) \{
!		switch (event->any.type) \{
};
		for (@occ) {
			next if $_ eq '_common_';
			selectType $event $_;
			$f = selectField $event $n;
			my $fucname = ucfirst $n;
			my $uctype = uc $_;
			my $flctype = lcfirst $_;
			if ($flctype =~ m/^pBS/) { $flctype = ucfirst $flctype; }
			if ($flctype =~ m/^cREAM/) { $flctype = ucfirst $flctype; }
			if ($flctype =~ m/^condor/) { $flctype = ucfirst $flctype; }
			my $frs = $f->{codes} ?
				"event->$flctype.$name = edg_wll_StringTo$_${fucname}(value);" :
				fromString $f 'value',"event->$flctype.$name";
			gen qq{
!			case EDG_WLL_EVENT_$uctype: $frs break;
};
		}
		gen qq{
!			default: edg_wll_set_event_field_warn(event,name,value); break;
!		\} /* switch */
!		free(value);
!		return;
!	\}
};
	}
@@@}

	edg_wll_set_event_field_warn(event,name,value);
	free(value);
	return;
}
		

int compare_events_by_tv(const void *a, const void *b)
{
	const edg_wll_Event *e = (edg_wll_Event *)a;
	const edg_wll_Event *f = (edg_wll_Event *)b;

	if (e->any.timestamp.tv_sec < f->any.timestamp.tv_sec) return -1;
	if (e->any.timestamp.tv_sec > f->any.timestamp.tv_sec) return 1;
	if (e->any.timestamp.tv_usec < f->any.timestamp.tv_usec) return -1;
	if (e->any.timestamp.tv_usec > f->any.timestamp.tv_usec) return 1;
	return 0;
}
