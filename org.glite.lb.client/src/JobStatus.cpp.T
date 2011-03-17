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

#include <utility>
#include <vector>
#include <string>
#include <errno.h>
#include <stdlib.h>

#include "glite/jobid/JobId.h"
#include "glite/lb/LoggingExceptions.h"

#include "consumer.h"
#include "JobStatus.h"


EWL_BEGIN_NAMESPACE;

#define CLASS_PREFIX "glite::lb::JobStatus::"

@@@{

require './StatusAttrNames.pl';

sub typeswitch {
        my ($ftype,$ctype,$resc) = @_;
        local $_;

	$resc = "break;" unless $resc;

        gen $indent."switch (attr) {\n";
        selectType $status $ftype;
        for (getFields $status) {
                my $f = selectField $status $_;
                if($f->{type} eq $ctype) {
		  my $fnu = $f->{name};
		  $fnu =~ s/([a-z])([A-Z])/$1_$2/g;
		  $fnu = uc $fnu;
		  my $cname = getName $f 'C';
		  gen "$indent\tcase JobStatus::$fnu: return cstat->$cname;\n"
		};
		# XXX: when there are conversion functions, we may get these as strings as well
		#elsif (($f->{type} eq "int") && 
		#	 ($ctype eq "string") && 
		#	 $f->{codes}) {
		#  my $fnu = $f->{name};
		#  $fnu =~ s/([a-z])([A-Z])/$1_$2/g;
		#  $fnu = uc $fnu;
		#  my $cname = getName $f 'C';
		#  my $fn = $ftype eq '_common_' ? "" : ucfirst $ftype;
		#  my $c =  $fn . ucfirst $f->{name}; 
		#  $cast = ($c eq 'Level') ? "(edg_wll_Level)" : "";
		#  gen "$indent\tcase JobStatus::$fnu: return((const char *)edg_wll_${c}ToString(${cast}cstat->$cname));\n";		}
        }
        gen "$indent\tdefault: $resc\n" if $resc;
        gen "$indent\}\n";
}
@@@}

JobStatus::JobStatus(void)
{
  status = UNDEF;
  flesh = 0;
}

JobStatus::JobStatus(const JobStatus & in)
{
  status = in.status;
  flesh = in.flesh;
  if (flesh) flesh->use();
}

JobStatus & 
JobStatus::operator=(const JobStatus & in)
{
  if (flesh) flesh->release();
  status = in.status;
  flesh = in.flesh;
  if (flesh) flesh->use();
  return *this;
}

JobStatus::JobStatus(const edg_wll_JobStat & in,int destroy)
{
  status = (Code)in.state;
  flesh = new CountRef<JobStatus>((void*)&in);
  if (!destroy) flesh->use();
}
 
JobStatus & 
JobStatus::operator=(const edg_wll_JobStat & in)
{
  if(flesh)
    flesh->release();
  status = (Code)in.state;
  flesh = new CountRef<JobStatus>((void*)&in);
  return(*this);
}

JobStatus::~JobStatus()
{
  if (flesh) flesh->release();
}

edg_wll_JobStat *
JobStatus::c_ptr(void)
{
  edg_wll_JobStat *s;

  if(flesh)
    return((edg_wll_JobStat*)flesh->ptr);

  s = new edg_wll_JobStat;
  // XXX - is it neccessary? new should throw exception itself... 
  if(!s) throw(Exception(EXCEPTION_MANDATORY,
			 ENOMEM,
			 "out of memory allocating c-struct for JobStatus"));

  edg_wll_InitStatus(s);
  flesh = new CountRef<JobStatus>((void*)s);
  return(s);
}


static std::string const names[JobStatus::CODE_MAX] = {
	"undefined",
@@@{
	for (sort { $status->{order}->{$a} <=> $status->{order}->{$b} }
		$status->getTypes)
	{
		gen "\t\"$_\",\n";
	}
@@@}
};

const std::string& JobStatus::name(void) const
{
  if (status<0 || status>=CODE_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY, 
		    EINVAL,
		    "status code invalid"));
  }
  return names[status];
}

