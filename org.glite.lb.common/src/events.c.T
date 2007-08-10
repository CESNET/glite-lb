#ident "$Header$"
/*
@@@AUTO
*/
@@@LANG: C

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/events.h"
#include "glite/lb/ulm_parse.h"

static const struct timeval null_timeval = {0,0};

/**
 * Predefined event types names
 */
static const char *eventNames[] = {
	"Undefined",
@@@{
for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	gen "\t\"$e\",\n";
}
@@@}
	"SysCmpStat",
	"SysClStat",
};

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
		if (strcasecmp(eventNames[i],name) == 0) return (edg_wll_EventCode) i;
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
	return strdup(eventNames[event]);
}


/**
 * Predefined ULM key types names
 */
static const char *keyNames[] = {
	"Undefined",
	"DG.EVNT",
@@@{
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = getName $f 'ULM';
	my $fnu = uc $fn;
	my $c = $f->{comment};
	if (hasAlias $f 'ULM') {
		gen "\t\"$fnu\",\n";
	} else {
		gen "\t\"DG.$fnu\",\n";
	}
}
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fnu = uc $f->{name};
		gen "\t\"DG.$tu.$fnu\",\n";
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
	if ((int)code < 0 || code >= sizeof($char)/sizeof(${char}\[0\])) return NULL;
	return strdup(${char}\[code\]);

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
	}
	gen $indent.$indent."break;\n"
}
@@@}
   case EDG_WLL_EVENT_UNDEF :
   default:
      break;
}

}
