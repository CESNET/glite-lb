#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdsoap2.h>

#include "soap_version.h"
#include "bk_ws_H.h"
#include "ws_typemap.h"

#include "glite/lb/consumer.h"
/* XXX: references only, src and dest share pointers */

void edg_wll_JobStatCodeToSoap(edg_wll_JobStatCode in, enum lbt__statName *out)
{
	switch ( in )
	{
/* XXX: these two should never occur */
	case EDG_WLL_NUMBER_OF_STATCODES:
	case EDG_WLL_JOB_UNDEF: *out = -1; break;
@@@{
	for my $stat ($status->getTypesOrdered) {
		my $u = uc $stat;
		my $c = getTypeComment $status $stat;
		gen qq{
!	case EDG_WLL_JOB_$u: *out = $u; break; /**< $c */
};
	}
@@@}
	}
}

void edg_wll_SoapToJobStatCode(enum lbt__statName in, edg_wll_JobStatCode *out)
{
	switch ( in )
	{
//	case UNDEF: *out = EDG_WLL_JOB_UNDEF; break;
@@@{
	for my $stat ($status->getTypesOrdered) {
		my $u = uc $stat;
		my $c = getTypeComment $status $stat;
		gen qq{
!	case $u: *out = EDG_WLL_JOB_$u; break; /**< $c */
};
	}
@@@}
	}
}

int edg_wll_StatusToSoap(struct soap *soap,edg_wll_JobStat const *src,struct lbt__jobStatus **dest)
{
	int	i,j;
	char	*s;
	struct lbt__jobStatus *js;

	if ( !(js = soap_malloc(soap, sizeof(*js))) ) return SOAP_FAULT;
	memset(js, 0, sizeof(*js));
	edg_wll_JobStatCodeToSoap(src->state, &(js->state));
@@@{
	selectType $status '_common_';
	for (getFieldsOrdered $status) {
		my $f = selectField $status $_;
		my $ft = $f->{type};
		my $usuc = $_;
		$usuc = $1.uc($2).$3 while ($usuc =~ /([^_]*)_([a-z])(.*)/);
		if ($ft eq 'jobid') {
			gen "\ts = edg_wlc_JobIdUnparse(src->$_);\n";
			gen "\tjs->$usuc = soap_strdup(soap,s); free(s);\n";
		}
		elsif ($ft eq 'strlist') {
			gen qq{
!	for (i=0; src->$_ && src->$_\[i]; i++);
!	js->__size$usuc = i;
!	js->$usuc = soap_malloc(soap,sizeof(char *) * i);
!	for (j=0; j<i; j++) js->$usuc\[j] = soap_strdup(soap,src->$_\[j]);
};
		} elsif ($ft eq 'intlist') {
			my %fmap = ( children_hist=>'count', stateEnterTimes=>'time');
			gen qq{
!	/* XXX: don't count UNDEF */
!	if (src->$_) {
!		struct lbt__${usuc}Item	**h = soap_malloc(soap,sizeof *h * (EDG_WLL_NUMBER_OF_STATCODES-1));
!		js->__size$usuc = EDG_WLL_NUMBER_OF_STATCODES-1;
!		for (i=1; i<EDG_WLL_NUMBER_OF_STATCODES; i++) {
!			h[i-1] = soap_malloc(soap, sizeof **h);
!			edg_wll_JobStatCodeToSoap(i,&h[i-1]->state);
!			h[i-1]->$fmap{$_} = src->$_\[i];
!		}
!		js->$usuc = h;
!	}
!	else {
!		js->__size$usuc = 0;
!		js->$usuc = NULL;
!	}
};
		} elsif ($ft eq 'taglist') {
			gen qq{
!	{
!		struct lbt__tagValue	*t,**tl = NULL;
!		for (i=0; src->$_ && src->$_\[i].tag; i++) {
!			t = soap_malloc(soap,sizeof *t);
!			tl = realloc(tl,(i+1) * sizeof *tl);
!			t->tag = soap_strdup(soap,src->$_\[i].tag);
!			t->value = soap_strdup(soap,src->$_\[i].value);
!			tl[i] = t;
!		}
!		js->__size$usuc = i;
!		if (i) {
!			js->$usuc = soap_malloc(soap,i * sizeof *tl);
!			memcpy(js->$usuc,tl,i * sizeof *tl);
!			free(tl);
!		} else js->$usuc = NULL;
!	}
};
		} elsif ($ft eq 'stslist') {
			gen qq{
!	for (i=0; src->$_ && src->$_\[i].state; i++);
!	if ((js->__size$usuc = i)) {
!		js->$usuc = soap_malloc(soap,i * sizeof *js->$usuc);
!		for (j=0; j<i; i++) edg_wll_StatusToSoap(soap,src->$_+j,&js->$usuc\[j]);
!	}
!	else js->$usuc = NULL;
};
		} elsif ($ft eq 'timeval') {
			gen "\tjs->$usuc = soap_malloc(soap,sizeof *js->$usuc);\n";
			gen "\tjs->$usuc->tvSec = src->$_.tv_sec;\n";
			gen "\tjs->$usuc->tvUsec = src->$_.tv_usec;\n";
		} elsif ($ft eq 'string') {
			gen "\tjs->$usuc = soap_strdup(soap,src->$_);\n";
		} else { gen "\tjs->$usuc = src->$_;\n"; }
	}
@@@}

	*dest = js;
	return SOAP_OK;
}

