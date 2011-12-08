#ifndef GLITE_LB_EVENT_HPP
#define GLITE_LB_EVENT_HPP


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


/** @file Event.h
 *  @version $Revision$
 */

/*
@@@AUTO
*/
@@@LANG: C++

#include <utility>
#include <vector>
#include <string>

#include "glite/jobid/JobId.h"

#ifdef BUILDING_LB_COMMON
#include "CountRef.h"
#include "events.h"
#include "notifid.h"
#else
#include "glite/lb/CountRef.h"
#include "glite/lb/events.h"
#include "glite/lb/notifid.h"
#endif

EWL_BEGIN_NAMESPACE

/** Class representing one event in the L&B database.
 *
 * This class represents a L&B event, which is basically list of
 * attribute -- value pairs. For each particular event type (returned
 * by name()) there is a list of allowed attributes (returned by
 * getAttrs()). The Event class provides methods for reading these
 * attributes, but no means of changing the 
 * event are provided; this class is used as a result of L&B queries.
 */
class Event {
	friend class Job;
	friend class ServerConnection;
	friend class CountRef<Event>;
public:
	/** Event type codes
	 * identify which of the event fields are valid.
	 */
	enum Type {
		UNDEF = 0, /**< Undefined event type. */
@@@{
	for my $e ($event->getTypesOrdered) {
		my $u = uc $e;
		my $c = getTypeComment $event $e;
		gen "\t\t$u = EDG_WLL_EVENT_$u,\t/**< $c */\n";
	}
@@@}
		TYPE_MAX = EDG_WLL_EVENT__LAST /**< Limit for checking type validity. */
	};

	/** Event attribute symbolic identifier. 
	 * These symbols provide symbolic names for event
	 * attributes. In braces are shown the event types for which the
	 * attribute is defined, for each event type the attribute
	 * meaning is described.
	 */
	enum Attr {
@@@{

	require "./EventAttrNames.pl";
	my %EventAttrNums;
	my $evno = 0;
	for (@main::EventAttrNames) {
		$EventAttrNums{$_} = $evno++;
	}

	my $max = 0;
	for (sort {$a cmp $b} getAllFields $event) {
		my $u = $_;
# $u =~ s/([a-z])([A-Z])/$1_$2/g;
		$u = uc $u;

		my $c = "\t/**\n";
		for my $t (sort $event->getFieldOccurence($_)) {
			selectType $event $t;
			my $cc = getFieldComment $event $_;
			$t = 'common' if $t eq '_common_';
			$c .= "\t * \\n\[$t\] $cc\n";
		}
		$c .= "\t */\n";

		gen defined($EventAttrNums{$u}) ?
			"$c\t\t$u = $EventAttrNums{$u},\n" :
			"#error \"$u not defined in EventAttrNames\"\n";

		$max = $EventAttrNums{$u} if defined($EventAttrNums{$u}) && $EventAttrNums{$u} > $max;
	}
	$max++;
	gen "\t\tATTR_MAX = $max	/**< Limit for checking attribute code validity. */\n";
@@@}
	};

@@@{
	for my $f (getAllFields $event) {
		for my $t (getFieldOccurence $event $f) {
			my $ff;
			my $ut;
			my $utf;
			my $fc;
			my $fn;
			if ($t eq '_common_') {
				$ff = $f;
				$ut = '';
				$utf = '';
				$fc = '';
			}
			else {
				selectType $event $t;
				selectField $event $f;
				$ff = getField $event;
				$ut = uc $t . '_';
				$utf = ucfirst $t;
				$fc = $event->getFieldComment($f);
			}
			$fn = ucfirst $ff->{name};
			my @a = ($fn =~ /_(.)/g);
			while (@a) { my $c = uc shift @a; $fn =~ s/_(.)/$c/; }
			if ($ff->{codes}) {
				gen qq{
!       /** $fc */
!	enum ${utf}${fn}Code \{
};
				for (@{$ff->{codes}}) {
					gen qq{
!		$ut$_->{name} = EDG_WLL_$ut$_->{name},	/**< $_->{comment} */
};
				}
				gen qq{
!	\};
};
				unless ($main::defined_enums{$utf}) {
					gen qq{/* backward compatibility */
!\#define ${utf}Code ${utf}${fn}Code
!
};
					$main::defined_enums{$utf} = 1;
				}
			}
		}
	}
@@@}

