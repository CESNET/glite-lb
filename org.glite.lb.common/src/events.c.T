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
#include "ulm_parse.h"

static const struct timeval null_timeval = {0,0};

/**
 * Predefined event types names
 */
static const char *eventNames[] = {
	"Undefined",
@@@{
my $flesh = 'gLite';    #XXX
my $fleshno = 0;
my $eventno = $fleshno;

for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {

	if ($flesh ne $event->{flesh}->{$e}) {
		$flesh = $event->{flesh}->{$e};
		$fleshno += 100;

		gen "\t";
		while ($eventno < $fleshno) {
			gen "NULL, "; 
			$eventno++;
			if (($eventno % 10) == 0) { gen "\n\t"; }
		}
		gen "\n";
	}
	
	$eventno++;
	gen "\t\"$e\", /* $eventno */\n";
}
@@@}
	"SysCmpStat",
	"SysClStat",
};

/* Free the tags and values from list */
void edg_wll_FreeTagList(edg_wll_TagValue *list) {
	edg_wll_TagValue *tag;

	if(NULL == list) return;

	for(tag = list; tag->tag; tag++) {
		free(tag->tag);
		if(tag->value) free(tag->value);
	}
}

/* Deep copy the tag list */
edg_wll_TagValue *edg_wll_CopyTagList(edg_wll_TagValue *src) {
	edg_wll_TagValue *dst, *tag, *dtag;
	int count;
	
	if(NULL == src) return NULL;

	for(tag = src, count = 0; tag->tag; tag++, count++);

	dst = (edg_wll_TagValue*)calloc(count + 1, sizeof(edg_wll_TagValue));
	for(tag = src, dtag = dst; tag->tag; tag++, dtag++) {
		dtag->tag = strdup(tag->tag);
		dtag->value = strdup(tag->value);
	}

	return dst;
}


/**
 * \fn edg_wll_EventCode edg_wll_StringToEvent(const char *name)
 * \param name          a string event name (e.g. "JobTransfer")
 * \return corresponding numeric code (edg_wll_EventCode)
 * \brief convert a string event name to the corresponding numeric code  
 * Calls: strcasecmp
 * Algorithm: array lookup
 */
edg_wll_EventCode edg_wll_StringToEvent(const char *name)
{
	unsigned int	i;

	for (i=1; i<sizeof(eventNames)/sizeof(eventNames[0]); i++)
		if ( (eventNames[i]) && (strcasecmp(eventNames[i],name) == 0)) return (edg_wll_EventCode) i;
	return EDG_WLL_EVENT_UNDEF;
}

/**
 * \fn char *edg_wll_EventToString(edg_wll_EventCode event)
 * \param event         an event numeric code (edg_wll_EventCode)
 * \return corresponding string (e.g. "JobTransfer")
 * \brief convert an event numeric code to the corresponding string
 * Calls: strdup
 */
char *edg_wll_EventToString(edg_wll_EventCode event)
{
	if ((int)event < 0 || event >= sizeof(eventNames)/sizeof(eventNames[0])) return NULL;
	return eventNames[event] ? strdup(eventNames[event]) : NULL;
}


/**
 * Predefined ULM key types names
 */
static const char *keyNames[] = {
	"Undefined",
	"DG.EVNT",
@@@{
my $keyno = 2;
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = getName $f 'ULM';
	my $fnu = uc $fn;
	my $c = $f->{comment};
	if (hasAlias $f 'ULM') {
		gen "\t\"$fnu\",  /* $keyno */\n";
	} else {
		gen "\t\"DG.$fnu\", /* $keyno */\n";
	}
	$keyno++;
}
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fnu = uc $f->{name};
		gen "\t\"DG.$tu.$fnu\", /* $keyno */\n";
		$keyno++;
	}
}
@@@}
	"DG.TYPE",
};

/*
 * \fn edg_wll_KeyNameCode edg_wll_StringToKeyName(const char *name)
 * \param name		a string ULM key name (e.g. "DG.JOB.TRANSFER.DEST")
 * \return corresponding numeric code (edg_wll_KeyNameCode)
 * \brief convert a string ULM key name to the corresponding numeric code
 * Calls: strcasecmp
 * Algorithm: array lookup
 */
edg_wll_KeyNameCode edg_wll_StringToKeyName(const char *name)
{
	unsigned int	i;

	for (i=1; i<sizeof(keyNames)/sizeof(keyNames[0]); i++)
		if (strcasecmp(keyNames[i],name) == 0) return (edg_wll_KeyNameCode) i;
	return UNDEFINED;
}

/*
 * \fn char *edg_wll_KeyNameToString(edg_wll_KeyNameCode key)
 * \param key		a ULM key name numeric code (edg_wll_KeyNameCode)
 * \return corresponding string (e.g. "DG.JOB.TRANSFER.DEST")
 * \brief convert a ULM key name numeric code to the corresponding string
 * Calls: strdup
 */
char *edg_wll_KeyNameToString(edg_wll_KeyNameCode key)
{
	if ((int)key < 0 || key >= sizeof(keyNames)/sizeof(keyNames[0])) return NULL;
	return strdup(keyNames[key]);
}


/**
 * Predefined _code_ types names and
 * related StringTo_code and _code_ToString function implementations
 */
