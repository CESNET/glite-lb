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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "events.h"
#include "events_parse.h"
#include "ulm_parse.h"
#include "context-int.h"
#include "glite/lbu/escape.h"
#include "glite/lbu/trio.h"

static const struct timeval null_timeval = {0,0};

/* -- Internal function prototype -- */
char *my_edg_wll_ULMGetValueAt( p_edg_wll_ULMFields, int );

/* Function for parsing name=value tag lists */
int edg_wll_TagListParse(const char *src, edg_wll_TagValue **list) {
	int n = 0;
	char *p;
	edg_wll_TagValue *tag;

	if(NULL == src)
		return -1;

	/* count tag=value pairs */
	for(p = (char*)src; *p; p++) {
		if(',' == *p) n++; /* counts empty tokens as well... */
	}
	/* one more for last empty struct */
	n += 2;
	
	*list = (edg_wll_TagValue*)malloc(n*sizeof(edg_wll_TagValue));
	if(NULL == *list) 
		return -1;

	for(p = strtok(src, ","), tag = *list; p; p = strtok(NULL, ","), tag++) {
		char *s = strchr(p, '=');

		if(NULL == s || '\0' == *(s+1)) 
			continue;

		*s = 0;
		tag->tag = strdup(p);
		*s = '=';
		tag->value = strdup(s+1);
	}
	tag->tag = NULL;
	tag->value = NULL;

	return 0;
}


/* Function for stringifying name=value tag lists */
char * edg_wll_TagListToString(edg_wll_TagValue *list) {
	int len = 0;
	edg_wll_TagValue *tag;
	char *out, *p;

	if(NULL == list) return NULL;

	for(tag = list; tag->tag; tag++) {
		len += strlen(tag->tag);
		len += strlen(tag->value);
		len += 2;
	}
	
	out = malloc(len*sizeof(char) + 1);
	if(NULL == out) return NULL;

	*out = '\0';
	p = out;
	for(tag = list; tag->tag; tag++) {
		if(p != out) {
			strcat(p++, ",");
		}
		strcat(p, tag->tag);
		p += strlen(tag->tag);
		strcat(p++, "=");
		strcat(p, tag->value);
		p += strlen(tag->value);
	}

	return out;
}


/**
 * Edg_wll_ParseEvent - parse ULM message to internal structures
 * Calls: calloc, free, sprintf, strdup
 *        edg_wll_ULMNewParseTable, edg_wll_ULMFreeParseTable, edg_wll_ULMProcessParseTable,
 *        edg_wll_ULMGetNameAt, my_edg_wll_ULMGetValueAt, edg_wll_ULMDateToTimeval (edg_wll_ULMDateToDouble),
 *        edg_wll_StringToEvent, edg_wll_StringToKeyName, edg_wll_StringToLevel, edg_wlc_jobid_tParse,
 *        edg_wll_EventToString, edg_wll_KeyNameToString, edg_wll_SetError (or edg_wll_ResetError)
 * Algorithm: break ULM string into fields and then according the event type
 *            fill in the proper structures
 */

#define DUPLICITY { \
	char *k =  edg_wll_KeyNameToString(keycode); \
	sprintf(err_desc,"Key %s already exists.", k); \
	free(k); \
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_KEY_DUPLICITY,err_desc); \
	goto end; }

#define MISUSE { \
	char *e = edg_wll_EventToString(eventcode); \
	char *k =  edg_wll_KeyNameToString(keycode); \
	sprintf(err_desc,"Key %s schouldn't be used for event type %s.", k,e); \
	free(e); \
	free(k); \
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_KEY_MISUSE,err_desc); \
	goto end; }

edg_wll_ErrorCode edg_wll_ParseEvent(
        edg_wll_Context context,   /* context IN */
        edg_wll_LogLine logline,     /* logline IN: ULM string to parse */
        edg_wll_Event **event      /* event OUT: parsed event 
                           (may be NULL - syntax checking with no output */
)
{
int 			i;
int 			extra=0;	/* number of extra unknown key=value pairs */
char 			err_desc[128];	/* error description for use in edg_wll_SetError */
char 			*value=NULL;
p_edg_wll_ULMFields	table = edg_wll_ULMNewParseTable(logline);
edg_wll_KeyNameCode	keycode=UNDEFINED;
edg_wll_EventCode 	eventcode=EDG_WLL_EVENT_UNDEF;
edg_wll_ErrorCode	ret;
edg_wll_Event		*this=NULL;

/* break ULM string into fields */
if ( edg_wll_ULMProcessParseTable(table) != 0 ) { 
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_BROKEN_ULM,"ULM parse error"); 
	goto end; 
}

