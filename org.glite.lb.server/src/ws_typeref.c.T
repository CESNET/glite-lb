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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdsoap2.h>
#include <cclassad.h>

#include "soap_version.h"
#include "bk_ws_H.h"
#include "ws_typemap.h"

#include "glite/lb/events.h"
#include "glite/lb/query_rec.h"
/* XXX: references only, src and dest share pointers */

#include "glite/security/glite_gscompat.h"

#define VALUEEV_GET(X, V) GLITE_SECURITY_GSOAP_CHOICE_GET((X), V, event, 1)
#define VALUEEV_SET(X, V, VALUE) GLITE_SECURITY_GSOAP_CHOICE_SET((X), V, lbt, event, 1, VALUE)
#define VALUEEV_ISTYPE(X, V) GLITE_SECURITY_GSOAP_CHOICE_ISTYPE((X), V, lbt, event, 1)
#define VALUEQR_GET(X, V) GLITE_SECURITY_GSOAP_CHOICE_GET((X), V, queryRecValue, 2)
#define VALUEQR_SETTYPE(X, V) GLITE_SECURITY_GSOAP_CHOICE_SETTYPE((X), V, lbt, queryRecValue, 2)
#define VALUEQR_SET(X, V, VALUE) GLITE_SECURITY_GSOAP_CHOICE_SET((X), V, lbt, queryRecValue, 2, VALUE)
#define VALUEQR_ISTYPE(X, V) GLITE_SECURITY_GSOAP_CHOICE_ISTYPE((X), V, lbt, queryRecValue, 2)

@@@{
#
# generate the command for move native data to soap web services structures
#
# if the field is optional, is ALWAYS assigned!
#
sub eventFieldAssign {
	my ($indent, $src, $dst, $e, $f, $is_common) = @_;
	my ($tn, $soap_en, $soap_fn, $native_en, $native_fn, $soap_En, $soap_Fn, $func_name, $source, $dest);

	$tn = $f->{type};
	$native_en = lcfirst $e;
	if ($native_en =~ m/^pBS/) { $native_en = ucfirst $native_en; }
	if ($native_en =~ m/^cREAM/) { $native_en = ucfirst $native_en; }
	if ($native_en =~ m/^condor/) { $native_en = ucfirst $native_en; }
	$native_fn = $f->{name};
	$soap_en = $e;
	while ($soap_en =~ /([[:alpha:]]*)_([[:alpha:]_]*)/) {
		$soap_en = $1.ucfirst($2);
	};
	$soap_fn = $native_fn;
	while ($soap_fn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/) {
		$soap_fn = $1.ucfirst($2);
	};
	$source = "$src->$native_en.$native_fn";
	$dest = "VALUEEV_GET($dst, $soap_en)->$soap_fn";
	print STDERR "$src, $dst, $soap_en, $soap_fn, $native_fn, $tn, $usuc\n";

	if ($tn eq 'int') {
		if ($f->{codes}) {
			$soap_En = ucfirst $soap_en;
			$soap_Fn = ucfirst $soap_fn;
			$func_name = $is_common ? "edg_wll_${soap_Fn}ToSoap" : "edg_wll_${soap_En}${soap_Fn}ToSoap";
			gen $indent."$func_name(soap, $source, &$dest);\n";
		} elsif ($f->{optional}) {
			gen $indent."$dest = soap_malloc(soap, sizeof(*$dest));\n";
			gen $indent."*$dest = $source;\n";
		} else {
			gen $indent."$dest = $source;\n";
		}
	} elsif ($tn eq 'port' || $tn eq 'bool' || $tn eq 'float' || $tn eq 'double') {
		if ($f->{optional}) {
			gen $indent."$dest = soap_malloc(soap, sizeof(*$dest));\n";
			gen $indent."*$dest = $source;\n";
		} else {
			gen $indent."$dest = $source;\n";
		}
	} elsif ($tn eq 'logsrc') {
			gen $indent."edg_wll_SourceToSoap($source, &$dest);\n";
	} elsif ($tn eq 'string' || $tn eq 'notifid') {
			gen $indent."$dest = soap_strdup(soap, $source);\n";
	} elsif ($tn eq 'jobstat') {
			gen $indent."$dest = soap_malloc(soap, sizeof(struct lbt__jobStatus))";
			gen $indent."edg_wll_StatusToSoap(soap, $source, $dest);\n";
	} elsif ($tn eq 'timeval') {
			gen $indent."$dest = soap_malloc(soap, sizeof(struct lbt__timeval));\n";
			gen $indent."$dest->tvSec = $source.tv_sec;\n";
			gen $indent."$dest->tvUsec = $source.tv_usec;\n";
	} elsif ($tn eq 'jobid') {
			gen $indent."s = edg_wlc_JobIdUnparse($source);\n";
			gen $indent."$dest = soap_strdup(soap, s);\n";
	} elsif ($tn eq 'usertag') {
			gen $indent."$dest = soap_malloc(soap, sizeof(*$dst->$soap_fn));\n";
			gen $indent."$dest->tag = soap_strdup(soap, $source.tag);\n";
			gen $indent."$dest->value = soap_strdup(soap, $source.value);\n";
	} elsif ($tn eq 'taglist') {
			gen qq{
!	{
!		struct lbt__tagValue	*t;
!               int i;
!
!		for (i=0; $source && $source\[i].tag; i++);
!		/* GLITE_SECURITY_GSOAP_LIST_CREATE(soap, $dst, $soap_en.$soap_fn, struct lbt__tagValue, i); */
!		GLITE_SECURITY_GSOAP_LIST_CREATE0(soap, $dest, VALUEEV_GET($dst,$soap_en)->__size$soap_fn, struct lbt__tagValue, i);
!		for (i=0; $source && $source\[i].tag; i++) {
!			t = GLITE_SECURITY_GSOAP_LIST_GET($dest, i);
!			t->tag = soap_strdup(soap,$source\[i].tag);
!			t->value = soap_strdup(soap,$source\[i].value);
!		}
!	}
};

	} else {
		die "Unknown type $tn";
	}
}


sub eventFieldFree {
	my ($indent, $dst, $e, $f) = @_;
	my ($tn, $soap_en, $soap_fn, $dest);

	$tn = $f->{type};
	$soap_en = $e;
	while ($soap_en =~ /([[:alpha:]]*)_([[:alpha:]_]*)/) {
		$soap_en = $1.ucfirst($2);
	};
	$soap_fn = $f->{name};
	while ($soap_fn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/) {
		$soap_fn = $1.ucfirst($2);
	};
	$dest = "VALUEEV_GET($dst, $soap_en)->$soap_fn";

	if ($tn eq 'int' || $tn eq 'port' || $tn eq 'bool' || $tn eq 'float' || $tn eq 'double' || $tn eq 'logsrc') {
		if ($f->{optional}) {
			gen $indent."if ($dest) soap_dealloc(soap, $dest);\n";
		}
	} elsif ($tn eq 'string' || $tn eq 'notifid' || $tn eq 'jobid' || $tn eq 'timeval') {
		gen $indent."if ($dest) soap_dealloc(soap, $dest);\n";
	} elsif ($tn eq 'usertag') {
		gen $indent."if ($dest) {\n";
		gen $indent."	if ($dest->tag) soap_dealloc(soap, $dest->tag);\n";
		gen $indent."	if ($dest->value) soap_dealloc(soap, $dest->value);\n";
		gen $indent."	soap_dealloc(soap, $dest);\n";
		gen $indent."}\n";
	} elsif ($tn eq 'taglist') {
		gen qq{
!               if($dest) {
!			struct lbt__tagValue *t;
!			int i;
!
!			for(i=0; i < VALUEEV_GET($dst, $soap_en)->__size$soap_fn; i++) {
!				t = GLITE_SECURITY_GSOAP_LIST_GET($dest, i);
!				if(t->tag) soap_dealloc(soap, t->tag);
!				if(t->value) soap_dealloc(soap, t->value);
!			}
!			GLITE_SECURITY_GSOAP_LIST_DESTROY0(soap, $dest, VALUEEV_GET($dst, $soap_en)->__size$soap_fn);
!		}
		};
	} else {
		die "Unknown type $tn";
	}
}
@@@}

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