int JobStatus::getValInt(JobStatus::Attr attr) const
{
  edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;
@@@{
    $indent = "\t";
    typeswitch '_common_','int';
@@@}

 STACK_ADD; 
 throw(Exception(EXCEPTION_MANDATORY,
		 ENOENT,
		 "no such attribute"));
 return -1;	/* make gcc shut up -- never returns */
}

bool JobStatus::getValBool(JobStatus::Attr attr) const
{
  edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;
@@@{
    $indent = "\t";
    typeswitch '_common_','bool';
@@@}

 STACK_ADD; 
 throw(Exception(EXCEPTION_MANDATORY,
		 ENOENT,
		 "no such attribute"));
 return -1;	/* make gcc shut up -- never returns */
}

static const char *
get_string_val(const edg_wll_JobStat *cstat,JobStatus::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_','string';
@@@}
 STACK_ADD;
 throw(Exception(EXCEPTION_MANDATORY,
		 ENOENT,
		 "no such attribute"));
 return 0;	/* make gcc shut up -- never returns */
}

std::string 
JobStatus::getValString(JobStatus::Attr attr) const
{
  edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;
  std::string	ret;
  
  try{
    const char *s = get_string_val(cstat,attr);
    if (s) ret.assign(s);
    return ret;
  } catch (Exception &e) {
    STACK_ADD;
    throw;
  }
}


struct timeval 
JobStatus::getValTime(Attr attr) const
{
  edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;

@@@{
	$indent = "\t";
	typeswitch '_common_','timeval';
@@@}
 STACK_ADD;
 throw(Exception(EXCEPTION_MANDATORY,
		 ENOENT,
		 "no such attribute"));
}


const
edg_wlc_JobId 
get_val_jobid(edg_wll_JobStat const *cstat, JobStatus::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_','jobid';
@@@}
 STACK_ADD;
 throw(Exception(EXCEPTION_MANDATORY,
		 ENOENT,
		 "no such attribute"));
 return 0;	/* make gcc shut up -- never returns */
}


const 
glite::jobid::JobId  
JobStatus::getValJobId(Attr attr) const
{
  edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;

  try {
	  edg_wlc_JobId job_id = get_val_jobid(cstat, attr);
	  return(glite::jobid::JobId(job_id));
  }
  catch(Exception &e) {
	  STACK_ADD;
	  throw;
  }
}

const
int *
get_val_intlist(edg_wll_JobStat const *cstat, JobStatus::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_','intlist';
@@@}
  STACK_ADD;
  throw(Exception(EXCEPTION_MANDATORY,
		  ENOENT,
		  "no such attribute"));
  return 0;	/* make gcc shut up -- never returns */
}


const 
std::vector<int>
JobStatus::getValIntList(Attr attr) const
{
	edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;

	std::vector<int> result;
	const int *r;
	
	try {
		r = get_val_intlist(cstat, attr);
		if(r)
			for(int i = 1; i <= r[0]; i++)
				result.push_back(r[i]);
	} catch(Exception &e) {
		STACK_ADD;
		throw;
	}
	return result;
}


char ** const
get_val_stringlist(edg_wll_JobStat const *cstat, JobStatus::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_','strlist';
@@@}
  STACK_ADD;
  throw(Exception(EXCEPTION_MANDATORY,
		  ENOENT,
		  "no such attribute"));
  return 0;	/* make gcc shut up -- never returns */
}


const
std::vector<std::string>
JobStatus::getValStringList(Attr attr) const
{
	edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;

	std::vector<std::string> result;
	char **r , **p;

	try {
		r = (char**)get_val_stringlist(cstat, attr);
		if(r)
			for(p = r; *p; p++)
				result.push_back(std::string(*p));
	} catch(Exception &e) {
		STACK_ADD;
		throw;
	}
	return result;
}


edg_wll_TagValue * const
get_val_taglist(edg_wll_JobStat const *cstat, JobStatus::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_','taglist';
@@@}
  STACK_ADD;
  throw(Exception(EXCEPTION_MANDATORY,
		  ENOENT,
		  "no such attribute"));
  return 0;	/* make gcc shut up -- never returns */
}