/* determine the event type */
for (i=0; i<table->num; i++) {
   keycode=edg_wll_StringToKeyName(edg_wll_ULMGetNameAt(table,i));
   if ( keycode == EDG_WLL_EVNT ) { 
	value=my_edg_wll_ULMGetValueAt(table,i); 
	eventcode=edg_wll_StringToEvent(value); 
	free(value);
	value=NULL;
	break; 
   }
}
if ( eventcode == EDG_WLL_EVENT_UNDEF ) {
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_EVENT_UNDEF,"Unknown or missing event type"); 
	goto end; 
}
/* allocate memory for 'this' (edg_wll_Event) */
// XXX:  this = (edg_wll_Event *) calloc(1,sizeof(edg_wll_Event)); 
this = edg_wll_InitEvent(eventcode);

/* go through all fields and fill in the edg_wll_Event union
 * in each step check for key duplicity and key name misuse 
 */
for (i=0; i<table->num; i++) {
   keycode=edg_wll_StringToKeyName(edg_wll_ULMGetNameAt(table,i));
   value=my_edg_wll_ULMGetValueAt(table,i); 
   switch (keycode) {
   case EDG_WLL_EVNT : 
	if (this->any.type != edg_wll_StringToEvent(value)) DUPLICITY
      break;
@@@{
$indent = "   ";
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = getName $f;
	my $fnu = uc getName $f 'ULM';
	if (hasAlias $f 'ULM') {
		gen $indent."case ULM\_$fnu :\n";
	} else {
		gen $indent."case EDG_WLL\_COMMON\_$fnu :\n";
	}
	gen "\tif (";
		gen $f->isnotNULL("this->any.$fn");
		gen ") DUPLICITY \n";
	gen "\t";
	if ($f->{codes}) {
		my $c = ucfirst(${fn});
		gen "this->any.$fn = edg_wll_StringTo${c}(value);";
	} else {
		gen $f->fromString('value',"this->any.$fn");
	}
	gen "\n".$indent.$indent."break;\n";
}
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $tl = lcfirst $t;
	if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }
	my $misuse = "if (eventcode != EDG_WLL_EVENT_$tu ) MISUSE";
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		my $fnu = uc $fn;
		gen $indent."case EDG_WLL\_$tu\_$fnu :\n";
		gen "\tif ("; 
			gen $f->isnotNULL("this->$tl.$fn");
			gen ") DUPLICITY\n";
		gen "\t$misuse\n";
		if ($f->{codes}) {
			my $c = "$t".ucfirst(${fn});
			gen "\tthis->$tl.$fn = edg_wll_StringTo${c}(value);";
		} else {
			gen "\tif(strlen(value) > 0) ";
			gen $f->fromString('value',"this->$tl.$fn");
		}
		gen "\n".$indent.$indent."break;\n";
	}
}
@@@}

   case UNDEFINED :
   case EDG_WLL_INTERNAL_TYPE :
      break;

   default : 
	extra++;
      break;
   }
   free(value);
   value=NULL;
}

/* now check if all required fields are present */
ret=edg_wll_CheckEvent(context,this);
if (ret) { goto end; }

/* parse is OK, only extra fields could occur */
if (extra) {
	sprintf(err_desc,"There are %d extra fields in the logline.",extra);
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_OK_WITH_EXTRA_FIELDS,err_desc); }
else {	ret=edg_wll_ResetError(context); }

end:
   /* finally (if wanted) "return" pointer to the filled edg_wll_Event union
      NOTE: a pointer (to half-filled structure) is returned even if an error occured ! */
   if (event != NULL) {
	*event = this; }
	/* This also means, that the memory must be freed by edg_wll_FreeEvent() somewhere else */
   else {
	edg_wll_FreeEvent(this);
	free(this);
   }

   edg_wll_ULMFreeParseTable(table);
   return ret;
}

