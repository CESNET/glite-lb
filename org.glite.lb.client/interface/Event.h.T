#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_EVENT_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_EVENT_HPP__

#include "edg/workload/logging/client/CountRef.h"
#include "edg/workload/common/jobid/JobId.h"

#include <utility>
#include <vector>
#include <string>

#ident "$Header$"

/** @file Event.h
 *  @version $Revision$
 */

/*
@@@AUTO
*/
@@@LANG: C++

#include "edg/workload/logging/client/events.h"
#include "edg/workload/logging/common/notifid.h"

EWL_BEGIN_NAMESPACE;

class Event {
	friend class Job;
	friend class ServerConnection;
	friend class CountRef<Event>;
public:
	/** Event type codes.
	 * Identify which of the event fields are valid.
	 */

	enum Type {
		UNDEF = 0,
@@@{
	for my $e ($event->getTypesOrdered) {
		my $u = uc $e;
		my $c = getTypeComment $event $e;
		gen "\t\t$u,\t/**< $c */\n";
	}
@@@}
		TYPE_MAX
	};

	/** Event attribute symbolic identifier. */
	enum Attr {
@@@{
	for (sort {$a cmp $b} getAllFields $event) {
		my $u = $_;
# $u =~ s/([a-z])([A-Z])/$1_$2/g;
		$u = uc $u;

		my $c = "\t/**\n";
		for my $t (sort $event->getFieldOccurence($_)) {
			selectType $event $t;
			my $cc = getFieldComment $event $_;
			$t = 'common' if $t eq '_common_';
			$c .= "\t * $t: $cc\n";
		}
		$c .= "\t */\n";

		gen "$c\t\t$u,\n";
	}
@@@}
		ATTR_MAX
	};

@@@{
	for my $f (getAllFields $event) {
		for my $t (getFieldOccurence $event $f) {
			my $ff;
			my $ut;
			my $utf;
			if ($t eq '_common_') {
				$ff = $f;
				$ut = '';
				$utf = '';
			}
			else {
				selectType $event $t;
				selectField $event $f;
				$ff = getField $event;
				$ut = uc $t . '_';
				$utf = ucfirst $t;
			}
			if ($ff->{codes}) {
				gen qq{
!	enum ${utf}Code \{
};
				for (@{$ff->{codes}}) {
					gen qq{
!		$ut$_->{name},	/**< $_->{comment} */
};
				}
				gen qq{
!	\};
};
			}
		}
	}
@@@}

	enum AttrType { INT_T, STRING_T, TIMEVAL_T, PORT_T, LOGSRC_T, JOBID_T, NOTIFID_T };

	Type 	type;

	Event(void);
        Event(edg_wll_Event *);
	Event(const Event &);
	~Event(void);


	/** Assign new Event to an existing instance. */
	Event & operator= (const Event &);

	/** String representation of the event type */
	const std::string & name(void) const;

	/** Retrieve integer attribute */
	int	getValInt(Attr) const;

	/** Retrieve string attribute */
	std::string getValString(Attr) const;

        /** Retrieve time attribute */
        struct timeval	getValTime(Attr) const;
		
        /** Retrieve jobid attribute */
        const edg::workload::common::jobid::JobId getValJobId(Attr) const;

	/** Attribute name */
	const std::string & getAttrName(Attr) const;

	/** List of attributes and types valid for this instance */
	const std::vector<std::pair<Attr,AttrType> >  &	getAttrs(void) const;

private:
	static void	destroyFlesh(void *);
	CountRef<Event>	*flesh;
};

EWL_END_NAMESPACE;

#endif