void edg_wll_SoapToStatus(struct soap *soap,struct lbt__jobStatus const *src,edg_wll_JobStat *dest)
{
	int	i;

	memset(dest,0,sizeof *dest);
	edg_wll_SoapToJobStatCode(src->state, &(dest->state));

@@@{
	selectType $status '_common_';
	for (getFieldsOrdered $status) {
		my $f = selectField $status $_;
		my $ft = $f->{type};
		my $usuc = $_;
		$usuc = $1.uc($2).$3 while ($usuc =~ /([^_]*)_([a-z])(.*)/);

		if ($ft eq 'jobid') {
			gen "\tif (src->$usuc) edg_wlc_JobIdParse(src->$usuc,&dest->$_);\n";
		} elsif ($ft eq 'strlist') {
			gen "\tfor (i=0; i<src->__size$usuc; i++) soap_unlink(soap,src->$usuc\[i]);\n";
			gen "\tsoap_unlink(soap,src->$usuc);\n";
			gen "\tdest->$_ = realloc(src->$usuc,sizeof(*dest->$_) * (src->__size$usuc + 1));\n";
			gen "\tdest->$_\[src->__size$usuc] = NULL;\n";
		} elsif ($ft eq 'intlist') {
# XXX: only EDG_WLL_NUMBER_OF_STATCODES long lists work
			my %fmap = ( children_hist=>'count', stateEnterTimes=>'time');
			gen qq {
!	if (src->__size$usuc) {
!		dest->$_ = calloc(EDG_WLL_NUMBER_OF_STATCODES+1, sizeof *dest->$_);
!		dest->$_\[0] = src->__size$usuc;
!		for (i=0; i<src->__size$usuc; i++) {
!			edg_wll_JobStatCode	s;
!			edg_wll_SoapToJobStatCode(src->$usuc\[i]->state,&s);
!			assert(s>=0); assert(s<EDG_WLL_NUMBER_OF_STATCODES);
!			dest->$_\[s+1] = src->$usuc\[i]->$fmap{$_};
!		}
!	}
!	else dest->$_ = NULL;
};
		} elsif ($ft eq 'stslist') {
			gen qq{
!	if (src->__size$usuc) {
!		dest->$_ = calloc(src->__size$usuc + 1,sizeof *dest->$_);
!		for (i=0; i<src->__size$usuc; i++) edg_wll_SoapToStatus(soap,src->$usuc\[i],dest->$_ + i);
!	}
!	else dest->$_ = NULL;
};
 		} elsif ($ft eq 'taglist') {
			gen qq{
	if (src->__size$usuc) {
		dest->$_ = calloc(src->__size$usuc + 1,sizeof *dest->$_);
		for (i=0; i<src->__size$usuc; i++) {
			soap_unlink(soap,dest->$_\[i].tag = src->$usuc\[i]->tag);
			soap_unlink(soap,dest->$_\[i].tag = src->$usuc\[i]->tag);
		}
	}
	else dest->$_ = NULL;
};
		} elsif ($ft eq 'timeval') {
			gen "\tdest->$_.tv_sec = src->$usuc->tvSec;\n";
			gen "\tdest->$_.tv_usec = src->$usuc->tvUsec;\n";
		} elsif ($ft eq 'string') {
			gen "\tdest->$_ = src->$usuc;\n";
			gen "\tsoap_unlink(soap,src->$usuc);\n";
		} else {
			gen "\tdest->$_ = src->$usuc;\n";
}
}
@@@}
}