/**
 * edg_wll_UnparseEvent - unparse ULM message from internal structures
 * Calls: malloc, free, strlen, strcmp, asprintf, trio_asprintf
 *        edg_wll_ULMTimevalToDate,
 *        edg_wll_EventToString, edg_wll_KeyNameToString, edg_wll_LevelToString, 
 *        edg_wlc_jobid_tUnparse, 
 * Algorithm: format values from internal structures into a ULM string
 */
#define NOMEM { if (logline) free(logline); logline = NULL; edg_wll_SetError(context,ENOMEM,NULL); goto clean; }

edg_wll_LogLine edg_wll_UnparseEvent(       /* logline OUT */
          edg_wll_Context context,    /* context IN */
          edg_wll_Event *event        /* event IN */
)
{
edg_wll_LogLine		logline;
edg_wll_EventCode 	eventcode=event->type;
char 			*date, *common, *user, *var, *e, *l, *j, *s;
edg_wll_Event		nonulls;

logline = date = common = user = var = NULL;

/* FIXME: after EDG_WLL_FORMAT_COMMON automatic generation automate this code as well */
/* format the common fields */
date=(char *) malloc(1+ULM_DATE_STRING_LENGTH*sizeof(char));
edg_wll_ULMTimevalToDate(event->any.timestamp.tv_sec, event->any.timestamp.tv_usec, date);

memcpy(&nonulls,event,sizeof nonulls);
@@@{
	selectType $event '_common_';
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		gen "if (!nonulls.any.$fn) nonulls.any.$fn = \"\";\n"
			if $f->{type} eq 'string';
	}
@@@}

e = edg_wll_EventToString(eventcode);
l = edg_wll_LevelToString(event->any.level);
j = edg_wlc_JobIdUnparse(event->any.jobId);
s = edg_wll_SourceToString(event->any.source);
if (trio_asprintf(&common,EDG_WLL_FORMAT_COMMON, \
	date, event->any.host, l, event->any.priority, s, nonulls.any.src_instance, \
	e, j, nonulls.any.seqcode) == -1) { 
	if (e) free(e);
	if (l) free(l);
	if (j) free(j);
	if (s) free(s);
	NOMEM 
}
if (e) free(e);
if (l) free(l);
if (j) free(j);
if (s) free(s);

if (trio_asprintf(&user,EDG_WLL_FORMAT_USER, event->any.user) == -1) NOMEM

/* format the rest of the logline */
switch (eventcode) {
@@@{
$indent = "   ";
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $tl = lcfirst $t;
	if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }
	my $free = "";
	gen $indent."case EDG_WLL_EVENT_$tu :\n";
	gen "\t\{\n";
	selectType $event $t;
# if there are some _code_, convert them from string:
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		my $fnu = ucfirst $fn;
		if ($f->{codes}) {
			gen "\t\tchar \*$fn\_code = edg_wll\_$t${fnu}ToString(event->$tl.$fn);\n"; 
			$free = $free . "free($fn\_code); ";
		}
		elsif ($f->{type} eq 'jobid') {
			gen "\t\tchar \*$fn\_str = edg_wlc_JobIdUnparse(event->$tl.$fn);\n";
			$free .= "free($fn\_str); ";
		}
		elsif ($f->{type} eq 'notifid') {
			gen "\t\tchar \*$fn\_str = edg_wll_NotifIdUnparse(event->$tl.$fn);\n";
			$free .= "free($fn\_str); ";
		}
		elsif ($f->{type} eq 'timeval') {
			gen "\t\tchar $fn\_str[ULM_DATE_STRING_LENGTH + 1];\n";
			gen "\t\t".toString $f "event->$tl.$fn", "$fn\_str"; gen "\n";
		}
		elsif ($ULMasString{$f->{type}}) {
			#gen "\t\tchar \*$fn\_str = ".$f->getType()."ToString(event->$tl.$fn);\n";
			gen "\t\t". $f->toString("event->$tl.$fn","char \*$fn\_str",)."\n";
			$free .= "free($fn\_str); ";
		}
	}
	
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};

		gen "\t\tif (!nonulls.$tl.$fn) nonulls.$tl.$fn = \"\";\n"
			if $f->{type} eq 'string';
	}
	gen "\t\tif (trio_asprintf(&var,EDG_WLL_FORMAT_$tu";