int edg_wll_StatusToSoap(struct soap *soap,edg_wll_JobStat const *src,struct lbt__jobStatus *js)
{
	int	i,j;
	char	*s;

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
!		GLITE_SECURITY_GSOAP_LIST_TYPE(lbt, ${usuc}Item) h;
!
!		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, js, ${usuc}, struct lbt__${usuc}Item, EDG_WLL_NUMBER_OF_STATCODES-1);
!		h = js->${usuc};
!		for (i=1; i<EDG_WLL_NUMBER_OF_STATCODES; i++) {
!			edg_wll_JobStatCodeToSoap(i,&GLITE_SECURITY_GSOAP_LIST_GET(h, i-1)->state);
!			GLITE_SECURITY_GSOAP_LIST_GET(h, i-1)->$fmap{$_} = src->$_\[i+1];
!		}
!	}
!	else {
!		js->__size$usuc = 0;
!		js->$usuc = NULL;
!	}
};
		} elsif ($ft eq 'taglist') {
			gen qq{
!	{
!		struct lbt__tagValue	*t;
!
!		for (i=0; src->$_ && src->$_\[i].tag; i++);
!		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, js, $usuc, struct lbt__tagValue, i);
!		for (i=0; src->$_ && src->$_\[i].tag; i++) {
!			t = GLITE_SECURITY_GSOAP_LIST_GET(js->$usuc, i);
!			t->tag = soap_strdup(soap,src->$_\[i].tag);
!			t->value = soap_strdup(soap,src->$_\[i].value);
!		}
!	}
};
		} elsif ($ft eq 'stslist') {
			gen qq{
!	for (i=0; src->$_ && src->$_\[i].state; i++);
!	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, js, $usuc, struct lbt__jobStatus, i);
!	for (j=0; j<i; j++) edg_wll_StatusToSoap(soap,src->$_+j,GLITE_SECURITY_GSOAP_LIST_GET(js->$usuc, j));
};
		} elsif ($ft eq 'timeval') {
			gen "\tjs->$usuc = soap_malloc(soap,sizeof *js->$usuc);\n";
			gen "\tjs->$usuc->tvSec = src->$_.tv_sec;\n";
			gen "\tjs->$usuc->tvUsec = src->$_.tv_usec;\n";
		} elsif ($ft eq 'string') {
			gen "\tjs->$usuc = soap_strdup(soap,src->$_);\n";
		} elsif ($ft eq 'cclassad') {
			gen "\ts = src->$_ ? cclassad_unparse(src->$_) : NULL;\n";
			gen "\tjs->$usuc = s ? soap_strdup(soap, s) : NULL;\n";
			gen "\tfree(s);\n";
		} elsif ($f->{null}) {
			gen qq{
!	if (src->$_ == $f->{null}) js->$usuc = NULL;
!	else {
!		js->$usuc = soap_malloc(soap,sizeof *js->$usuc);
!		*js->$usuc = src->$_;
!	}
};
		} else { gen "\tjs->$usuc = src->$_;\n"; }
	}
