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

#ifndef GLITE_LB_JOBSTATUS_HPP
#define GLITE_LB_JOBSTATUS_HPP

/*
@@@AUTO
*/

@@@LANG: C++

#include <sys/time.h>
#include <string>
#include <utility>
#include <vector>

#include "glite/jobid/JobId.h"

#include "glite/lb/jobstat.h"
#include "glite/lb/CountRef.h"

EWL_BEGIN_NAMESPACE;

/** Class representing status of job.
 * 
 * This class is used to represent all information about the job status
 * as computed and stored in the bookkeeping server's database. Job
 * status is, like Event, list of attribute -- value pairs. One of the
 * attributes is the job's state as seen by the L&B, ie. something
 * like <tt>RUNNING</tt> or  * <tt>DONE</tt>, other attributes contain
 * more information about the job.
 *
 * The JobStatus class provides methods for reading values of these
 * attributes and it is used as a result of server queries.
 */
class JobStatus {
	friend class Job;
	friend class CountRef<JobStatus>;
public:

	/** Symbolic names of job states. */
	enum Code {
		UNDEF = 0,	/**< Indicates invalid, i.e. uninitialized instance. */
@@@{
	for my $stat ($status->getTypesOrdered)
	{
		my $u = uc($stat);
		my $c = getTypeComment $status $stat;
		gen qq{
!		$u,	/**< $c */
};
	}
@@@}
		CODE_MAX /**< Limit for range checking. */
	};

	/** Symbolic names of attributes.
	 *
	 * These constants are used for naming individual attributes
	 * when invoking their access methods.
	 */
	enum Attr {
@@@{
	require './StatusAttrNames.pl';
	my %StatusAttrNums;
	my $stno = 0;
	for (@main::StatusAttrNames) {
		$StatusAttrNums{$_} = $stno++;
	}

	my $max = 0;
	selectType $status '_common_';
	for my $u (sort {$a cmp $b} getAllFields $status) {
		selectField $status $u;
		my $f = getField $status;
		$u =~ s/([a-z])([A-Z])/$1_$2/g;
		$u = uc $u;

		gen defined($StatusAttrNums{$u}) ? 
			"\t/** $f->{comment} */\n\t\t$u = $StatusAttrNums{$u},\n" :
			"#error \"$u not defined in StatusAttrNames\"\n";
		$max = $StatusAttrNums{$u} if defined($StatusAttrNums{$u}) && $StatusAttrNums{$u} > $max;
	}
	$max++;
	gen "\t\tATTR_MAX = $max /**< Limit for range checking. */\n";
@@@}
	};

@@@{
	selectType $status '_common_';
	for my $n (sort {$a cmp $b} getAllFields $status) {
		selectField $status $n;
		my $f = getField $status;
		if ($f->{codes}) {
                        my $n = uc getName $f;
			gen qq{
!	enum \{
};
			for (@{$f->{codes}}) {
				my $name = $_->{name};
				$name =~ tr/-/_/; 
				gen qq{
!		$n\_$name,	/**< $_->{comment} */
};
			}
			gen qq{
!	\};
};
		}
	}
@@@}
	/** Symbolic names of attribute types.
	 *
	 * These constants are used to name the various attribute
	 * types.
	 */
	enum AttrType { INT_T, /**< Integer type. */
			STRING_T, /**< String type. */
			TIMEVAL_T, /**< <tt>struct timeval</tt> type. */
			BOOL_T, /**< Boolean type (true or false). */
			JOBID_T, /**< Job id type. */
			INTLIST_T, /**< List of integer values. */
			STRLIST_T, /**< List of string values. */
			TAGLIST_T, /**< List of user tags. */
			STSLIST_T,  /**< List of states. */
			CCLASSAD_T /**< Unparsed classad */
	};
  
	/** Numeric status code. 
	 *
	 * This code represents the state of the job.
	 * \see Code.
	 */
	Code	status;
  