# now format remaining parapteres for trio_asprintf:
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		if ($f->{codes}) {
			gen ",$fn\_code";
		}
		elsif ($f->{type} eq 'jobid' || $f->{type} eq 'notifid' || $f->{type} eq 'timeval' || $ULMasString{$f->{type}}) {
			gen ",$fn\_str?$fn\_str:\"\"";
		} else {
			gen ",nonulls.$tl.$fn";
		}
	}
	gen ") == -1 ) \{\n";
		gen "\t".$indent.$free."NOMEM \}\n";
	if ($free) { gen "\t\t$free\n"; }
	gen "\t\}\n";
	gen $indent.$indent."break;\n";
}
@@@}
  case EDG_WLL_EVENT_UNDEF :
  default :
     break;
}

/* put it all together (inc. the ending LF) */
if (trio_asprintf(&logline,"%s%s%s\n",common,user,var) == -1) NOMEM

clean:
	if (date) free(date);
	if (common) free(common);
	if (user) free(user);
	if (var) free(var);
	return logline;
}


/**
 * edg_wll_CheckEvent - check internal structures if all required fields are present
 * Calls: free, sprintf, strcmp
 *        edg_wll_KeyNameToString, edg_wll_SetError (or edg_wll_ResetError)
 * Algorithm:
 */
#define MISSING(m_key) { \
	char *k =  edg_wll_KeyNameToString(m_key); \
        sprintf(err_desc,"Message incomplete, missing or empty field %s.", k); \
	free(k); \
        ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_MSG_INCOMPLETE,err_desc); \
        goto end; }

edg_wll_ErrorCode edg_wll_CheckEvent(
	edg_wll_Context context,	/* context IN */
	edg_wll_Event *event	/* event IN */
)
{
char err_desc[128]; 		/* error description for use in edg_wll_SetError */
edg_wll_EventCode eventcode=EDG_WLL_EVENT_UNDEF;
edg_wll_ErrorCode ret;

eventcode=event->type;
if ( eventcode == EDG_WLL_EVENT_UNDEF ) {
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_EVENT_UNDEF,"Unknown or missing event type"); 
	goto end; 
}

/*
FIXME:
?	if (!event->any.user) MISSING(EDG_WLL_USER) - cannot use in edg_wll_LogEvent(), where is no DG.USER
*/

@@@{
	$indent = "   ";
	selectType $event '_common_';
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		next if $f->{optional};
		if (!$f->{codes}) {
			if (!$f->hasAlias('ULM')) {
				my $fu = uc $fn;
				gen "\tif (". $f->isNULL("event->any.$fn") .") MISSING(EDG_WLL\_COMMON\_$fu)\n";
			} else {
				my $fa = $f->getName('ULM');
				my $fu = uc $fa;
				gen "\tif (". $f->isNULL("event->any.$fn") .") MISSING(ULM\_$fu)\n";
			}
		}
	}
@@@}

switch (eventcode) {
@@@{
$indent = "   ";
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $tl = lcfirst $t;
	if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }
	gen $indent."case EDG_WLL_EVENT_$tu :\n";
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		next if $f->{optional};
		my $fn = $f->{name};
		my $fu = uc $fn;
		my $ft = $f->{type};
		gen "\tif (". $f->isNULL("event->$tl.$fn") .") MISSING(EDG_WLL\_$tu\_$fu)\n";
	}
	gen $indent.$indent."break;\n";
}
@@@}

   case EDG_WLL_EVENT_UNDEF :
   default:
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_EVENT_UNDEF,"Unknown or missing event type"); 
        goto end;
      break;
};
ret=edg_wll_ResetError(context);

end:
	return ret;
}

/**
 * edg_wll_CompareEvents - compare two event structures
 * Calls: 
 * Algorithm:
 */
