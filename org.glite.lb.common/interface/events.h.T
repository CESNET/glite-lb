#ifndef GLITE_LB_EVENTS_H
#define GLITE_LB_EVENTS_H

/**
 * \file events.h
 * \brief contains definition of event types
 */

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

#include <sys/time.h>
#include <inttypes.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/lb_types.h"


#ifdef BUILDING_LB_COMMON
#include "context.h"
#include "notifid.h"
#include "padstruct.h"
#else
#include "glite/lb/context.h"
#include "glite/lb/notifid.h"
#include "glite/lb/padstruct.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup events Events
 * \brief All L&B event types.
 *
 *@{
 */

/**
 * Predefined type for ULM string 
 */
typedef char *edg_wll_LogLine;

enum edg_wll_CREAMCommand {
	EDG_WLL_CREAM_CMD_UNDEFINED = 0,
	EDG_WLL_CREAM_CMD_START = 0,
	EDG_WLL_CREAM_CMD_CANCEL = 0,
	EDG_WLL_CREAM_CMD_PURGE = 0,
	EDG_WLL_CREAM_CMD_SUSPEND = 0,
	EDG_WLL_CREAM_CMD_RESUME = 0,
};

/**
 * \typedef edg_wll_EventCode
 * Predefined event types 
 */
typedef enum _edg_wll_EventCode {
/** invalid code, e.g. uninitialized variable */
        EDG_WLL_EVENT_UNDEF = 0,
@@@{
my $flesh = 'gLite';	#XXX
my $fleshno = 0;
my $eventno = $fleshno;
for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $u = uc $e;
	my $c = getTypeComment $event $e;

	if ($flesh ne $event->{flesh}->{$e}) {
		$flesh = $event->{flesh}->{$e};
		gen "/* new flesh $flesh */\n";
		$fleshno += 100;
		$eventno = $fleshno;
	}
	$eventno++;
	gen "\tEDG_WLL_EVENT_$u = $eventno,\t/**< $c */\n";
}
@@@}
        EDG_WLL_EVENT__LAST,	/**< last currently supported event code */
} edg_wll_EventCode;

/**
 * \fn edg_wll_EventCode edg_wll_StringToEvent(const char *name)
 * \param[in] name		a string event name (e.g. "JobTransfer")
 * \return corresponding numeric code (edg_wll_EventCode)
 * \brief convert a string event name to the corresponding numeric code
 */

extern edg_wll_EventCode edg_wll_StringToEvent(const char *name);

/**
 * \fn char *edg_wll_EventToString(edg_wll_EventCode event)
 * \param[in] event		an event numeric code (edg_wll_EventCode)
 * \return corresponding string (e.g. "JobTransfer")
 * \brief convert an event numeric code to the corresponding string
 */

extern char *edg_wll_EventToString(edg_wll_EventCode event);


/**
 * Free allocated edg_wll_TagValue * list
 * \param list IN: list to free
 */
extern void edg_wll_FreeTagList(
	edg_wll_TagValue *list
);


/**
 * \typedef edg_wll_KeyNameCode
 * Predefined ULM key types
 */
typedef enum _edg_wll_KeyNameCode {
	UNDEFINED = 0,		/**< undefined */
	EDG_WLL_EVNT = 1,		/**< event type */
@@@{
my $keyno = 2;
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = getName $f 'ULM';
	my $fnu = uc $fn;
	my $c = $f->{comment};
	if (hasAlias $f 'ULM') {
		gen "\tULM\_$fnu = $keyno,\t\t/**< $c */\n";
	} else {
		gen "\tEDG_WLL\_COMMON\_$fnu = $keyno,\t\t/**< $c */\n";
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
		my $c = $f->{comment};
		gen "\tEDG_WLL\_$tu\_$fnu = $keyno,\t/**< $c */\n";
		$keyno++;
	}
}
@@@}
	EDG_WLL_INTERNAL_TYPE,		/**< internal message type */
} edg_wll_KeyNameCode;

/**
 * \fn edg_wll_KeyNameCode edg_wll_StringToKeyName(const char *name)
 * \param[in] name		a string ULM key name (e.g. "DG.JOB.TRANSFER.DEST")
 * \return corresponding numeric code (edg_wll_KeyNameCode)
 * \brief convert a string ULM key name to the corresponding numeric code
 */

extern edg_wll_KeyNameCode edg_wll_StringToKeyName(const char *name);

/**
 * \fn char *edg_wll_KeyNameToString(edg_wll_KeyNameCode key)
 * \param[in] key		a ULM key name numeric code (edg_wll_KeyNameCode)
 * \return corresponding string (e.g. "DG.JOB.TRANSFER.DEST")
 * \brief convert a ULM key name numeric code to the corresponding string
 */