	/** Symbolic names for types of attribute values. */
	enum AttrType { INT_T,           /**< Integer value. */
			STRING_T,        /**< String value.  */
			TIMEVAL_T,       /**< Time value (ie. struct
					    timeval). */
			PORT_T,          /**< Service port  (integer). */
			LOGSRC_T,        /**< Source of the event
					    (integer). */
			JOBID_T,         /**< JobId value. */
			NOTIFID_T,        /**< NotifId value. */
			FLOAT_T,         /**< Float value.  */
			DOUBLE_T,        /**< Double value.  */
			TAGLIST_T,       /**< List of user tags (name = value pairs). */
	};

	Type 	type; /**< Type of the event as defined by Type. */

	/** Default constructor.
	 *
	 * Initializes an empty event.
	 */
	Event(void);

	/** Constructor from corresponding C type.
	 *
	 * Initializes the object from the corresponding C struct, in
	 * fact holding the pointer to it.
	 */
        Event(edg_wll_Event *);

	/** Copy constructor.
	 *
	 * Creates a copy of the object by sharing the C struct
	 * with the original.
	 */
	Event(const Event &);

	/** Destructor.
	 *
	 * Releases the C struct (possibly deallocating it).
	 */
	~Event(void);


	/** Assign new Event to an existing instance. 
	 *
	 * The original data are released and new ones are made
	 * accessible.
	 * \returns Reference to this object.
	 */
	Event & operator= (const Event &);

	/** String representation of the event type.
	 * 
	 * Returns string representing the event type.
	 * \returns Name of the event.
	 * \throw Exception Invalid event type.
	 */
	const std::string & name(void) const;

	/** Retrieve integer attribute.
	 * 
	 * Retrieves value for attributes of integer type.
	 * \param[in] name Name of the attribute to retrieve.
	 * \returns Integer value of the attribute.
	 * \throw Exception Invalid event type or attribute not
	 * defined for this event.
	 */
	int	getValInt(Attr name) const;

	/** Retrieve float attribute.
	 * 
	 * Retrieves value for attributes of float type.
	 * \param[in] name Name of the attribute to retrieve.
	 * \returns Integer value of the attribute.
	 * \throw Exception Invalid event type or attribute not
	 * defined for this event.
	 */
	float	getValFloat(Attr name) const;

	/** Retrieve double attribute.
	 * 
	 * Retrieves value for attributes of double type.
	 * \param[in] name Name of the attribute to retrieve.
	 * \returns Integer value of the attribute.
	 * \throw Exception Invalid event type or attribute not
	 * defined for this event.
	 */
	double	getValDouble(Attr name) const;

	/** Retrieve string attribute.
	 *
	 * Retrieves value for attributes of string type.
	 * \param[in] name Name of the attribute to retrieve.
	 * \returns String value of the attribute.
	 * \throw Exception Invalid event type or attribute not
	 * defined for this event.
	 */
	std::string getValString(Attr name) const;

        /** Retrieve time attribute.
	 *
	 * Retrieves value for attributes of timeval type.
	 * \param[in] name Name of the attribute to retrieve.
	 * \returns struct timeval  value of the attribute.
	 */
        struct timeval	getValTime(Attr name) const;
		
        /** Retrieve jobid attribute 
	 *
	 * Retrieves value for attributes of JobId type.
	 * \param[in] name Name of the attribute to retrieve.
	 * \returns JobId value of the attribute.
	 * \throw Exception Invalid event type or attribute not
	 * defined for this event.
	 */
        const glite::jobid::JobId getValJobId(Attr name) const;

	/** Attribute name.
	 *
	 * Retrieves string representation of the attribute name.
	 * \param[in] name Symbolic name of the attribute.
	 * \returns String name of the attribute.
	 * \throw Exception Invalid attribute name.
	 * 
	 */
	static const std::string & getAttrName(Attr name);

	/** List of attributes and types valid for this instance.
	 *
	 * Retrieves description of all attributes defined for this event.
	 * \returns Vector of Attr -- AttrType pairs.
	 * \throw Exception Invalid event type.
	 */
	const std::vector<std::pair<Attr,AttrType> >  &	getAttrs(void) const;

	/** Event name.
	 *
	 * Retrieves string representation of the event type.
	 * \param[in] type Symbolic name of the event type.
	 * \returns String name of the event.
	 * \throw Exception Invalid event type.
	 *
	 */
	static const std::string getEventName(Type type);

private:
	std::string	my_name;
	static void	destroyFlesh(void *);
	CountRef<Event>	*flesh;
};

EWL_END_NAMESPACE

#endif /* GLITE_LB_EVENT_HPP */