edg_wll_ErrorCode edg_wll_CompareEvents(
	edg_wll_Context context,        /* context IN */
	const edg_wll_Event *e1,	/* event 1 IN */ 
	const edg_wll_Event *e2		/* event 2 IN */
)
{
	char *tmp1, *tmp2;
	int tmp3;
	if (e1->any.type != e2->any.type) return edg_wll_SetError(context,EDG_WLL_ERROR_COMPARE_EVENTS,"Different event types");
@@@{
	selectType $event '_common_';
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = getName $f;
		my $ft = $f->{type};

		my $a = "e1->any.$fn";
		my $b = "e2->any.$fn";

		gen "\tif (!(".eval($main::compare{C}->{$ft}).")) return edg_wll_SetError(context,EDG_WLL_ERROR_COMPARE_EVENTS,\"Different $fn\");\n";
	}
	
	gen "\tswitch(e1->any.type) {\n";
	for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes)
	{
		my $tu = uc $t;
		my $tl = lcfirst $t;
		if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
		if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
		if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }

		selectType $event $t;
		gen "\t\tcase EDG_WLL_EVENT\_$tu :\n";
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = $f->{name};
			my $ft = $f->{type};

			my $a = "e1->$tl.$fn";
			my $b = "e2->$tl.$fn";

			gen "\t\t\tif (!(".eval($main::compare{C}->{$ft}).")) return edg_wll_SetError(context,EDG_WLL_ERROR_COMPARE_EVENTS,\"Different $fn\");\n";
		}
		gen "\t\tbreak;\n";
	}
@@@}
		default: return edg_wll_SetError(context,EDG_WLL_ERROR_COMPARE_EVENTS,"Unknown difference");
	} /* switch */
	return edg_wll_ResetError(context);
}


/**
 * my_edg_wll_ULMGetValueAt - get value at index, but also
 *                - remove surrounding quotes if any
 *                - and unescape the string
 *                - the new string will be allocated (by glite_lbu_UnescapeULM)
 * Calls: edg_wll_ULMGetValueAt, glite_lbu_UnescapeULM, calloc and strncpy or strdup
 */
char *my_edg_wll_ULMGetValueAt( p_edg_wll_ULMFields table, int index )
{
char *str = edg_wll_ULMGetValueAt( table, index);
size_t len = strlen(str);
char *ret,*first,*last,*tmp;

first = last = str;
last += len - 1;

/* remove surrounding quotes */
if ((*first == ULM_QM) && (*last == ULM_QM)) {
	/* be careful with doppel_ULM_QM */
	if (len > 2) {
		tmp = (char *) calloc (1,(len-1)*sizeof(char));
		strncpy(tmp,first+1,len-2);
		tmp[len-2] = '\0';
	}
	else {
		tmp = strdup("");
	}
}
else tmp = strdup(str);

if (len > 2) {
	ret = glite_lbu_UnescapeULM(tmp);
	if (tmp) free(tmp);
}
else ret = tmp;

return ret;
}


/**
 * edg_wll_GetJobId - parse jobId from ULM message
 * Returns: edg_wlc_jobid_t string or NULL
 * Calls: edg_wll_ULMNewParseTable, edg_wll_ULMFreeParseTable, edg_wll_ULMProcessParseTable,
 *        edg_wll_ULMGetNameAt, my_edg_wll_ULMGetValueAt, edg_wlc_jobid_tParse, strdup
 * Algorithm: break ULM string into fields and then look for jobId
 */
char *edg_wll_GetJobId(edg_wll_LogLine logline)
{
p_edg_wll_ULMFields table = edg_wll_ULMNewParseTable(logline);
int i;
char *ret=NULL,*fullid=NULL;
edg_wll_KeyNameCode keycode=UNDEFINED;
edg_wlc_JobId jobId = NULL;

/* break ULM string into fields */
if ( edg_wll_ULMProcessParseTable(table) != 0 ) goto clean;

/* look for jobId */
for (i=0; i<table->num; i++) {
	keycode=edg_wll_StringToKeyName(edg_wll_ULMGetNameAt(table,i));
	if ( keycode == EDG_WLL_COMMON_JOBID ) { fullid=my_edg_wll_ULMGetValueAt(table,i); break; }
}
if ( fullid == NULL ) goto clean;

if (edg_wlc_JobIdParse(fullid, &jobId)) goto clean;

ret = strdup(fullid);

clean:
   if (fullid) free(fullid);
   if (jobId) edg_wlc_JobIdFree(jobId);
   edg_wll_ULMFreeParseTable(table);
   return ret;
}

/**
 * Parse a special Notification ULM line into a edg_wll_Event structure
 * \param context IN: context to work with
 * \param logline IN: ULM string to parse
 * \param event OUT: parsed event
 * 	(may be NULL - syntax checking with no output)
 */