void edg_wll_SoapToJobStatFlags(const struct lbt__jobFlags *in, int *out)
{
	int		i;

	assert(in); assert(out);
	*out = 0;
	for ( i = 0; i < in->__sizeflag; i++ ) switch ( in->flag[i] )
	{
	case CLASSADS: *out |= EDG_WLL_STAT_CLASSADS; break;
	case CHILDREN: *out |= EDG_WLL_STAT_CHILDREN; break;
	case CHILDSTAT: *out |= EDG_WLL_STAT_CHILDSTAT; break;
	}
}

int edg_wll_JobStatFlagsToSoap(struct soap *soap, const int in, struct lbt__jobFlags *out)
{
	int		i = 0;

	assert(out);
	memset(out, 0, sizeof(*out));
	if ( in & EDG_WLL_STAT_CLASSADS ) out->__sizeflag++;
	if ( in & EDG_WLL_STAT_CHILDREN ) out->__sizeflag++;
	if ( in & EDG_WLL_STAT_CHILDSTAT ) out->__sizeflag++;
	if ( !out->__sizeflag ) return SOAP_OK;
	out->flag = soap_malloc(soap, sizeof(*(out->flag))*out->__sizeflag);
	if ( !out->flag ) return SOAP_FAULT;

	if ( in & EDG_WLL_STAT_CLASSADS ) out->flag[i++] = CLASSADS;
	if ( in & EDG_WLL_STAT_CHILDREN ) out->flag[i++] = CHILDREN;
	if ( in & EDG_WLL_STAT_CHILDSTAT ) out->flag[i++] = CHILDSTAT;

	return SOAP_OK;
}

void edg_wll_SoapToAttr(const enum lbt__queryAttr in, edg_wll_QueryAttr *out)
{
	switch ( in )
	{
//	case UNDEF: *out = EDG_WLL_QUERY_ATTR_UNDEF; break;
	case JOBID: *out = EDG_WLL_QUERY_ATTR_JOBID; break;
	case OWNER: *out = EDG_WLL_QUERY_ATTR_OWNER; break;
	case STATUS: *out = EDG_WLL_QUERY_ATTR_STATUS; break;
	case LOCATION: *out = EDG_WLL_QUERY_ATTR_LOCATION; break;
	case DESTINATION: *out = EDG_WLL_QUERY_ATTR_DESTINATION; break;
	case DONECODE: *out = EDG_WLL_QUERY_ATTR_DONECODE; break;
	case USERTAG: *out = EDG_WLL_QUERY_ATTR_USERTAG; break;
	case TIME: *out = EDG_WLL_QUERY_ATTR_TIME; break;
//	case LEVEL: *out = EDG_WLL_QUERY_ATTR_LEVEL; break;
	case HOST: *out = EDG_WLL_QUERY_ATTR_HOST; break;
	case SOURCE: *out = EDG_WLL_QUERY_ATTR_SOURCE; break;
	case INSTANCE: *out = EDG_WLL_QUERY_ATTR_INSTANCE; break;
	case EVENTTYPE: *out = EDG_WLL_QUERY_ATTR_EVENT_TYPE; break;
	case CHKPTTAG: *out = EDG_WLL_QUERY_ATTR_CHKPT_TAG; break;
	case RESUBMITTED: *out = EDG_WLL_QUERY_ATTR_RESUBMITTED; break;
	case PARENT: *out = EDG_WLL_QUERY_ATTR_PARENT; break;
	case EXITCODE: *out = EDG_WLL_QUERY_ATTR_EXITCODE; break;
	default: *out = in; break;
	}
}