	/** Get state name.
	 *
	 * Returns string representation of the job's state.
	 */
	const std::string & name(void) const;
  
	/** Access method for attribute values.
	 *
	 * Retrieve integer value of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	int	getValInt(Attr name) const;
  
	/** Access method for attribute values.
	 *
	 * Retrieve string value of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	std::string getValString(Attr name) const;
  
	/** Access method for attribute values.
	 *
	 * Retrieve <tt>struct timeval</tt>  value of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	struct timeval	getValTime(Attr name) const;
  
	/** Access method for attribute values.
	 *
	 * Retrieve JobId value of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	const glite::jobid::JobId  getValJobId(Attr name) const;

	/** Access method for attribute values.
	 *
	 * Retrieve <tt>bool</tt> value of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	bool getValBool(Attr name) const;

	/** Access method for attribute values.
	 *
	 * Retrieve integer values of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	const std::vector<int> getValIntList(Attr name) const;

	/** Access method for attribute values.
	 *
	 * Retrieve string values of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	const std::vector<std::string> getValStringList(Attr name) const;

	/** Access method for attribute values.
	 *
	 * Retrieve user tags values of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	const std::vector<std::pair<std::string,std::string> > getValTagList(Attr name) const;

	/** Access method for attribute values.
	 *
	 * Retrieve status values of named attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Value of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	const std::vector<JobStatus> getValJobStatusList(Attr name) const;

	/** Get name of attribute.
	 *
	 * Retrieve string representation of symbolic name of attribute.
	 * \param[in] name Symbolic name of attribute.
	 * \returns Name of attribute.
	 * \throws LoggingException Invalid attribute name.
	 */
	static const std::string& getAttrName(Attr name);

	/** Find attribute by name (case insensitive)
	 * \param[in] name String name.
	 * \returns enum attr value.
	 * \throws LoggingException Invalid attribute name.
	 */
	static Attr attrByName(std::string const &);
  
	/** List of attributes and their types valid for this
	 * instance.
	 *
	 * Returns the vector of (attribute, attribute type) pairs
	 * that this instance of JobStatus contains. 
	 * \returns List of attributes.
	 */
	const std::vector<std::pair<Attr,AttrType> >& getAttrs(void) const;

	/** Get name of state.
	 *
	 * Retrieve string representation of symbolic job state.
	 * \param[in] state Symbolic state name.
	 * \returns String state name.
	 * \throw Exception Invalid state.
	 *
	 */
	static const std::string &getStateName(Code state);
  
	/** Default constructor.
	 *
	 * Initializes an empty instance.
	 */
	JobStatus(void);

        /** Copy constructor.
	 *
	 * Creates identical copy of the original object.
	 * The underlying C struct edg_wll_JobStatus is shared using
	 * the CountRef mechanism.
	 * \param[in] orig Original.
	 */
	JobStatus(const JobStatus &orig);

	/** Assignment operator.
	 *
	 * Creates identical copy of the original object.
	 * The underlying C struct edg_wll_JobStatus is shared using
	 * the CountRef mechanism.
	 * \param[in] orig Original.
	 */
	JobStatus & operator=(const JobStatus &orig);

        /** Constructor from the C type.
	 *
	 * Encapsulates the given struct.
	 * \param[in] src C struct that holds the status.
	 * \param[in] destroy destroy src after use.
	 */
	JobStatus(const edg_wll_JobStat &src,int destroy = 1);

        /** Assignment from the C type.
	 *
	 * Encapsulates the given struct.
	 * \param[in] src C struct that holds the status.
	 */
	JobStatus & operator=(const edg_wll_JobStat& src);

        /** Destructor.
	 *
	 * Releases the encapsulated C struct.
	 */
	virtual ~JobStatus();

protected:
	edg_wll_JobStat *c_ptr(void);

private:
	static void	destroyFlesh(void *);
	CountRef<JobStatus> *flesh;
};

EWL_END_NAMESPACE;

#endif /* GLITE_LB_JOBSTATUS_HPP */