@@@}

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
!		dest->$_ = calloc(src->__size$usuc + 1, sizeof *dest->$_);
!		dest->$_\[0] = src->__size$usuc + 2;
!		for (i=0; i<src->__size$usuc; i++) {
!			edg_wll_JobStatCode	s;
!			edg_wll_SoapToJobStatCode(GLITE_SECURITY_GSOAP_LIST_GET(src->$usuc, i)->state,&s);
!			assert(s>=0); assert(s<EDG_WLL_NUMBER_OF_STATCODES);
!			dest->$_\[s+1] = GLITE_SECURITY_GSOAP_LIST_GET(src->$usuc, i)->$fmap{$_};
!		}
!	}
!	else dest->$_ = NULL;
};
		} elsif ($ft eq 'stslist') {
			gen qq{
!	if (src->__size$usuc) {
!		dest->$_ = calloc(src->__size$usuc + 1,sizeof *dest->$_);
!		for (i=0; i<src->__size$usuc; i++) edg_wll_SoapToStatus(soap,GLITE_SECURITY_GSOAP_LIST_GET(src->$usuc, i),dest->$_ + i);
!	}
!	else dest->$_ = NULL;
};
 		} elsif ($ft eq 'taglist') {
			gen qq{
	if (src->__size$usuc) {
		dest->$_ = calloc(src->__size$usuc + 1,sizeof *dest->$_);
		for (i=0; i<src->__size$usuc; i++) {
			soap_unlink(soap,dest->$_\[i].tag = GLITE_SECURITY_GSOAP_LIST_GET(src->$usuc, i)->tag);
			soap_unlink(soap,dest->$_\[i].tag = GLITE_SECURITY_GSOAP_LIST_GET(src->$usuc, i)->tag);
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
		} elsif ($ft eq 'cclassad') {
			gen "\tif (src->$usuc) {";
			gen "\t\tdest->$_ = cclassad_create(src->$usuc);\n";
			gen "\t\tsoap_unlink(soap,src->$usuc);\n";
			gen "\t}";
		} elsif ($f->{null}) {
			gen "\tdest->$_ = (src->$usuc) ? *(src->$usuc) : $f->{null};\n";
		} else {
			gen "\tdest->$_ = src->$usuc;\n";
}
}
@@@}
}