extern char *edg_wll_KeyNameToString(edg_wll_KeyNameCode key);


/**
 * Predefined _code_ types and
 * related StringTo_code and _code_ToString function prototypes
 */
@@@{

%enum_done = ();
$indent = "\t";
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	if ($f->{codes}) {
		my $fn = ucfirst($f->{name});
		my $fnu = uc $fn;
		my $c = "${fn}"; # new code
		my $enum = "enum edg_wll\_$c"; # new enum name

		next if $enum_done{$enum};
		$enum_done{$enum} = 1;

# enum
		gen qq{
/**
 * \\$enum
 * $fn codes 
 */
$enum \{
};
		gen $indent."EDG_WLL_${fnu}_UNDEFINED,\t/**< undefined code */ \n";
		for (@{$f->{codes}}) {
			gen $indent."EDG_WLL_${fnu}_$_->{name},\t/**< $_->{comment} */ \n";
		}
		gen "};\n";

# function StringTo:
		gen qq{
/**
 * \\fn $enum edg_wll_StringTo${c}(const char *name);
 * \\param[in] name		a string representing $fn code (e.g. \"${$f->{codes}}[1]->{name}\")
 * \\return corresponding numeric code ($enum)
 * \\brief converts a string $fn code to corresponding numeric code
 */
extern $enum edg_wll_StringTo${c}(const char *name);
};

# function ToString:
		gen qq{
/**
 * \\fn char *edg_wll\_${c}ToString($enum code);
 * \\param[in] code		a $fn numeric code ($enum)
 * \\return corresponding string (e.g. \"${$f->{codes}}[1]->{name}\")
 * \\brief converts a $fn numeric code to corresponding string
 */
extern char *edg_wll\_${c}ToString($enum code);
\n};
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
			my $fu = uc $fn;
			my $c = "$t${fn}"; # new code
			my $enum = "enum edg_wll\_$c"; # new enum name

			next if $enum_done{$enum};
			$enum_done{$enum} = 1;
# enum
			gen qq{
/**
 * \\$enum
 * $fn codes of the $t event
 */
$enum \{
};
			my $null = $f->{null} ? $f->{null} : 0;
			gen $indent."EDG_WLL_${tu}${fu}_UNDEFINED = $null,\t/**< undefined code */ \n";
			unless ($main::defined_undefs{$tu}) {
				gen qq{/* backward compatibility */
!\#define EDG_WLL_${tu}UNDEFINED EDG_WLL_${tu}${fu}_UNDEFINED
!
};
				$main::defined_undefs{$tu} = 1;
			}
			for (@{$f->{codes}}) {
				my $alias = '';
				if ($f->{ptype} ne $t) {
					my $ptu = uc($f->{ptype}) .'_';
					$alias = " = EDG_WLL_$ptu$_->{name}";
			       	} 
				gen $indent."EDG_WLL_$tu$_->{name}$alias,\t/**< $_->{comment} */ \n";
			}
			gen "};\n";

# function StringTo:
			gen qq{
/**
 * \\fn $enum edg_wll_StringTo${c}(const char *name);
 * \\param[in] name		a string representing $t $fn code (e.g. \"${$f->{codes}}[1]->{name}\")
 * \\return corresponding numeric code ($enum)
 * \\brief converts a string $t $fn code to corresponding numeric code
 */
extern $enum edg_wll_StringTo${c}(const char *name);
};

# function ToString:
			gen qq{
/**
 * \\fn char *edg_wll\_${c}ToString($enum code);
 * \\param[in] code		a $t $fn numeric code ($enum)
 * \\return corresponding string (e.g. \"${$f->{codes}}[1]->{name}\")
 * \\brief converts a $t $fn numeric code to corresponding string
 */
extern char *edg_wll\_${c}ToString($enum code);
\n};
		}
	}
}
@@@}


/**
 * common fields of all event types: 
 */

@@@{
$indent = "\t";
gen "#define _EDG_WLL_EVENT_COMMON \\\n";
gen $indent."edg_wll_EventCode\t\ttype;\\\n";
selectType $event '_common_';
for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = $f->{name};
	my $tn = $f->getType;
	gen $indent."$tn\t\t$fn /**<< $f->{comment} */;\t\\\n";
}
gen "\n";
@@@}

/**
 * \typedef edg_wll_AnyEvent
 * common structure definition contained in all types of the events
 */
typedef struct _edg_wll_AnyEvent {
_EDG_WLL_EVENT_COMMON
} edg_wll_AnyEvent;


/**
 *  Event types specific structures: 
 */
