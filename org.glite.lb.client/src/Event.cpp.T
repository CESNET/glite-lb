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

#include <utility>
#include <vector>
#include <string>
#include <stdio.h>

#include <errno.h>

#include "glite/jobid/cjobid.h"

#include "Event.h"
#include "glite/lb/events.h"
#include "glite/lb/notifid.h"
#include "glite/lb/LoggingExceptions.h"

#include "consumer.h"

EWL_BEGIN_NAMESPACE;

#define CLASS_PREFIX "glite::lb::Event::"

@@@{

require './EventAttrNames.pl';

sub typeswitch {
	my $ftype = shift;
	my $resc = shift;
	local $_;
	my %ctype;
	$ctype{$_} = 1 while $_ = shift;

	$resc = 'break;' unless $resc;

	gen $indent."switch (attr) {\n";
	selectType $event $ftype;
	for (getFields $event) {
		my $f = selectField $event $_;
		if($ctype{$f->{type}}) {
  		  my $cstr = $ftype eq '_common_' ? 'any' : lcfirst $ftype;
		  if ($cstr =~ m/^pBS/) { $cstr = ucfirst $cstr; }
		  if ($cstr =~ m/^cREAM/) { $cstr = ucfirst $cstr; }
		  if ($cstr =~ m/^condor/) { $cstr = ucfirst $cstr; }
		  my $cname = getName $f 'C';
		  gen "$indent\tcase Event::".uc($f->{name}).": return(cev->$cstr.$cname);\n";
		} elsif (($f->{type} eq "int") && 
			 ($ctype{"string"}) && 
			 $f->{codes}) {
		  # conversion from int to string (well, enum to string)
  		  my $cstr = $ftype eq '_common_' ? 'any' : lcfirst $ftype;
		  if ($cstr =~ m/^pBS/) { $cstr = ucfirst $cstr; }
		  if ($cstr =~ m/^cREAM/) { $cstr = ucfirst $cstr; }
		  if ($cstr =~ m/^condor/) { $cstr = ucfirst $cstr; }
		  my $cname = getName $f 'C';
		  my $fn = $ftype eq '_common_' ? "" : ucfirst $ftype;
		  my $c =  $fn . ucfirst $f->{name}; 
		  $cast = ($c eq 'Level') ? "(edg_wll_Level)" : "";
		  gen "$indent\tcase Event::".uc($f->{name}).": return((const char *)edg_wll_${c}ToString(${cast}cev->$cstr.$cname));\n";
		} elsif (($f->{type} eq "jobid") &&
			 ($ctype{"string"})) {
                  # conversion from jobid to string
		}
	}
	gen "$indent\tdefault: $resc\n" if $resc;
	gen "$indent\}\n";
}
@@@}

Event::Event(void)
{
  type = UNDEF;
  flesh = 0;
}


Event::Event(const Event &in)
{
  type = in.type;
  flesh = in.flesh;
  if (flesh) flesh->use();
}


Event::Event(edg_wll_Event *in)
{
  type = (Type)in->type;
  flesh = new CountRef<Event>((void*)in);
}


Event::~Event(void)
{
  if (flesh) flesh->release();
}


Event & 
Event::operator= (const Event &in)
{
  if (flesh) flesh->release();
  type = in.type;
  flesh = in.flesh;
  if (flesh) flesh->use();
  
  return *this;
}

int Event::getValInt(Attr attr)	const
{
	edg_wll_Event const	*cev = (edg_wll_Event *) flesh->ptr;

@@@{
	$indent = "\t";
	typeswitch '_common_',undef,'int','port','logsrc';
@@@}

	switch (cev->type) {
@@@{
	$indent = "\t\t";
	for my $t (getTypes $event) {
		gen "\t\tcase ".uc($t).":\n";
		typeswitch $t,'goto badattr;','int','port','logsrc';
	}
@@@}
		default:
		  STACK_ADD;
		  throw(Exception(EXCEPTION_MANDATORY, EINVAL,
				  "attribute is not of int type"));
        }
badattr:
        STACK_ADD;
        throw(Exception(EXCEPTION_MANDATORY, ENOENT, "invalid attribute"));
	return -1; /* gcc, shut up! */
}