void edg_wll_SoapToJobStatFlags(const struct lbt__jobFlags *in, int *out)
{
	int		i;

	assert(out);
	*out = 0;
	if (in) for ( i = 0; i < in->__sizeflag; i++ ) switch ( in->flag[i] )
	{
	case CLASSADS: *out |= EDG_WLL_STAT_CLASSADS; break;
	case CHILDREN: *out |= EDG_WLL_STAT_CHILDREN; break;
	case CHILDSTAT: *out |= EDG_WLL_STAT_CHILDSTAT; break;
	case CHILDHIST_FAST: *out |= EDG_WLL_STAT_CHILDHIST_FAST; break;
	case CHILDHIST_THOROUGH: *out |= EDG_WLL_STAT_CHILDHIST_THOROUGH; break;
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
	if ( in & EDG_WLL_STAT_CHILDHIST_FAST ) out->__sizeflag++;
	if ( in & EDG_WLL_STAT_CHILDHIST_THOROUGH ) out->__sizeflag++;
	if ( !out->__sizeflag ) return SOAP_OK;
	out->flag = soap_malloc(soap, sizeof(*(out->flag))*out->__sizeflag);
	if ( !out->flag ) return SOAP_FAULT;

	if ( in & EDG_WLL_STAT_CLASSADS ) out->flag[i++] = CLASSADS;
	if ( in & EDG_WLL_STAT_CHILDREN ) out->flag[i++] = CHILDREN;
	if ( in & EDG_WLL_STAT_CHILDSTAT ) out->flag[i++] = CHILDSTAT;
	if ( in & EDG_WLL_STAT_CHILDHIST_FAST ) out->flag[i++] = CHILDHIST_FAST;
	if ( in & EDG_WLL_STAT_CHILDHIST_THOROUGH ) out->flag[i++] = CHILDHIST_THOROUGH;

	return SOAP_OK;
}

int edg_wll_SoapToAttr(const enum lbt__queryAttr in, edg_wll_QueryAttr *out)
{
	assert(EDG_WLL_QUERY_ATTR_NETWORK_SERVER + 1 == EDG_WLL_QUERY_ATTR__LAST);

	switch ( in )
	{
	case JOBID: *out = EDG_WLL_QUERY_ATTR_JOBID; break;
	case OWNER: *out = EDG_WLL_QUERY_ATTR_OWNER; break;
	case STATUS: *out = EDG_WLL_QUERY_ATTR_STATUS; break;
	case LOCATION: *out = EDG_WLL_QUERY_ATTR_LOCATION; break;
	case DESTINATION: *out = EDG_WLL_QUERY_ATTR_DESTINATION; break;
	case DONECODE: *out = EDG_WLL_QUERY_ATTR_DONECODE; break;
	case USERTAG: *out = EDG_WLL_QUERY_ATTR_USERTAG; break;
	case TIME: *out = EDG_WLL_QUERY_ATTR_TIME; break;
	case LEVEL: *out = EDG_WLL_QUERY_ATTR_LEVEL; break;
	case HOST: *out = EDG_WLL_QUERY_ATTR_HOST; break;
	case SOURCE: *out = EDG_WLL_QUERY_ATTR_SOURCE; break;
	case INSTANCE: *out = EDG_WLL_QUERY_ATTR_INSTANCE; break;
	case EVENTTYPE: *out = EDG_WLL_QUERY_ATTR_EVENT_TYPE; break;
	case CHKPTTAG: *out = EDG_WLL_QUERY_ATTR_CHKPT_TAG; break;
	case RESUBMITTED: *out = EDG_WLL_QUERY_ATTR_RESUBMITTED; break;
	case PARENT: *out = EDG_WLL_QUERY_ATTR_PARENT; break;
	case EXITCODE: *out = EDG_WLL_QUERY_ATTR_EXITCODE; break;
	case JDLATTR: *out = EDG_WLL_QUERY_ATTR_JDL_ATTR; break;
	case STATEENTERTIME: *out = EDG_WLL_QUERY_ATTR_STATEENTERTIME; break;
	case LASTUPDATETIME: *out = EDG_WLL_QUERY_ATTR_LASTUPDATETIME; break;
	case NETWORKSERVER: *out = EDG_WLL_QUERY_ATTR_NETWORK_SERVER; break;
	case JOBTYPE: *out = EDG_WLL_QUERY_ATTR_JOB_TYPE; break;
	default: return 0;
	}

	return 1;
}

int edg_wll_AttrToSoap(const edg_wll_QueryAttr in, enum lbt__queryAttr *out)
{
	assert(EDG_WLL_QUERY_ATTR_NETWORK_SERVER + 1 == EDG_WLL_QUERY_ATTR__LAST);

	switch ( in )
	{
	case EDG_WLL_QUERY_ATTR_JOBID: *out = JOBID; break;
	case EDG_WLL_QUERY_ATTR_OWNER: *out = OWNER; break;
	case EDG_WLL_QUERY_ATTR_STATUS: *out = STATUS; break;
	case EDG_WLL_QUERY_ATTR_LOCATION: *out = LOCATION; break;
	case EDG_WLL_QUERY_ATTR_DESTINATION: *out = DESTINATION; break;
	case EDG_WLL_QUERY_ATTR_DONECODE: *out = DONECODE; break;
	case EDG_WLL_QUERY_ATTR_USERTAG: *out = USERTAG; break;
	case EDG_WLL_QUERY_ATTR_TIME: *out = TIME; break;
	case EDG_WLL_QUERY_ATTR_LEVEL: *out = LEVEL; break;
	case EDG_WLL_QUERY_ATTR_HOST: *out = HOST; break;
	case EDG_WLL_QUERY_ATTR_SOURCE: *out = SOURCE; break;
	case EDG_WLL_QUERY_ATTR_INSTANCE: *out = INSTANCE; break;
	case EDG_WLL_QUERY_ATTR_EVENT_TYPE: *out = EVENTTYPE; break;
	case EDG_WLL_QUERY_ATTR_CHKPT_TAG: *out = CHKPTTAG; break;
	case EDG_WLL_QUERY_ATTR_RESUBMITTED: *out = RESUBMITTED; break;
	case EDG_WLL_QUERY_ATTR_PARENT: *out = PARENT; break;
	case EDG_WLL_QUERY_ATTR_EXITCODE: *out = EXITCODE; break;
	case EDG_WLL_QUERY_ATTR_JDL_ATTR: *out = JDLATTR; break;
	case EDG_WLL_QUERY_ATTR_STATEENTERTIME: *out = STATEENTERTIME; break;
	case EDG_WLL_QUERY_ATTR_LASTUPDATETIME: *out = LASTUPDATETIME; break;
	case EDG_WLL_QUERY_ATTR_NETWORK_SERVER: *out = NETWORKSERVER; break;
	case EDG_WLL_QUERY_ATTR_JOB_TYPE: *out = JOBTYPE; break;
	default: return 0;
	}

	return 1;
}

void edg_wll_SoapToQueryOp(const enum lbt__queryOp in, edg_wll_QueryOp *out)
{
	switch ( in )
	{
	case EQUAL: *out = EDG_WLL_QUERY_OP_EQUAL; break;
	case UNEQUAL: *out = EDG_WLL_QUERY_OP_UNEQUAL; break;
	case LESS: *out = EDG_WLL_QUERY_OP_LESS; break;
	case GREATER: *out = EDG_WLL_QUERY_OP_GREATER; break;
	case WITHIN: *out = EDG_WLL_QUERY_OP_WITHIN; break;
	case CHANGED: *out = EDG_WLL_QUERY_OP_CHANGED; break;
	}
}

void edg_wll_QueryOpToSoap(const edg_wll_QueryOp in, enum lbt__queryOp *out)
{
	switch ( in )
	{
	case EDG_WLL_QUERY_OP_EQUAL: *out = EQUAL; break;
	case EDG_WLL_QUERY_OP_UNEQUAL: *out = UNEQUAL; break;
	case EDG_WLL_QUERY_OP_LESS: *out = LESS; break;
	case EDG_WLL_QUERY_OP_GREATER: *out = GREATER; break;
	case EDG_WLL_QUERY_OP_WITHIN: *out = WITHIN; break;
	case EDG_WLL_QUERY_OP_CHANGED: *out = CHANGED; break;
	default: assert(0);
	}
}

int edg_wll_SoapToQueryVal(
	const edg_wll_QueryAttr				attr,
	const struct lbt__queryRecValue *in,
	union edg_wll_QueryVal			   *out)
{
	assert(in); assert(out);
	if ( VALUEQR_ISTYPE(in, c) ) {
		if ( attr == EDG_WLL_QUERY_ATTR_JOBID || attr == EDG_WLL_QUERY_ATTR_PARENT ) {
			if ( glite_jobid_parse(VALUEQR_GET(in, c), (glite_jobid_t *)&(out->j)) ) return -1;
		} else {
			if ( !(out->c = strdup(VALUEQR_GET(in, c))) ) return -1;
		}
	}
	else if ( VALUEQR_ISTYPE(in, t) ) {
		out->t.tv_sec = VALUEQR_GET(in, t)->tvSec;
		out->t.tv_usec = VALUEQR_GET(in, t)->tvUsec;
	}
	else if ( VALUEQR_ISTYPE(in, i) ) out->i = *(VALUEQR_GET(in, i));

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
	case EDG_WLL_QUERY_ATTR_CHKPT_TAG:
	case EDG_WLL_QUERY_ATTR_JDL_ATTR:
		VALUEQR_SET(out, c, in->c ? soap_strdup(soap, in->c) : NULL);
		if (in->c && !VALUEQR_GET(out, c) ) return SOAP_FAULT;
		break;
	case EDG_WLL_QUERY_ATTR_JOBID:
	case EDG_WLL_QUERY_ATTR_PARENT: {
			char *s = edg_wlc_JobIdUnparse(in->j);
			VALUEQR_SET(out, c, soap_strdup(soap, s));
			free(s);
			if ( !VALUEQR_GET(out, c) ) return SOAP_FAULT;
		}
		break;
	case EDG_WLL_QUERY_ATTR_TIME:
	case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
	case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
		VALUEQR_SET(out, t, soap_malloc(soap, sizeof(*(VALUEQR_GET(out, t)))));
		if ( !VALUEQR_GET(out, t) ) return SOAP_FAULT;
		VALUEQR_SETTYPE(out, t);
		VALUEQR_GET(out, t)->tvSec = in->t.tv_sec;
		VALUEQR_GET(out, t)->tvUsec = in->t.tv_usec;
		break;
	case EDG_WLL_QUERY_ATTR_STATUS:
	case EDG_WLL_QUERY_ATTR_DONECODE:
	case EDG_WLL_QUERY_ATTR_LEVEL:
	case EDG_WLL_QUERY_ATTR_SOURCE:
	case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
	case EDG_WLL_QUERY_ATTR_RESUBMITTED:
	case EDG_WLL_QUERY_ATTR_EXITCODE:
	case EDG_WLL_QUERY_ATTR_JOB_TYPE:
		VALUEQR_SET(out, i, soap_malloc(soap, sizeof(*(VALUEQR_GET(out, i)))));
		if ( !VALUEQR_GET(out, i) ) return SOAP_FAULT;
		*(VALUEQR_GET(out, i)) = in->i;
		break;
	default:
		return SOAP_FAULT;
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
	if (!edg_wll_SoapToAttr(collection->attr, &out->attr)) return -1;
	switch ( out->attr )
	{
	case EDG_WLL_QUERY_ATTR_TIME:
		edg_wll_SoapToJobStatCode(*(collection->statName), &(out->attr_id.state));
		break;
	case EDG_WLL_QUERY_ATTR_USERTAG:
	case EDG_WLL_QUERY_ATTR_JDL_ATTR:
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

int edg_wll_QueryRecToSoap(
	struct soap			*soap,
	const edg_wll_QueryRec	   	*in,
	struct lbt__queryRecord *qr)
{
	assert(in); assert(qr);
	memset(qr, 0, sizeof(*qr));
	if ( !in ) goto ret;
	edg_wll_QueryOpToSoap(in->op, &(qr->op));

#ifdef GLITE_SECURITY_GSOAP_CHOICE_SETNULL
	if ((qr->value2 = soap_malloc(soap, sizeof(*(qr->value2)))) == NULL) return SOAP_FAULT;
	GLITE_SECURITY_GSOAP_CHOICE_SETNULL(qr->value2,  queryRecValue);
#endif
	switch ( in->op ) {
	case EDG_WLL_QUERY_OP_WITHIN:
#ifndef GLITE_SECURITY_GSOAP_CHOICE_SETNULL
		if ( !(qr->value2 = soap_malloc(soap, sizeof(*(qr->value2)))) ) return SOAP_FAULT;
#endif
		if ( edg_wll_QueryValToSoap(soap, in->attr, &(in->value2), qr->value2) ) return SOAP_FAULT;
	default:
		if ( !(qr->value1 = soap_malloc(soap, sizeof(*(qr->value1)))) ) return SOAP_FAULT;
		if ( edg_wll_QueryValToSoap(soap, in->attr, &(in->value), qr->value1) ) return SOAP_FAULT;
		break;
	}

ret:
	return SOAP_OK;
}

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
		if ( edg_wll_SoapToQueryRec(in, GLITE_SECURITY_GSOAP_LIST_GET(in->record, i), qr+i) ) goto err;
	*out = qr;
	return 0;

err:
	if ( qr ) {
		for ( i = 0; qr[i].attr; i++ ) edg_wll_QueryRecFree(qr+i);
		free(qr);
	}
	return -1;
}

int edg_wll_QueryCondsToSoap(
	struct soap			*soap,
	const edg_wll_QueryRec		*in,
	struct lbt__queryConditions		*qc)
{
	int				i;


	assert(qc);
	memset(qc, 0, sizeof(*qc));
	if ( !in )
		goto ret;

	if (!edg_wll_AttrToSoap(in[0].attr, &(qc->attr))) return SOAP_FAULT;

	if ( in->attr == EDG_WLL_QUERY_ATTR_TIME ) {
		qc->statName = soap_malloc(soap, sizeof(*(qc->statName)));
		if ( !qc->statName ) return SOAP_FAULT;
		edg_wll_JobStatCodeToSoap(in->attr_id.state, qc->statName);
	}
	else if ( in->attr == EDG_WLL_QUERY_ATTR_USERTAG) {
		qc->tagName = soap_strdup(soap, in->attr_id.tag);
		if ( !qc->tagName ) return SOAP_FAULT;
	}

	i = 0;
	while ( in[i].attr ) i++;

	if ( !i )
		goto ret;

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, qc, record, struct lbt__queryRecord, i);
	if ( !qc->record )
		return SOAP_FAULT;

	for ( i = 0; in[i].attr; i++ )
		if ( edg_wll_QueryRecToSoap(soap, &in[i], GLITE_SECURITY_GSOAP_LIST_GET(qc->record, i)) )
			return SOAP_FAULT;

ret:
	return SOAP_OK;
}

/**
 * Translate extended query conditions from Soap form to query rec structure
 *
 * \param IN in array of soap query condition
 * \param OUT out target array of queryRec
 */
int edg_wll_SoapToQueryCondsExt(GLITE_SECURITY_GSOAP_LIST_TYPE(lbt, queryConditions) in, int __sizecondition, edg_wll_QueryRec ***out)
{
	int					i;
	edg_wll_QueryRec  **qr;

	assert(out);
	if ( !(qr = calloc(__sizecondition + 1, sizeof(*qr))) ) return -1;
	for ( i = 0; i < __sizecondition && GLITE_SECURITY_GSOAP_LIST_GET(in, i); i++ )
		if ( edg_wll_SoapToQueryConds(GLITE_SECURITY_GSOAP_LIST_GET(in, i), qr+i) ) goto err;

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


/**
 * Translate extended query conditions from Soap form to query rec structure
 *
 * \param IN in target array of queryRec 
 * \param OUT out_size number of array elements
 * \param OUT out array of soap query condition
 */
int edg_wll_QueryCondsExtToSoap(
	struct soap			*soap,
	const edg_wll_QueryRec		**in,
	int				*out_size,
	GLITE_SECURITY_GSOAP_LIST_TYPE(lbt, queryConditions) *out)
{
	int				i, size;
	GLITE_SECURITY_GSOAP_LIST_TYPE(lbt, queryConditions) qc;
	assert(in); assert(out);

	size = 0;
	while ( in[size] ) size++;

	if ( !size ) 
		goto ret;
	GLITE_SECURITY_GSOAP_LIST_CREATE0(soap, qc, size, struct lbt__queryConditions, size);
	if ( !qc ) return SOAP_FAULT;

	for ( i = 0; in[i]; i++ )
		if ( edg_wll_QueryCondsToSoap(soap, in[i], GLITE_SECURITY_GSOAP_LIST_GET(qc, i)) )
			return SOAP_FAULT;

ret:
	*out = qc;
	*out_size = size;
	return SOAP_OK;
}


#define edg_wll_CommonJobsResponseToSoap do { \
	int	count, i; \
\
	assert(out); \
	out->__sizejobs = 0; \
	out->jobs = NULL; \
	out->__sizestates = 0; \
	out->states = NULL; \
\
	count = 0; \
	if ( jobs ) { \
		for ( i = 0; jobs[i]; i++ ) ; \
		count = i; \
		out->jobs = soap_malloc(soap, sizeof(*(out->jobs))*count); \
		if ( !out->jobs ) return SOAP_FAULT; \
		out->__sizejobs = count; \
		for ( i = 0; jobs[i]; i++ ) { \
			char	   *s; \
			if ( !(s = edg_wlc_JobIdUnparse(jobs[i])) ) return SOAP_FAULT; \
			if ( !(out->jobs[i] = soap_strdup(soap, s)) ) return SOAP_FAULT; \
			free(s); \
		} \
	} \
	if ( states ) { \
		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, states, struct lbt__jobStatus, count); \
		if ( !out->states ) return SOAP_FAULT; \
\
		for ( i = 0; i < count; i++ ) \
			if ( edg_wll_StatusToSoap(soap, states+i, GLITE_SECURITY_GSOAP_LIST_GET(out->states, i)) ) \
				return SOAP_FAULT; \
	} \
\
} while(0)