@@@{
$indent = "\t";
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	if ($f->{codes}) {
		my $fn = ucfirst($f->{name});
		my $fnu = uc $fn;
		my $c = "${fn}"; 		# code
		my $enum = "enum edg_wll\_$c"; 	# enum name
		my $char = "edg_wll\_${fn}Names"; # char name
		my $null = $f->{null} ? $f->{null} : 0;
# static const char:
		gen qq{
/**
 * Predefined code names for $c
 */
static const char \*${char}\[\] = \{
};
		gen $indent."\"UNDEFINED\",\n";
		for (@{$f->{codes}}) {
			gen $indent."\"$_->{name}\",\n";
		}
		gen "}; \n";

# function StringTo:
		gen qq{
/**
 * \\fn $enum edg_wll_StringTo${c}(const char *name)
 * Calls: strcasecmp
 * Algorithm: array lookup
 */
$enum edg_wll_StringTo${c}(const char *name) 
\{
	unsigned int	i;

	for (i=1; i<sizeof($char)/sizeof(${char}\[0\]); i++) 
		if (strcasecmp(${char}\[i\],name) == 0) return ($enum) i;
	return ($enum) EDG_WLL_${fnu}_UNDEFINED;
\}
\n};

# function ToString:
		gen qq{
/**
 * \\fn *edg_wll\_${c}ToString($enum code)
 * Calls: strdup
 */
char *edg_wll\_${c}ToString($enum code) 
\{
	if ((int)code < 0 || code >= sizeof($char)/sizeof(${char}\[0\])) return NULL;
	return strdup(${char}\[code\]);

\}
\n\n};
	}
}
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t . '_';
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		if ($f->{codes}) {
			my $fn = ucfirst($f->{name});
			my $c = "$t${fn}"; 		# code
			my $enum = "enum edg_wll\_$c"; 	# enum name
			my $char = "edg_wll\_$t${fn}Names"; # char name
			my $null = $f->{null} ? $f->{null} : 0;
# static const char:
			gen qq{
/**
 * Predefined code names for $c
 */
static const char \*${char}\[\] = \{
};
			gen $indent."\"UNDEFINED\",\n";
			for (@{$f->{codes}}) {
				gen $indent."\"$_->{name}\",\n";
			}
			gen "}; \n";

# function StringTo:
			gen qq{
/**
 * \\fn $enum edg_wll_StringTo${c}(const char *name)
 * Calls: strcasecmp
 * Algorithm: array lookup
 */
$enum edg_wll_StringTo${c}(const char *name) 
\{
	unsigned int	i;

	for (i=1; i<sizeof($char)/sizeof(${char}\[0\]); i++) 
		if (strcasecmp(${char}\[i\],name) == 0) return ($enum) i + ($null);
	return ($enum) EDG_WLL_${tu}UNDEFINED;
\}
\n};

# function ToString:
			gen qq{
/**
 * \\fn *edg_wll\_${c}ToString($enum code)
 * Calls: strdup
 */
char *edg_wll\_${c}ToString($enum code) 
\{
	if ((int)code - ($null) < 0 || code - ($null) >= sizeof($char)/sizeof(${char}\[0\])) return NULL;
	return strdup(${char}\[code - ($null)\]);

\}
\n\n};
		}
	}
}
@@@}

/**
 * Initialise an event structure
 * \return pointer to initialised event structure
 */
edg_wll_Event *edg_wll_InitEvent(
		edg_wll_EventCode eventcode	/* eventcode IN */
) 
{
edg_wll_Event *event=NULL;

/* allocate memory for 'event' (edg_wll_Event); use calloc to also clean it */
	event = (edg_wll_Event *) calloc(1,sizeof(edg_wll_Event));
	if (event == NULL) return event;

	event->type = eventcode;
/* initialize common fields */
@@@{
$indent = "\t";
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = $f->getName;
	my $fd = $f->getDefaultNullValue;
	gen $indent."event->any.$fn = $fd;\n"
}
@@@}
/* initialize dependent fields */
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
	gen $indent."case EDG_WLL_EVENT_$tu : \n";
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->getName;
		my $fd = $f->getDefaultNullValue;
		gen $indent."\tevent->$tl.$fn = $fd;\n"
	}
	gen $indent.$indent."break;\n";
}
@@@}
   case EDG_WLL_EVENT_UNDEF :
   default :
      break;
}
return event;
}

/**
 * edg_wll_FreeEvent 
 * \brief free memory allocated for edg_wll_Event internal fields
 * Calls: free
 */
void edg_wll_FreeEvent(
          edg_wll_Event *event       /* event IN */
)
{
edg_wll_EventCode eventcode=event->type;

/* free the common fields */
        if (event->any.jobId) edg_wlc_JobIdFree(event->any.jobId);
@@@{
$indent = "\t";
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = $f->{name};
	my $ft = $f->{type};
	if ($ft eq 'string') {
		gen $indent."if (event->any.$fn) free(event->any.$fn);\n"
	}
}
@@@}

/* free the rest */
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
		my $fn = $f->{name};
		my $ft = $f->{type};
		if ($ft eq 'string') {
			gen $indent."\tif (event->$tl.$fn) free(event->$tl.$fn);\n"
		}
		if ($ft eq 'jobid') {
			gen $indent."\tif (event->$tl.$fn) edg_wlc_JobIdFree(event->$tl.$fn);\n"
		}
		if ($ft eq 'notifid') {
			gen $indent."\tif (event->$tl.$fn) edg_wll_NotifIdFree(event->$tl.$fn);\n"
		}
		if ($ft eq 'taglist') {
			gen $intent."\tif (event->$tl.$fn) { edg_wll_FreeTagList(event->$tl.$fn); free(event->$tl.$fn); }\n"
		}	
	}
	gen $indent.$indent."break;\n"
}
@@@}
   case EDG_WLL_EVENT_UNDEF :
   default:
      break;
}

}
