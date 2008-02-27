#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>

#include "glite/lb/LoggingExceptions.h"
#include "glite/lb/JobStatus.h"
#include "glite/lb/notification.h"

#include "notify_supp.h"

using namespace glite::lb;

char * parse_fields(const char *arg,void **out)
{
	char	*aux = strdup(arg),*p;
	std::vector<JobStatus::Attr>	*fields = new std::vector<JobStatus::Attr>;

	for (p = strtok(aux,","); p; p = strtok(NULL,",")) {
		try { fields->push_back(JobStatus::attrByName(p)); }
		catch (std::runtime_error &e) { delete fields; return p; };
	}
	
	*out = (void *) fields;
	return NULL;
}

std::string & escape(std::string &s)
{
	for (std::string::iterator p = s.begin(); p < s.end(); p++) switch (*p) {
		case '\n':
		case '\t':
			s.insert(p-s.begin(),"\\"); p++;
			break;
		default: break;
	}
	return s;
}

typedef std::vector<std::pair<JobStatus::Attr,JobStatus::AttrType> > attrs_t;

void print_fields(void **ff,const edg_wll_NotifId n,edg_wll_JobStat const *s)
{
	std::vector<JobStatus::Attr>	*fields = (std::vector<JobStatus::Attr> *) ff;
	JobStatus	stat(*s,0);
	attrs_t 	attrs = stat.getAttrs();
	attrs_t::iterator a;

	std::cout << glite_jobid_unparse(s->jobId) << '\t' << stat.name() << '\t';

	for (std::vector<JobStatus::Attr>::iterator f = fields->begin(); f != fields->end(); f++) {
		for (a = attrs.begin(); a != attrs.end() && a->first != *f; a++);
		if (a != attrs.end() ) switch (a->second) {
			case JobStatus::INT_T:
				std::cout << stat.getValInt(a->first) << '\t';
				break;
			case JobStatus::STRING_T: {
				std::string val = stat.getValString(a->first);
				std::cout << (val.empty() ? "(null)" : escape(val)) << '\t';
				} break;
			default:
				std::cout << "(unsupported)";
				break;
		}
	}
	std::cout << std::endl;
}