int edg_wll_JobsQueryResToSoap(
	struct soap           *soap,
	const edg_wlc_JobId   *jobs,
	const edg_wll_JobStat *states,
	struct _lbe__QueryJobsResponse *out)
{
	edg_wll_CommonJobsResponseToSoap;
	return SOAP_OK;
}


int edg_wll_UserJobsResToSoap(
	struct soap           *soap,
	const edg_wlc_JobId   *jobs,
	const edg_wll_JobStat *states,
	struct _lbe__UserJobsResponse *out)
{
	edg_wll_CommonJobsResponseToSoap;
	return SOAP_OK;
}


static void edg_wll_SoapToSource(enum lbt__eventSource in, edg_wll_Source *out)
{
	assert(EDG_WLL_SOURCE_PBS_SCHEDULER + 1 == EDG_WLL_SOURCE__LAST);

	switch ( in ) {
        case UserInterface: *out = EDG_WLL_SOURCE_USER_INTERFACE; break;
        case NetworkServer: *out = EDG_WLL_SOURCE_NETWORK_SERVER; break;
        case WorkloadManager: *out = EDG_WLL_SOURCE_WORKLOAD_MANAGER; break;
        case BigHelper: *out = EDG_WLL_SOURCE_BIG_HELPER; break;
        case JobSubmission: *out = EDG_WLL_SOURCE_JOB_SUBMISSION; break;  
        case LogMonitor: *out = EDG_WLL_SOURCE_LOG_MONITOR; break;
        case LRMS: *out = EDG_WLL_SOURCE_LRMS; break;
        case Application: *out = EDG_WLL_SOURCE_APPLICATION; break;
        case LBServer: *out = EDG_WLL_SOURCE_LB_SERVER; break;
        case CREAMInterface: *out = EDG_WLL_SOURCE_CREAM_INTERFACE; break;
        case CREAMExecutor: *out = EDG_WLL_SOURCE_CREAM_EXECUTOR; break;
	case PBSClient: *out = EDG_WLL_SOURCE_PBS_CLIENT; break;
	case PBSServer: *out = EDG_WLL_SOURCE_PBS_SERVER; break;
        case PBSsMom: *out = EDG_WLL_SOURCE_PBS_SMOM; break;
        case PBSMom: *out = EDG_WLL_SOURCE_PBS_MOM; break;
        case PBSScheduler: *out = EDG_WLL_SOURCE_PBS_SCHEDULER; break;
        default: assert(0); break;
        }
}


