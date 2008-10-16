#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
//#include <classad.h>

#include "glite/lb/LoggingExceptions.h"
#include "JobStatus.h"
#include "notification.h"

#include "stat_fields.h"

using namespace glite::lb;

typedef std::pair<JobStatus::Attr,std::string> FieldPair;

char * glite_lb_parse_stat_fields(const char *arg,void **out)
{
	char	*aux = strdup(arg),*p;
	std::vector<FieldPair>	*fields = new std::vector<FieldPair>;

	for (p = strtok(aux,","); p; p = strtok(NULL,",")) {
		/*special treatment for JDL (and possibly other) two-valued fields with ':' used as a separator  */
		if (strncasecmp("jdl:", p, 4)) {
			try { fields->push_back(std::make_pair(JobStatus::attrByName(p), "")); }
			catch (std::runtime_error &e) { delete fields; return p; };
		}
		else {
			try { fields->push_back(std::make_pair(JobStatus::attrByName("jdl"), p + 4)); }
			catch (std::runtime_error &e) { delete fields; return p; };
		}
	}
	
	*out = (void *) fields;
	return NULL;
}


static std::string & escape(std::string &s)
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

void glite_lb_dump_stat_fields(void)
{
	JobStatus	s;
	attrs_t 	a = s.getAttrs(); 
	for (attrs_t::iterator i=a.begin(); i != a.end(); i++) {
		switch (i->second) {
			case JobStatus::INT_T:
			case JobStatus::STRING_T:
			case JobStatus::TIMEVAL_T:
				std::cerr << JobStatus::getAttrName(i->first) << ", ";
			default: break;
		}
	}
}

extern "C" { char * TimeToStr(time_t); }

void glite_lb_print_stat_fields(void **ff,edg_wll_JobStat *s)
{
	std::vector<FieldPair>	*fields = (std::vector<FieldPair> *) ff;
	JobStatus	stat(*s,0);
	attrs_t 	attrs = stat.getAttrs();
	attrs_t::iterator a;
	std::vector<FieldPair>::iterator f;
	std::string val;
	struct timeval t;
	JobStatus::Attr attr;
	char *jdl_param = NULL;

	std::cout << glite_jobid_unparse(s->jobId) << '\t' << stat.name() << '\t';

	for (f = fields->begin(); f != fields->end(); f++) {
		for (a = attrs.begin(); a != attrs.end() && a->first != f->first; a++);
		if (a != attrs.end() ) {
			attr = (a->first);
			switch (a->second) {
				case (JobStatus::INT_T):
					std::cout << stat.getValInt(attr) << '\t';
					break;
				case (JobStatus::STRING_T):
					if (attr != JobStatus::JDL) {
						val = stat.getValString(attr);
						std::cout << (val.empty() ? "(null)" : escape(val)) << '\t'; 
					}
					else {
                                                val = f->second;
                                                jdl_param = edg_wll_JDLField(s, val.c_str());
                                                std::cout << (jdl_param ? jdl_param : "(null)") << '\t'; 
                                                free(jdl_param); jdl_param = NULL;
					}
					break;
				case (JobStatus::TIMEVAL_T): 
					t = stat.getValTime(attr);
					std::cout << TimeToStr(t.tv_sec) << '\t'; 
					break;
				default:
					std::cout << "(unsupported)";
					break;
			}
		}
	}
	std::cout << std::endl;
}