void edg_wll_AttrToSoap(const edg_wll_QueryAttr in, enum lbt__queryAttr *out)
{
	switch ( in )
	{
	case EDG_WLL_QUERY_ATTR_UNDEF: *out = -1; break;
	case EDG_WLL_QUERY_ATTR_JOBID: *out = JOBID; break;
	case EDG_WLL_QUERY_ATTR_OWNER: *out = OWNER; break;
	case EDG_WLL_QUERY_ATTR_STATUS: *out = STATUS; break;
	case EDG_WLL_QUERY_ATTR_LOCATION: *out = LOCATION; break;
	case EDG_WLL_QUERY_ATTR_DESTINATION: *out = DESTINATION; break;
	case EDG_WLL_QUERY_ATTR_DONECODE: *out = DONECODE; break;
	case EDG_WLL_QUERY_ATTR_USERTAG: *out = USERTAG; break;
	case EDG_WLL_QUERY_ATTR_TIME: *out = TIME; break;
//	case EDG_WLL_QUERY_ATTR_LEVEL: *out = LEVEL; break;
	case EDG_WLL_QUERY_ATTR_HOST: *out = HOST; break;
	case EDG_WLL_QUERY_ATTR_SOURCE: *out = SOURCE; break;
	case EDG_WLL_QUERY_ATTR_INSTANCE: *out = INSTANCE; break;
	case EDG_WLL_QUERY_ATTR_EVENT_TYPE: *out = EVENTTYPE; break;
	case EDG_WLL_QUERY_ATTR_CHKPT_TAG: *out = CHKPTTAG; break;
	case EDG_WLL_QUERY_ATTR_RESUBMITTED: *out = RESUBMITTED; break;
	case EDG_WLL_QUERY_ATTR_PARENT: *out = PARENT; break;
	case EDG_WLL_QUERY_ATTR_EXITCODE: *out = EXITCODE; break;
	default: *out = in; break;
	}
}

void edg_wll_SoapToQueryOp(const enum lbt__queryOp in, edg_wll_QueryOp *out)
{
	switch ( in )
	{
	case EQUAL: *out = EDG_WLL_QUERY_OP_EQUAL; break;
	case LESS: *out = EDG_WLL_QUERY_OP_LESS; break;
	case GREATER: *out = EDG_WLL_QUERY_OP_GREATER; break;
	case WITHIN: *out = EDG_WLL_QUERY_OP_WITHIN; break;
	default: *out = in; break;
	}
}

void edg_wll_QueryOpToSoap(const edg_wll_QueryOp in, enum lbt__queryOp *out)
{
	switch ( in )
	{
	case EDG_WLL_QUERY_OP_EQUAL: *out = EQUAL; break;
	case EDG_WLL_QUERY_OP_LESS: *out = LESS; break;
	case EDG_WLL_QUERY_OP_GREATER: *out = GREATER; break;
	case EDG_WLL_QUERY_OP_WITHIN: *out = WITHIN; break;
	default: *out = in; break;
	}
}