float Event::getValFloat(Attr attr)	const
{
	edg_wll_Event const	*cev = (edg_wll_Event *) flesh->ptr;

@@@{
	$indent = "\t";
	typeswitch '_common_',undef,'float';
@@@}

	switch (cev->type) {
@@@{
	$indent = "\t\t";
	for my $t (getTypes $event) {
		gen "\t\tcase ".uc($t).":\n";
		typeswitch $t,'goto badattr;','float';
	}
@@@}
		default:
		  STACK_ADD;
		  throw(Exception(EXCEPTION_MANDATORY, EINVAL,
				  "attribute is not of float type"));
        }
badattr:
        STACK_ADD;
        throw(Exception(EXCEPTION_MANDATORY, ENOENT, "invalid attribute"));
	return -1; /* gcc, shut up! */
}

double Event::getValDouble(Attr attr)	const
{
	edg_wll_Event const	*cev = (edg_wll_Event *) flesh->ptr;

@@@{
	$indent = "\t";
	typeswitch '_common_',undef,'double';
@@@}

	switch (cev->type) {
@@@{
	$indent = "\t\t";
	for my $t (getTypes $event) {
		gen "\t\tcase ".uc($t).":\n";
		typeswitch $t,'goto badattr;','double';
	}
@@@}
		default:
		  STACK_ADD;
		  throw(Exception(EXCEPTION_MANDATORY, EINVAL,
				  "attribute is not of double type"));
        }
badattr:
        STACK_ADD;
        throw(Exception(EXCEPTION_MANDATORY, ENOENT, "invalid attribute"));
	return -1; /* gcc, shut up! */
}

static char const *get_string_val(const edg_wll_Event *cev, Event::Attr attr)
{
@@@{
	$indent = "\t";
	typeswitch '_common_',undef,'string';
@@@}

	switch (cev->type) {
@@@{
	$indent = "\t\t";
	for my $t (getTypes $event) {
		gen "\t\tcase Event::".uc($t).":\n";
		typeswitch $t,'goto badattr;','string';
	}
@@@}
		default:
		  STACK_ADD;
		  throw(Exception(EXCEPTION_MANDATORY, EINVAL, 
				  "attribute is not of string type and can not be converted"));
	}
badattr:
	STACK_ADD;
	throw(Exception(EXCEPTION_MANDATORY, ENOENT, "invalid attribute"));
	return NULL; /* gcc, shut up! */
}


std::string 
Event::getValString(Attr attr) const
{
  edg_wll_Event const *cev = (edg_wll_Event *) flesh->ptr;
  std::string	ret;
  
  try {
    char const	*s = get_string_val(cev,attr);
    if (s) ret.assign(s);
    return ret;
  } catch (Exception &e) {
    STACK_ADD;
    throw;
  }
}


struct timeval 
Event::getValTime(Attr attr) const
{
  edg_wll_Event const *cev = (edg_wll_Event *) flesh->ptr;
@@@{
    $indent = "\t";
    typeswitch '_common_',undef,'timeval';
@@@}
  
  /* XXX
   * to make things simpler we don't include this here as there are no
   * type specific timeval attributes currently
   *
   *	switch (cev->type) {
   */

  STACK_ADD; 
  throw(Exception(EXCEPTION_MANDATORY, ENOENT, "invalid attribute"));
}