/*
 * Soap to C and C to Soap enumeration translate functions.
 */

@@@{
foreach ('_common_', sort { $event->{order}->{$a} <=> $event->{order}->{$b} } getTypes $event) {
	my ($e, $upper_en, $soap_en, $en);

	$e = $_;
	if ($e eq '_common_') {
		$en = '';
		$upper_en = '';
		$soap_en = '';
	} else {
		$en = ucfirst $e;
		$upper_en = uc $e;
		$soap_en = $en;
		$soap_en = $1.ucfirst $2 while $soap_en =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
	}

	selectType $event $_;
	foreach ($event->getFieldsOrdered) {
		my $f = selectField $event $_;

		if ($f->{codes}) {
			my ($fn, $soap_fn, $upper_fn, $enum_name, $soap_enum_name, $enum_value_prefix, $func2C_name, $func2S_name);

			$fn = ucfirst $f->{name};
			$upper_fn = uc $f->{name};
			$soap_fn = $fn;
			$soap_fn = $1.ucfirst $2 while $soap_fn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;

			$enum_name = $e eq '_common_' && $fn eq 'Level' ? "int": "enum edg_wll_${en}${fn}";
			$enum_value_prefix = $e eq '_common_' ? "EDG_WLL_${upper_fn}_" : "EDG_WLL_${upper_en}_";
			$soap_enum_name = "enum lbt__$soap_en$soap_fn ";
			if ($f->{optional}) {$soap_enum_name .= "*";}

			# Soap to C function
			$func2C_name = "edg_wll_SoapTo$soap_en$soap_fn";
			gen "\n";
			gen "static void $func2C_name(const ${soap_enum_name}in, $enum_name *out) {\n";
			if ($f->{optional}) {
				gen "\tif (in == NULL) {\n";
				gen "\t\t*out = ${enum_value_prefix}UNDEFINED;\n";
				gen "\t\treturn;\n";
				gen "\t}\n";
				gen "\n";
				gen "\tswitch(*in) {\n";
			} else {
				gen "\tswitch(in) {\n";
			}
			foreach $code (@{$f->{codes}}) {
				my ($enum_value, $soap_enum_value, $upper_cn);

				$upper_cn = $code->{name};
				$enum_value = "${enum_value_prefix}${upper_cn}";
				$upper_cn =~ s/_/_USCORE/g;
				$soap_enum_value = "lbt__${soap_en}${soap_fn}__${upper_cn}";
				gen "\tcase $soap_enum_value: *out = $enum_value; break;\n"
			}
			gen "\tdefault: assert(0);\n";
			gen "\t};\n";
			gen "}\n";
			gen "\n";

			# C to Soap function
			my $target;

			$func2S_name = "edg_wll_${soap_en}${soap_fn}ToSoap";
			gen "\n";
			gen "static void $func2S_name(struct soap *soap, const $enum_name in, ${soap_enum_name}*out) {\n";
			if ($f->{optional}) {
				gen "\tif (in == ${enum_value_prefix}UNDEFINED) {\n";
				gen "\t\t*out = NULL;\n";
				gen "\t\treturn;\n";
				gen "\t}\n";
				gen "\n";
				gen "\t*out = soap_malloc(soap, sizeof(**out));\n";
				$target = "**out";
			} else {
				$target = "*out";
			}
			gen "\tswitch(in) {\n";
			foreach $code (@{$f->{codes}}) {
				my ($enum_value, $soap_enum_value, $upper_cn);

				$upper_cn = $code->{name};
				$enum_value = "${enum_value_prefix}${upper_cn}";
				$upper_cn =~ s/_/_USCORE/g;
				$soap_enum_value = "lbt__${soap_en}${soap_fn}__${upper_cn}";
				gen "\tcase $enum_value: $target = $soap_enum_value; break;\n"
			}
			if (not $e eq '_common_') {
				gen "\tcase EDG_WLL_${upper_en}_UNDEFINED: assert(0);\n";
			}
			gen "\tdefault: assert(0);\n";
			gen "\t};\n";
			gen "}\n";
			gen "\n";

		} # if code
	} # fields
} # _common_, types
@@@}