edg_wll_ErrorCode edg_wll_ParseNotifEvent(
	edg_wll_Context	context,
	edg_wll_LogLine	logline,
	edg_wll_Event ** event
)
{
int 			i;
int			extra=0; 	/* number of extra unknown key=value pairs */
char			err_desc[128];	/* error description for use in edg_wll_SetError */
char *value=NULL;
p_edg_wll_ULMFields	table = edg_wll_ULMNewParseTable(logline);
edg_wll_KeyNameCode	keycode=UNDEFINED;
const edg_wll_EventCode	eventcode=EDG_WLL_EVENT_NOTIFICATION;
edg_wll_ErrorCode	ret;
edg_wll_Event		*this=NULL;

/* break ULM string into fields */
if ( edg_wll_ULMProcessParseTable(table) != 0 ) {
	ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_BROKEN_ULM,"ULM parse error");
	goto end;
}

/* allocate memory for 'this' (edg_wll_Event) */
this = edg_wll_InitEvent(eventcode);

/* go through all fields and fill in the edg_wll_Event union
 * in each step check for key duplicity and key name misuse 
 */
for (i=0; i<table->num; i++) {
   keycode=edg_wll_StringToKeyName(edg_wll_ULMGetNameAt(table,i));
   value=my_edg_wll_ULMGetValueAt(table,i); 
   switch (keycode) {
   case EDG_WLL_EVNT : 
	if (this->any.type != edg_wll_StringToEvent(value)) DUPLICITY
      break;
   case ULM_DATE :
	if (!((this->any.timestamp).tv_sec == (null_timeval).tv_sec && (this->any.timestamp).tv_usec == (null_timeval).tv_usec)) DUPLICITY 
	edg_wll_ULMDateToTimeval(value,&this->any.timestamp);
      break;
   case ULM_ARR_DATE :
	if (!((this->any.arrived).tv_sec == (null_timeval).tv_sec && (this->any.arrived).tv_usec == (null_timeval).tv_usec)) DUPLICITY 
	edg_wll_ULMDateToTimeval(value,&this->any.arrived);
      break;
   case ULM_HOST :
	if (!(((this->any.host) == NULL) || ((this->any.host)&& !strcmp(this->any.host,"")))) DUPLICITY
	this->any.host = strdup(value);
      break;
   case ULM_LVL :
	if (!((this->any.level == 0))) DUPLICITY 
	this->any.level = edg_wll_StringToLevel(value);
      break;
   case EDG_WLL_COMMON_SOURCE :
	if (!((this->any.source) == (EDG_WLL_SOURCE_NONE))) DUPLICITY 
	this->any.source = edg_wll_StringToSource(value);
      break;
   case EDG_WLL_COMMON_SRC_INSTANCE :
	if (!(((this->any.src_instance) == NULL) || ((this->any.src_instance)&& !strcmp(this->any.src_instance,"")))) DUPLICITY
	this->any.src_instance = strdup(value);
      break;
@@@{
	$indent = "   ";
	my $t = 'Notification';
	my $tu = uc $t;
	my $tl = lcfirst $t;
	if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }
	my $misuse = "if (eventcode != EDG_WLL_EVENT_$tu ) MISUSE";
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		my $fnu = uc $fn;
		gen $indent."case EDG_WLL\_$tu\_$fnu :\n";
		gen "\tif ("; 
			gen $f->isnotNULL("this->$tl.$fn");
			gen ") DUPLICITY\n";
		gen "\t$misuse\n";
		if ($f->{codes}) {
			my $c = "$t".ucfirst(${fn});
			gen "\tthis->$tl.$fn = edg_wll_StringTo${c}(value);";
		} else {
			gen "\t";
			gen $f->fromString('value',"this->$tl.$fn");
		}
		gen "\n".$indent.$indent."break;\n";
	}
@@@}

   case UNDEFINED :
   case EDG_WLL_INTERNAL_TYPE :
      break;

   default : 
	extra++;
      break;
   }
   free(value);
   value=NULL;
}

/* parse is OK, only extra fields could occur */
if (extra) {
        sprintf(err_desc,"There are %d extra fields in the logline.",extra);
        ret=edg_wll_SetError(context,EDG_WLL_ERROR_PARSE_OK_WITH_EXTRA_FIELDS,err_desc); }
else {  ret=edg_wll_ResetError(context); }

