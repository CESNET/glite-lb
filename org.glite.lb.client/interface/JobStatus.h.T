#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_JOBSTATUS_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_JOBSTATUS_HPP__

/*
@@@AUTO
*/

@@@LANG: C++

#include "edg/workload/logging/client/CountRef.h"
#include "edg/workload/common/jobid/JobId.h"

#include "edg/workload/common/jobid/jobid.h"
#include <sys/time.h>
#include "edg/workload/logging/client/jobstat.h"

#include <string>
#include <utility>
#include <vector>

EWL_BEGIN_NAMESPACE;

/**
 * Description of job status.
 * The status is computed from a sequence of logged events
 */


class JobStatus {
	friend class Job;
	friend class CountRef<JobStatus>;
public:
	enum Code {
		UNDEF = 0,	/**< indicates invalid, i.e. uninitialized instance */
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
		CODE_MAX
	};

	enum Attr {
@@@{
	selectType $status '_common_';
	for my $u (sort {$a cmp $b} getAllFields $status) {
		selectField $status $u;
		my $f = getField $status;
		$u =~ s/([a-z])([A-Z])/$1_$2/g;
		$u = uc $u;

		gen "\t/** $f->{comment} */\n\t\t$u,\n";
	}
@@@}
		ATTR_MAX
	};

@@@{
	selectType $status '_common_';
	for my $n (getAllFields $status) {
		selectField $status $n;
		my $f = getField $status;
		if ($f->{codes}) {
                        my $n = uc getName $f;
			gen qq{
!	enum \{
};
			for (@{$f->{codes}}) {
				gen qq{
!		$n\_$_->{name},	/**< $_->{comment} */
};
			}
			gen qq{
!	\};
};
		}
	}
@@@}
	enum AttrType { INT_T, 
			STRING_T, 
			TIMEVAL_T, 
			BOOL_T,
			JOBID_T,
			INTLIST_T, 
			STRLIST_T, 
			TAGLIST_T, 
			STSLIST_T 
	};
  
	/** Numeric status code */
	Code	status;
  
	/** String representation of the status code */
	const std::string & name(void) const;
  
	/** Retrieve integer attribute */
	int	getValInt(Attr) const;
  
	/** Retrieve string attribute */
	std::string getValString(Attr) const;
  
	/** Retrieve time attribute */
	struct timeval	getValTime(Attr) const;
  
	/** Retrieve jobid attribute */
	const edg::workload::common::jobid::JobId  getValJobId(Attr) const;

	/** Retrieve bool attribute */
	bool getValBool(Attr) const;

	/** Retrieve int list attribute */
	const std::vector<int> getValIntList(Attr) const;

	/** Retrieve string list attribute */
	const std::vector<std::string> getValStringList(Attr) const;

	/** Retrieve tag list attribute */
	const std::vector<std::pair<std::string,std::string> > getValTagList(Attr) const;

	/** Retrieve job status list attribute */
	const std::vector<JobStatus> getValJobStatusList(Attr) const;

	/** Attribute name */
	const std::string& getAttrName(Attr) const;
  
	/** List of attributes and types valid for this instance */
	const std::vector<std::pair<Attr,AttrType> >& getAttrs(void) const;
  
	JobStatus(void);
	JobStatus(const JobStatus &);
	JobStatus & operator=(const JobStatus &);
	JobStatus(const edg_wll_JobStat &);
	JobStatus & operator=(const edg_wll_JobStat&);
	virtual ~JobStatus();

protected:
	edg_wll_JobStat *c_ptr(void);

private:
	static void	destroyFlesh(void *);
	CountRef<JobStatus> *flesh;
};

EWL_END_NAMESPACE;

#endif