static void edg_wll_SourceToSoap(edg_wll_Source in, enum lbt__eventSource *out)
{
	assert(EDG_WLL_SOURCE_PBS_SCHEDULER + 1 == EDG_WLL_SOURCE__LAST);

	switch ( in ) {
	        case EDG_WLL_SOURCE_USER_INTERFACE: *out = UserInterface; break;
        	case EDG_WLL_SOURCE_NETWORK_SERVER: *out = NetworkServer; break;
	        case EDG_WLL_SOURCE_WORKLOAD_MANAGER: *out = WorkloadManager; break;
        	case EDG_WLL_SOURCE_BIG_HELPER: *out = BigHelper; break;
	        case EDG_WLL_SOURCE_JOB_SUBMISSION: *out = JobSubmission; break;  
        	case EDG_WLL_SOURCE_LOG_MONITOR: *out = LogMonitor; break;
	        case EDG_WLL_SOURCE_LRMS: *out = LRMS; break;
        	case EDG_WLL_SOURCE_APPLICATION: *out = Application; break;
		case EDG_WLL_SOURCE_LB_SERVER: *out = LBServer; break;
		case EDG_WLL_SOURCE_CREAM_INTERFACE: *out = CREAMInterface; break;
		case EDG_WLL_SOURCE_CREAM_EXECUTOR: *out = CREAMExecutor; break;
		case EDG_WLL_SOURCE_PBS_CLIENT: *out = PBSClient; break;
		case EDG_WLL_SOURCE_PBS_SERVER: *out = PBSServer; break;
		case EDG_WLL_SOURCE_PBS_SMOM: *out = PBSsMom; break;
		case EDG_WLL_SOURCE_PBS_MOM: *out = PBSMom; break;
		case EDG_WLL_SOURCE_PBS_SCHEDULER: *out = PBSScheduler; break;
        	case EDG_WLL_SOURCE_NONE:
        	case EDG_WLL_SOURCE__LAST:
		default:
			assert(0);
        }
}


/**
 * TODO: not tested
 * Translate event structure to Soap event.
 *
 * \param INOUT soap instance to work with
 * \param IN event source event
 * \param OUT sevent target Soap event
 */
int edg_wll_EventToSoap(struct soap* soap, const edg_wll_Event *event, struct lbt__event *sevent) {
	char *s;

	switch (event->any.type) {
@@@{
	my ($soap_en);

	for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
			$event->getTypes) {
		my ($u);

		$u = uc $e;
		$soap_en = $e;
		while ($soap_en =~ /([[:alpha:]]*)_([[:alpha:]_]*)/) {
			$soap_en = $1.ucfirst($2);
		};
gen qq {
!		case EDG_WLL_EVENT_$u:
!			VALUEEV_SET(sevent, $soap_en, soap_malloc(soap, sizeof(struct lbt__event$soap_en)));
};
		selectType $event '_common_';
		for (getFieldsOrdered $event) {
			my ($f);

			$f = selectField $event $_;
			eventFieldAssign("\t\t\t", "event", "sevent", $e, $f, 1);
		}

		gen "\n";

		selectType $event $e;
		for (getFieldsOrdered $event) {
			my ($f);

			$f = selectField $event $_;
			eventFieldAssign("\t\t\t", "event", "sevent", $e, $f, 0);
		}
gen qq {
!			break;
!
};
	}
@@@}
		default:
			return 0;
	}

	return 1;
}


/** 
 * TODO: not tested
 * Free Soap event.
 *
 * \param INOUT soap instance to work with
 * \param INOUT sevent released Soap event
 */
void edg_wll_FreeSoapEvent(struct soap *soap, struct lbt__event *sevent) {
@@@{
	my ($soap_en);

	for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} } getTypes $event) {
		$soap_en = $e;
		$soap_en = $1.ucfirst $2 while $soap_en =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;

		gen "\tif (VALUEEV_ISTYPE(sevent, $soap_en)) {\n";

		selectType $event '_common_';
		for (getFieldsOrdered $event) {
			my ($f);

			$f = selectField $event $_;
			eventFieldFree("\t\t", "sevent", $e, $f);
		}

		selectType $event $e;
		for (getFieldsOrdered $event) {
			my ($f);

			$f = selectField $event $_;
			eventFieldFree("\t\t", "sevent", $e, $f);
		}
		gen qq "\t\tsoap_dealloc(soap, VALUEEV_GET(sevent, $soap_en));\n";
		gen qq "\t}\n";
}
@@@}
	// XXX: element dealocation belongs to higher layer
#if GSOAP_VERSION <= 20706
	soap_dealloc(soap, sevent);
