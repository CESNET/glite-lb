#ident "$Header$"

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
#include <syslog.h>

#include "glite/lbu/trio.h"
#include "get_events.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"

static void edg_wll_set_event_field(edg_wll_Event *,char *,char *);
static void edg_wll_set_event_field_warn(edg_wll_Event *,char *,char *);

int edg_wll_get_event_flesh(edg_wll_Context ctx,int n,edg_wll_Event *e)
{
	char	*jobid = edg_wlc_JobIdGetUnique(e->any.jobId),
		*q = NULL,*nameval[2];
	edg_wll_Stmt	sh;
	int	ret,t;
	const char	*tables[] = { "short_fields","long_fields" };

	edg_wll_ResetError(ctx);

	for (t=0; t<=1; t++) {
		trio_asprintf(&q,"select name,value from %s "
				"where jobid = '%|Ss' and event = %d ",
				tables[t],jobid,n);

		if ((ret=edg_wll_ExecStmt(ctx,q,&sh)) < 0) goto cleanup;

		while ((ret=edg_wll_FetchRow(sh,nameval)) > 0) {
			edg_wll_set_event_field(e,nameval[0],nameval[1]);
			free(nameval[0]);
			/* XXX: nameval[1] freed in edg_wll_set_event_field
			 * if necessary
			 */
		}

		if (ret<0) goto cleanup;
		edg_wll_FreeStmt(&sh);
		free(q); q=NULL;
	}

	ret=edg_wll_CheckEvent(ctx,e);

cleanup:
	if (sh) edg_wll_FreeStmt(&sh);
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

	fprintf(stderr, "edg_wll_set_event_field: bad field:"
		"code=\"%s\" name=\"%s\" value=\"%s\"\n",
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