@@@{
$indent = "\t";
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $s = "edg_wll_${t}Event";
# typedef struct
	gen qq{
/**
* \\typedef $s
* structure definition for $t event 
*/
typedef struct \_$s \{
_EDG_WLL_EVENT_COMMON
};
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		my $fnu = ucfirst $fn;
		my $c = $f->{comment};
		my $tn;
		if ($f->{codes}) {
			$tn = "enum edg_wll\_$t${fnu}";
		} else {
			$tn = $f->getType;
		}
		gen $indent."$tn\t$fn;\t/**< $c */\n";
	}
	gen "\} $s;\n";
}
@@@}

#undef _EDG_WLL_EVENT_COMMON


/**
 * \union edg_wll_Event
 * \brief All event types union
 */
glite_lb_padded_union(_edg_wll_Event,30,
        edg_wll_EventCode           type; /* it is probably never used */
        edg_wll_AnyEvent            any;
@@@{
$indent = "\t";
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tl = lcfirst $t;
	if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
	if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }
	gen $indent."edg_wll_${t}Event\t${tl};\n";
}
@@@}
)
typedef union _edg_wll_Event edg_wll_Event;


/**
 * Initialise an event structure
 * \return pointer to initialised event structure
 */
extern edg_wll_Event *edg_wll_InitEvent(edg_wll_EventCode eventcode);


/**
 * Free the contents of event structure
 * \param[in] event 		structure to be freed
 * \warning As event structures are likely to be allocated in arrays,
 *      the structure itself is not freed.
 *      Its the responsibility of the caller to call free(event)
 *      if appropriate.
 */
void edg_wll_FreeEvent(
	edg_wll_Event * event
);


/* Event formats: */

#define EDG_WLL_FORMAT_COMMON	"DATE=%s HOST=\"%|Us\" PROG=edg-wms LVL=%s DG.PRIORITY=%d DG.SOURCE=\"%|Us\" DG.SRC_INSTANCE=\"%|Us\" DG.EVNT=\"%s\" DG.JOBID=\"%s\" DG.SEQCODE=\"%|Us\" "
#define EDG_WLL_FORMAT_USER	"DG.USER=\"%|Us\" "
@@@{
# FIXME:
# this is all functional, but doesn't fit to all common fields (eg. USER)
#
#gen "#define EDG_WLL_FORMAT_COMMON\t\"";
#selectType $event '_common_';
#for ($event->getFieldsOrdered) {
#	my $f = selectField $event $_;
#	my $fn = getName $f 'ULM';
#	my $fnu = uc $fn;
#	if (hasAlias $f 'ULM') {
#		gen "$fnu=%s ";
#	} else {
#		gen "DG\.$fnu=";
#			gen $f->toFormatString;
#			gen " ";
#	}
#}
#gen "\"\n";
#
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	gen "#define EDG_WLL_FORMAT_$tu\t\"";
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->{name};
		my $fnu = uc $fn;
		gen "DG.$tu.$fnu=\\\"";
		if ($f->{codes}) {
			gen "%s";
		} else {
			gen $f->toFormatString;
		}
		gen "\\\" ";
	}
	gen "\"\n";
}
@@@}
#define EDG_WLL_FORMAT_NOTIFICATION_COMMON	"DATE=%s HOST=\"%|Us\" PROG=edg-wms LVL=%s DG.SOURCE=\"%|Us\" DG.SRC_INSTANCE=\"%|Us\" DG.TYPE=\"notification\" "
#define EDG_WLL_FORMAT_SYSCMPSTAT	"DG.SCHED.STATUS=\"%|Us\" "
#define EDG_WLL_FORMAT_SYSCLSTAT	"DG.SCHED.NODE=\"%|Us\" DG.SCHED.STATUS=\"%|Us\" "


/* Flags (priority bits) defining the type of logging */

#define EDG_WLL_LOGFLAG_ASYNC		0 /**< asynchronous logging */
#define EDG_WLL_LOGFLAG_SYNC_COMPAT	1 /**< synchronous logging in previous versions*/
#define EDG_WLL_LOGFLAG_SYNC		2 /**< synchronous logging */
#define EDG_WLL_LOGFLAG_LOCAL		4 /**< logging to local logger */
#define EDG_WLL_LOGFLAG_PROXY		8 /**< logging to L&B Proxy */
#define EDG_WLL_LOGFLAG_DIRECT		16 /**< logging directly to bkserver */
#define EDG_WLL_LOGFLAG_INTERNAL	32 /**< internally generated event, don't forward */
#define EDG_WLL_LOGLFLAG_EXCL		64 /**< exclusive flag, to be used with RegisterJob only (if job already exists, than registration fails) */
#define EDG_WLL_LOGFLAG_FILE		128 /**< logging to local file to be consumed by IL (IL must be running on the same node, which is not a typical setup) */

/*
 *@} end of group
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_EVENTS_H */