int edg_wll_SoapToQueryVal(
	const edg_wll_QueryAttr				attr,
	const struct lbt__queryRecValue *in,
	union edg_wll_QueryVal			   *out)
{
	assert(in); assert(out);
	if ( in->c ) {
		if ( attr == EDG_WLL_QUERY_ATTR_JOBID || attr == EDG_WLL_QUERY_ATTR_PARENT ) {
			if ( edg_wlc_JobIdParse(in->c, &(out->j)) ) return -1;
		} else {
			if ( !(out->c = strdup(in->c)) ) return -1;
		}
	}
	else if ( in->t ) {
		out->t.tv_sec = in->t->tvSec;
		out->t.tv_usec = in->t->tvUsec;
	}
	else if ( in->i ) out->i = *(in->i);

	return 0;
}

int edg_wll_QueryValToSoap(
	struct soap					   *soap,
	const edg_wll_QueryAttr			attr,
	const union edg_wll_QueryVal   *in,
	struct lbt__queryRecValue   *out)
{
	assert(in); assert(out);
	memset(out, 0, sizeof(*out));
	switch ( attr ) {
	case EDG_WLL_QUERY_ATTR_USERTAG:
	case EDG_WLL_QUERY_ATTR_OWNER: 
	case EDG_WLL_QUERY_ATTR_LOCATION:
	case EDG_WLL_QUERY_ATTR_DESTINATION:
	case EDG_WLL_QUERY_ATTR_HOST:
	case EDG_WLL_QUERY_ATTR_INSTANCE:
		if ( in->c && !(out->c = soap_strdup(soap, in->c)) ) return SOAP_FAULT;
		break;
	case EDG_WLL_QUERY_ATTR_JOBID:
	case EDG_WLL_QUERY_ATTR_PARENT: {
			char *s = edg_wlc_JobIdUnparse(in->j);
			out->c = soap_strdup(soap, s);
			free(s);
			if ( !out->c ) return SOAP_FAULT;
		}
		break;
	case EDG_WLL_QUERY_ATTR_TIME:
		out->t = soap_malloc(soap, sizeof(*(out->t)));
		if ( !out->t ) return SOAP_FAULT;
		out->t->tvSec = in->t.tv_sec;
		out->t->tvUsec = in->t.tv_usec;
		break;
	case EDG_WLL_QUERY_ATTR_STATUS:
	case EDG_WLL_QUERY_ATTR_DONECODE:
	case EDG_WLL_QUERY_ATTR_LEVEL:
	case EDG_WLL_QUERY_ATTR_SOURCE:
	case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
	case EDG_WLL_QUERY_ATTR_RESUBMITTED:
	default:
		out->i = soap_malloc(soap, sizeof(*(out->i)));
		if ( !out->i ) return SOAP_FAULT;
		*(out->i) = in->i;
		break;
	}

	return SOAP_OK;
}

int edg_wll_SoapToQueryRec(
	const struct lbt__queryConditions *collection,
	const struct lbt__queryRecord  *in,
	edg_wll_QueryRec			   *out)
{
	assert(collection); assert(in); assert(out);
	memset(out, 0, sizeof(*out));
	edg_wll_SoapToAttr(collection->attr, &out->attr);
	switch ( out->attr )
	{
	case EDG_WLL_QUERY_ATTR_TIME:
		edg_wll_SoapToJobStatCode(*(collection->statName), &(out->attr_id.state));
		break;
	case EDG_WLL_QUERY_ATTR_USERTAG:
		out->attr_id.tag = strdup(collection->tagName);
		break;
	default:
		break;
	}
	edg_wll_SoapToQueryOp(in->op, &(out->op));
	switch ( out->op )
	{
	case EDG_WLL_QUERY_OP_WITHIN:
		if ( edg_wll_SoapToQueryVal(out->attr, in->value2, &(out->value2)) ) goto err;
	default:
		if ( edg_wll_SoapToQueryVal(out->attr, in->value1, &(out->value)) ) goto err;
		break;
	}

	return 0;
err:
	edg_wll_QueryRecFree(out);
	return -1;
}
#if 0