end:
   /* finally (if wanted) "return" pointer to the filled edg_wll_Event union
      NOTE: a pointer (to half-filled structure) is returned even if an error occured ! */
   if (event != NULL) {
	*event = this; }
	/* This also means, that the memory must be freed by edg_wll_FreeEvent() somewhere else */
   else {
	edg_wll_FreeEvent(this);
	free(this);
   }

   edg_wll_ULMFreeParseTable(table);
   return ret;
}

/** 
 * Generate a special Notification ULM line from edg_wll_Event structure
 * \param context IN: context to work with
 * \param event IN: event to unparse
 */
edg_wll_LogLine edg_wll_UnparseNotifEvent(
	edg_wll_Context	context,
	edg_wll_Event *	event
)
{
edg_wll_LogLine logline;
char 		*date, *common, *var, *l, *s;
edg_wll_Event   nonulls;

logline = date = common = var = l = s = NULL;

/* format the common fields */
date=(char *) malloc(1+ULM_DATE_STRING_LENGTH*sizeof(char));
edg_wll_ULMTimevalToDate(event->any.timestamp.tv_sec, event->any.timestamp.tv_usec, date);

memcpy(&nonulls,event,sizeof nonulls);
if (!nonulls.any.host) nonulls.any.host = "";
if (!nonulls.any.src_instance) nonulls.any.src_instance = "";

l = edg_wll_LevelToString(event->any.level);
s = edg_wll_SourceToString(event->any.source);
if (trio_asprintf(&common,EDG_WLL_FORMAT_NOTIFICATION_COMMON, \
        date, event->any.host, l, s, nonulls.any.src_instance) == -1) {
		if (l) free(l);
		if (s) free(s);
		NOMEM 
	}
if (l) free(l);
if (s) free(s);

// n = edg_wlc_NotifIdUnparse(event->notification.notifId);

/* format the rest of the logline */
@@@{
$indent = "   ";
	my $t = 'Notification';
	my $tu = uc $t;
	my $tl = lcfirst $t;
	if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }
	my $free = "";
##	gen $indent."case EDG_WLL_EVENT_$tu :\n";
	gen "\t\{";
	selectType $event $t;
# if there are some _code_, convert them from string:
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		my $fnu = ucfirst $fn;
		if ($f->{codes}) {
			gen "\tchar \*$fn\_code = edg_wll\_$t${fnu}ToString(event->$tl.$fn);\n"; 
			$free = $free . "free($fn\_code); ";
		}
		if ($f->{type} eq 'jobid') {
			gen "\tchar \*$fn\_str = edg_wlc_JobIdUnparse(event->$tl.$fn);\n";
			$free .= "free($fn\_str); ";
		}
		if ($f->{type} eq 'notifid') {
			gen "\tchar \*$fn\_str = edg_wll_NotifIdUnparse(event->$tl.$fn);\n";
			$free .= "free($fn\_str); ";
		}
		if ($ULMasString{$f->{type}}) {
			gen "\tchar \*$fn\_str = ".$f->getType()."ToString(event->$tl.$fn);\n";
			$free .= "free($fn\_str); ";
		}
	}
	
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};

		gen "\t\tif (!nonulls.$tl.$fn) nonulls.$tl.$fn = \"\";\n"
			if $f->{type} eq 'string';
	}
	gen "\tif (trio_asprintf(&var,EDG_WLL_FORMAT_$tu";
# now format remaining parapteres for trio_asprintf:
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		if ($f->{codes}) {
			gen ",$fn\_code";
		}
		elsif ($f->{type} eq 'jobid' || $f->{type} eq 'notifid' || $ULMasString{$f->{type}}) {
			gen ",$fn\_str?$fn\_str:\"\"";
		} else {
			gen ",nonulls.$tl.$fn";
		}
	}
	gen ") == -1 ) \{\n";
		gen "\t".$indent.$free."NOMEM \}\n";
	if ($free) { gen "\t$free\n"; }
	gen "\t\}\n";
##	gen $indent.$indent."break;\n";
@@@}

/* put it all together (inc. the ending LF) */
if (trio_asprintf(&logline,"%s%s\n",common,var) == -1) NOMEM

clean:
        if (date) free(date);
        if (common) free(common);
        if (var) free(var);
        return logline;
}