const
std::vector<std::pair<std::string,std::string> >
JobStatus::getValTagList(Attr attr) const
{
	edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;

	std::vector<std::pair<std::string,std::string> > result;
	edg_wll_TagValue *r , *p;

	try {
		r = get_val_taglist(cstat, attr);
		if(r)
			for(p = r; p->tag ; p++)
				result.push_back(std::pair<std::string,std::string>
							(std::string(p->tag),std::string(p->value)));
	} catch(Exception &e) {
		STACK_ADD;
		throw;
	}
	return result;
}


const
edg_wll_JobStat *
get_val_stslist(edg_wll_JobStat const *cstat, JobStatus::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_','stslist';
@@@}
  STACK_ADD;
  throw(Exception(EXCEPTION_MANDATORY,
		  ENOENT,
		  "no such attribute"));
  return 0;	/* make gcc shut up -- never returns */
}


const
std::vector<JobStatus>
JobStatus::getValJobStatusList(Attr attr) const
{
	edg_wll_JobStat const *cstat = (edg_wll_JobStat *) flesh->ptr;

	std::vector<JobStatus> result;
	const edg_wll_JobStat *r, *p;

	try {
		r = get_val_stslist(cstat, attr);
		if(r)
			for(p=r; p->state != EDG_WLL_JOB_UNDEF; p++) {
				edg_wll_JobStat *jsep = new edg_wll_JobStat;
				if (!edg_wll_CpyStatus(p,jsep)) {
					STACK_ADD;
  					throw(Exception(EXCEPTION_MANDATORY,
		  			ENOMEM,
		  			"cannot copy edg_wll_JobStat"));
				}
				result.push_back(JobStatus(*jsep));
			}
	} catch(Exception &e) {
		STACK_ADD;
		throw;
	}
	return(result);
}


static std::string const attr_names[JobStatus::ATTR_MAX] = {
@@@{
	my %s;

	for (getAllFields $status) {
		my $id = $_;
		$id =~ s/([a-z])([A-Z])/$1_$2/g;
		$s{uc($id)} = $_;
		$s{uc($_)} = $_;
	}
	for (@main::StatusAttrNames) {
		gen "\t\"$s{$_}\",\n";
	}
@@@}
};

const std::string & 
JobStatus::getAttrName(JobStatus::Attr attr)
{
  if (attr<0 || attr>=ATTR_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY,
		    ENOENT,
		    "no such attribute"));
  }

  return attr_names[attr];
}

JobStatus::Attr JobStatus::attrByName(std::string const & name)
{
	int	a;
	for (a=0; a<ATTR_MAX; a++) {
		if (strcasecmp(attr_names[a].c_str(),name.c_str()) == 0) return (JobStatus::Attr) a;
	}
	STACK_ADD;
	throw(Exception(EXCEPTION_MANDATORY,EINVAL,"no such attribute"));
}


typedef std::pair<JobStatus::Attr,JobStatus::AttrType> tpair;
static std::vector<tpair> attrs;

static bool attrs_inited = false;

static void init_attrs(void)
{
/* XXX: currently only common attributes in JobStatus */
@@@{
	selectType $status '_common_';
	for (getFields $status) {
		my $fu = $_;
		my $f = selectField $status $_;
                my $ftu = uc "$f->{type}_T";
		$fu =~ s/([a-z])([A-Z])/$1_$2/g;
		$fu = uc $fu;

		gen "\tattrs.push_back(tpair(JobStatus::$fu,JobStatus::$ftu));\n";
	}
@@@}
}


const std::vector<tpair>& 
JobStatus::getAttrs(void) const
{
  if (status<0 || status>=CODE_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY,
		    EINVAL,
		    "status code invalid"));
  }

/* FIXME: thread safety */
  if (!attrs_inited) {
    init_attrs();
    attrs_inited = true;
  }
  return attrs;
}

const std::string &
JobStatus::getStateName(Code state)
{
  if (state<0 || state>=CODE_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY, 
		    EINVAL,
		    "status code invalid"));
  }
  return names[state];
}


void 
JobStatus::destroyFlesh(void *p)
{
  edg_wll_JobStat *stat = (edg_wll_JobStat *) p;
  if (stat) {
    edg_wll_FreeStatus(stat);
    free(stat);
  }
}

EWL_END_NAMESPACE;