int edg_wll_QueryRecToSoap(
	struct soap				   *soap,
	const edg_wll_QueryRec	   *in,
	struct lbt__queryRecord	  **out)
{
	struct lbt__queryRecord	   *qr;


	assert(in); assert(out);
	if ( !(qr = soap_malloc(soap, sizeof(*qr))) ) return SOAP_FAULT;
	memset(qr, 0, sizeof(*qr));
	if ( !in ) goto ret;
	edg_wll_QueryOpToSoap(in->op, &(qr->op));
	if (   in->attr == EDG_WLL_QUERY_ATTR_TIME
		|| in->attr == EDG_WLL_QUERY_ATTR_USERTAG ) {
		if ( !(qr->attrid = soap_malloc(soap, sizeof(*(qr->attrid)))) ) return SOAP_FAULT;
		memset(qr->attrid, 0, sizeof(*(qr->attrid)));
		if ( in->attr == EDG_WLL_QUERY_ATTR_TIME ) {
			qr->attrid->state = soap_malloc(soap, sizeof(*(qr->attrid->state)));
			if ( !qr->attrid->state ) return SOAP_FAULT;
			edg_wll_JobStatCodeToSoap(in->attr_id.state, qr->attrid->state);
		}
		else {
			qr->attrid->tag = soap_strdup(soap, in->attr_id.tag);
			if ( !qr->attrid->tag ) return SOAP_FAULT;
		}
	}
	switch ( in->op ) {
	case EDG_WLL_QUERY_OP_WITHIN:
		if ( !(qr->value2 = soap_malloc(soap, sizeof(*(qr->value2)))) ) return SOAP_FAULT;
		if ( edg_wll_QueryValToSoap(soap, in->attr, &(in->value2), qr->value2) ) return SOAP_FAULT;
	default:
		if ( !(qr->value1 = soap_malloc(soap, sizeof(*(qr->value1)))) ) return SOAP_FAULT;
		if ( edg_wll_QueryValToSoap(soap, in->attr, &(in->value), qr->value1) ) return SOAP_FAULT;
		break;
	}

ret:
	*out = qr;
	return SOAP_OK;
}
#endif

/**
 * Translate query conditions from Soap form to query rec structure
 *
 * \param IN in pointer to soap query condition
 * \param OUT out target array of queryRec
 */
int edg_wll_SoapToQueryConds(const struct lbt__queryConditions *in, edg_wll_QueryRec **out)
{
	int					i;
	edg_wll_QueryRec   *qr;

	assert(in); assert(out);
	if ( !(qr = calloc(in->__sizerecord+1, sizeof(*qr))) ) goto err;
	for ( i = 0; i < in->__sizerecord; i++ )
		if ( edg_wll_SoapToQueryRec(in, in->record[i], qr+i) ) goto err;
	*out = qr;
	return 0;

err:
	if ( qr ) {
		for ( i = 0; qr[i].attr; i++ ) edg_wll_QueryRecFree(qr+i);
		free(qr);
	}
	return -1;
}

#if 0
int edg_wll_QueryCondsToSoap(
	struct soap					   *soap,
	const edg_wll_QueryRec		   *in,
	struct lbt__queryConditions **out)
{
	int								i;
	struct lbt__queryConditions  *qc;