#endif
}


static void free_events(edg_wll_Event *events)
{
        int     i;

        if (events) {
                for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++)
                        edg_wll_FreeEvent(&(events[i]));
                edg_wll_FreeEvent(&(events[i])); /* free last line */
                free(events);
                events = NULL;
        }
}


/**
 * Translate event array to Soap event array for eventsQueryResponse.
 *
 * \param INOUT soap instance to work with
 * \param IN events source array of the events
 * \param OUT out result Soap structure
 */
int edg_wll_EventsQueryResToSoap(
	struct soap *soap,
	const edg_wll_Event *events,
        struct _lbe__QueryEventsResponse *out)
{
	int i;
	struct lbt__event *event;

	/* alloc the events array */
	assert(out != NULL);
	memset(out, 0, sizeof(*out));
	for (out->__sizeevents = 0; events[out->__sizeevents].any.type != EDG_WLL_EVENT_UNDEF; out->__sizeevents++);
	if (!out->__sizeevents) return SOAP_OK;
	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, events, struct lbt__event, out->__sizeevents);
	for (i = 0; i < out->__sizeevents; i++)
		memset(GLITE_SECURITY_GSOAP_LIST_GET(out->events, i), 0, sizeof(struct lbt__event));
	if (!out->events) return SOAP_FAULT;

	for (i = 0; i < out->__sizeevents; i++) {
		if (!edg_wll_EventToSoap(soap, &events[i], GLITE_SECURITY_GSOAP_LIST_GET(out->events, i))) goto err;
	}

	return SOAP_OK;

err:
	event = GLITE_SECURITY_GSOAP_LIST_GET(out->events, 0);
	while (event) {
		edg_wll_FreeSoapEvent(soap, event);
		event++;
	}
	/*
	 * XXX: better list destroy here w/o deallocstion in
	 * edg_wll_FreeSoapEvent
	 */
	soap_dealloc(soap, out->events);

	return SOAP_FAULT;
}


static int edg_wll_SoapToEvent(
	struct soap *soap,
	struct lbt__event *in,
	edg_wll_Event *out)
{
	//XXX: needs flash

@@@{
        for my $name (sort { $event->{order}->{$a} <=> $event->{order}->{$b} } getTypes $event)
	{
                my $comment = getTypeComment $event $name;
                $name = $1.ucfirst $2 while $name =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
		gen "\t\tif (VALUEEV_ISTYPE(in, $name)) {\n";
		my $event_type = uc $name;
		gen "\t\t\tout->type = EDG_WLL_EVENT_$event_type;\n";

		for ('_common_',$name) {
			my $type = $_;
			selectType $event $type;
			for ($event->getFieldsOrdered) {
				my $f = selectField $event $_;
				my $fn = $f->{name};
				my $ft = $f->{type};
				my $tn = $f->getType;
				my $act;
				if ($type eq '_common_') {
					$act = 'any';
				} else {
					$act = lcfirst $name;
					if ($act =~ m/^pBS/) { $act = ucfirst $act; }
					if ($act =~ m/^cREAM/) { $act = ucfirst $act; }
					if ($act =~ m/^condor/) { $act = ucfirst $act; }
				}
				my $usuc = $fn;
				$usuc = $1.uc($2).$3 while ($usuc =~ /([^_]*)_([a-z])(.*)/);
				my $fuc = ucfirst($usuc);

				my $src = "VALUEEV_GET(in, $name)->$usuc";
				if ($ft eq 'jobid') {

					gen "\t\t\tif ($src)\n";
					gen "\t\t\t\tedg_wlc_JobIdParse($src,&out->$act.$fn);\n";
				} elsif ($ft eq 'timeval') {
					gen "\t\t\tout->$act.$fn.tv_sec = $src->tvSec;\n";
					gen "\t\t\tout->$act.$fn.tv_usec = $src->tvUsec;\n";
				} elsif ($ft eq 'string') {
					gen "\t\t\tout->$act.$fn = $src;\n";
					gen "\t\t\tsoap_unlink(soap,$src);\n";
				} elsif ($ft eq 'int') {
					if ($f->{codes}) {
						if ($type eq '_common_') {
							gen "\t\t\tedg_wll_SoapTo$fuc($src,&(out->$act.$fn));\n";
						}
						else {
							gen "\t\t\tedg_wll_SoapTo$name$fuc($src,&(out->$act.$fn));\n";
						}
					}
					elsif ($f->{optional}) {
						gen "\t\t\tout->$act.$fn = *$src;\n";
					} else {
						gen "\t\t\tout->$act.$fn = $src;\n";
					}
				} elsif ($tn eq 'port' || $tn eq 'bool' || $tn eq 'float' || $ft eq 'double') {
					if ($f->{optional}) {
						gen "\t\t\tout->$act.$fn = *$src;\n";
					} else {
						gen "\t\t\tout->$act.$fn = $src;\n";
					}
				} elsif ($ft eq 'logsrc') {
					gen "\t\t\tedg_wll_SoapToSource($src, &(out->$act.$fn));\n";
				} else {
					gen "\t\t\tout->$act.$fn = $src;\n";
				}
			}	
		}
		gen "\n\t\t\treturn SOAP_OK;\n";
		gen "\t\t}\n";
	}
@@@}

	return 0;
}


/**
 * Translate Soap event array of eventsQueryResponse to event array.
 *
 * \param INOUT soap instance to work with
 * \param IN in result Soap structure
 * \param OUT out source array of the events
 */ 
int edg_wll_SoapToEventsQueryRes(
        struct soap *soap,
        struct _lbe__QueryEventsResponse in,
        edg_wll_Event **out)
{
	edg_wll_Event 	*events;
	int 		i;


	events = calloc(in.__sizeevents + 1, sizeof(*events));
	if (!events) return SOAP_FAULT;

	for (i=0; i < in.__sizeevents; i++) {
		if (edg_wll_SoapToEvent(soap, GLITE_SECURITY_GSOAP_LIST_GET(in.events, i), &events[i]) != SOAP_OK) {
			free_events(events);
			return SOAP_FAULT;
		}
	}

	*out = events;
	return SOAP_OK;
}