static
edg_wlc_JobId 
get_val_jobid(edg_wll_Event const *cev, Event::Attr attr) 
{
@@@{
        $indent = "\t";
        typeswitch '_common_', undef,'jobid';
@@@}
	switch (cev->type) {
@@@{
	$indent = "\t\t";
	for my $t (getTypes $event) {
		gen "\t\tcase Event::".uc($t).":\n";
		typeswitch $t,'goto badattr;','jobid';
	}
@@@}
		default:
		  STACK_ADD;
		  throw(Exception(EXCEPTION_MANDATORY, EINVAL, 
				  "attribute is not of jobid type"));
	}
badattr:
	STACK_ADD;
	throw(Exception(EXCEPTION_MANDATORY, ENOENT, "invalid attribute"));
	return NULL; /* gcc, shut up! */
}

const 
glite::jobid::JobId 
Event::getValJobId(Attr attr) const
{
	edg_wll_Event const *cev = (edg_wll_Event *) flesh->ptr;
	try {
		edg_wlc_JobId job_id = get_val_jobid(cev,attr);
		return(glite::jobid::JobId(job_id));
	} 
	catch (Exception &e) {
		STACK_ADD;
		throw;
	}

}


static std::string const names[Event::TYPE_MAX] = {
	"undefined",
@@@{
  for (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
	 $event->getTypes)
  {
    gen "\t\"$_\",\n";
  }
@@@}
};

const std::string  & 
Event::name(void) const
{
  if (type<0 || type>TYPE_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY, EINVAL, "invalid event type"));
  }
  return names[type];
}


static 
std::string const attr_names[Event::ATTR_MAX] = {
@@@{
	my %s;

	for (getAllFields $event) {
		my $id = $_;
		$id =~ s/([a-z])([A-Z])/$1_$2/g;
		$s{uc($id)} = $_;
		$s{uc($_)} = $_;
	}
	for (@main::EventAttrNames) {
		gen "\t\"$s{$_}\",\n";
	}
@@@}
};

const std::string & 
Event::getAttrName(Attr attr)
{
  if (attr<0 || attr>=ATTR_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY, EINVAL, "invalid attribute"));
  }
  return attr_names[attr];
}


typedef std::pair<Event::Attr,Event::AttrType> tpair;
static std::vector<tpair> attrs[Event::TYPE_MAX];

#define apush(etype,attr,atype) \
	attrs[etype].push_back(tpair(attr,atype))

static bool attrs_inited = false;

static void init_attrs(void)
{
@@@{
	for my $t (getTypes $event) {
		my $tu = uc $t;
		selectType $event '_common_';
		for (getFields $event) {
			my $fu = uc $_;
			my $f = selectField $event $_;
			my $ftu = uc "$f->{type}_T";
			gen "\tapush(Event::$tu,Event::$fu,Event::$ftu);\n";
		}
		selectType $event $t;
		for (getFields $event) {
			my $fu = uc $_;
			my $f = selectField $event $_;
			my $ftu = uc "$f->{type}_T";
			gen "\tapush(Event::$tu,Event::$fu,Event::$ftu);\n";
		}
	}
@@@}
}

std::vector<std::pair<Event::Attr,Event::AttrType> > const & Event::getAttrs(void) const
{
  if (type<0 || type>=TYPE_MAX) {
    STACK_ADD;
    throw(Exception(EXCEPTION_MANDATORY, EINVAL, "invalid event type"));
  }
  
  /* FIXME: thread safety */
  if (!attrs_inited) {
    init_attrs();
    attrs_inited = true;
  }
  
  return attrs[type];
}


const std::string Event::getEventName(Type type)
{
	char *s;

	if(type < 0 || type >= TYPE_MAX) {
		STACK_ADD;
		throw(Exception(EXCEPTION_MANDATORY, EINVAL, "invalid event type"));
	}

	s = edg_wll_EventToString(edg_wll_EventCode(type));
	if (!s) {
		STACK_ADD;
		throw(Exception(EXCEPTION_MANDATORY, EINVAL, "invalid event type"));
	}
	std::string ret(s);
	free(s);
	return ret;
}


void 
Event::destroyFlesh(void *in) 
{
  edg_wll_FreeEvent((edg_wll_Event *) in);
  free(in);
}

EWL_END_NAMESPACE;