	assert(out);
	if ( !(qc = soap_malloc(soap, sizeof(*qc))) ) return SOAP_FAULT;
	memset(qc, 0, sizeof(*qc));
	if ( !in ) goto ret;
	while ( in[qc->__sizerecords].attr ) qc->__sizerecords++;
	if ( !qc->__sizerecords ) goto ret;
	qc->records = soap_malloc(soap, sizeof(*(qc->records))*qc->__sizerecords);
	if ( !qc->records ) return SOAP_FAULT;
	edg_wll_AttrToSoap(in[0].attr, &(qc->attr));
	for ( i = 0; in[i].attr; i++ )
		if ( edg_wll_QueryRecToSoap(soap, in+i, qc->records+i) )
			return SOAP_FAULT;

ret:
	*out = qc;
	return SOAP_OK;
}
#endif

/**
 * Translate extended query conditions from Soap form to query rec structure
 *
 * \param IN in array of soap query condition
 * \param OUT out target array of queryRec
 */
int edg_wll_SoapToQueryCondsExt(const struct lbt__queryConditions *in, int __sizecondition, edg_wll_QueryRec ***out)
{
	int					i;
	edg_wll_QueryRec  **qr;


	assert(in); assert(out);
	if ( !(qr = calloc(__sizecondition, sizeof(*qr))) ) return -1;
	for ( i = 0; i < __sizecondition; i++ )
		if ( edg_wll_SoapToQueryConds(&in[i], qr+i) ) goto err;

	*out = qr;
	return 0;

err:
	if ( qr ) {
		int		j;
		for ( i = 0; qr[i]; i++ )
		{
			for ( j = 0; qr[i][j].attr; j++ ) edg_wll_QueryRecFree(qr[i]);
			free(qr[i]);
		}
		free(qr);
	}
	return -1;
}

#if 0
int edg_wll_QueryCondsExtToSoap(
	struct soap							   *soap,
	const edg_wll_QueryRec				  **in,
	struct lbt__queryConditions		  **out)
{
	int								i;
	struct lbt__queryConditions *qc;


	assert(out);
	if ( !(qc = soap_malloc(soap, sizeof(*qc))) ) return SOAP_FAULT;
	memset(qc, 0, sizeof(*qc));
	if ( !in ) goto ret;
	while ( in[qc->__sizecondition] ) qc->__sizecondition++;
	if ( !qc->__sizecondition ) goto ret;
	qc->condition = soap_malloc(soap, sizeof(*(qc->condition))*qc->__sizecondition);
	if ( !qc->condition ) return SOAP_FAULT;
	for ( i = 0; in[i]; i++ )
		if ( edg_wll_QueryCondsToSoap(soap, in[i], qc->condition+i) )
			return SOAP_FAULT;

ret:
	*out = qc;
	return SOAP_OK;
}
#endif

int edg_wll_JobsQueryResToSoap(
	struct soap           *soap,
	const edg_wlc_JobId   *jobs,
	const edg_wll_JobStat *states,
	struct _lbe__QueryJobsResponse *out)
{
	int							i;


	assert(out); //assert(out->states); assert(out->jobs);
	out->jobs = NULL;
	out->states = NULL;
	out->__sizejobs = 0;
	out->__sizestates = 0;

	if ( jobs ) {
		for ( i = 0; jobs[i]; i++ ) ;
		out->jobs = soap_malloc(soap, sizeof(*(out->jobs))*i);
		if ( !out->jobs ) return SOAP_FAULT;
		out->__sizejobs = i;
		for ( i = 0; jobs[i]; i++ ) {
			char	   *s;
			if ( !(s = edg_wlc_JobIdUnparse(jobs[i])) ) return SOAP_FAULT;
			if ( !(out->jobs[i] = soap_strdup(soap, s)) ) return SOAP_FAULT;
			free(s);
		}
	}
	if ( states ) {
		for ( i = 0; states[i].state; i++ ) ;
		out->states = soap_malloc(soap, sizeof(*(out->states))*i);
		if ( !out->states ) return SOAP_FAULT;
		out->__sizestates = i;

		for ( i = 0; states[i].state; i++ )
			if ( edg_wll_StatusToSoap(soap, states+i, out->states+i) )
				return SOAP_FAULT;
	}

	return SOAP_OK;
}
