//#ident "$Header$"

/**
 * @file ServerConnection.cpp
 * @version $Revision$
 */
#include <string>
#include <utility>
#include <vector>

#include <time.h>
#include <errno.h>
#include <stdio.h>

#include <expat.h>

#include "glite/wmsutils/jobid/JobId.h"
#include "glite/wmsutils/jobid/JobIdExceptions.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"

#include "glite/lb/ServerConnection.h"
#include "glite/lb/LoggingExceptions.h"

EWL_BEGIN_NAMESPACE;

/**
 * definitions of QueryRecord class
 */
#define CLASS_PREFIX "glite::lb::QueryRecord::"


QueryRecord::QueryRecord(const Attr a, 
			 const Op o, 
			 const std::string & v)
	: attr(a), oper(o),  state(EDG_WLL_JOB_UNDEF), string_value(v)
{
	switch(a) {
	case OWNER:
	case LOCATION:
	case DESTINATION:
	case HOST:
	case INSTANCE:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "invalid value for attribute: " + v);
	}
}


QueryRecord::QueryRecord(const Attr a, 
			 const Op o, 
			 const int v)
  : attr(a), oper(o), state(EDG_WLL_JOB_UNDEF), int_value(v)
{
	switch(a) {
	case DONECODE:
	case STATUS:
	case SOURCE:
	case EVENT_TYPE:
	case LEVEL:
	case EXITCODE:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of integer type");
	}
}


QueryRecord::QueryRecord(const Attr a, 
			 const Op o, 
			 const struct timeval& v)
  : attr(a), oper(o), state(EDG_WLL_JOB_UNDEF), timeval_value(v)
{
	switch(a) {
	case TIME:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of timeval type");
	}
}


QueryRecord::QueryRecord(const Attr a, 
			 const Op o, 
			 const glite::wmsutils::jobid::JobId& v)
	: attr(a), oper(o), state(EDG_WLL_JOB_UNDEF), jobid_value(v)
{
	switch(a) {
	case JOBID:
	case PARENT:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of JobId type");
	}
}


QueryRecord::QueryRecord(const Attr a,
			 const Op o,
			 const int s,
			 const struct timeval &v)
	: attr(a), oper(o), state(s), timeval_value(v)
{
	switch(a) {
	case TIME:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of timeval type");
	}
}


QueryRecord::QueryRecord(const Attr a,
			 const Op o,
			 const std::string &v1,
			 const std::string &v2)
	: attr(a), oper(o), state(EDG_WLL_JOB_UNDEF), string_value(v1), string_value2(v2)
{
	switch(a) {
	case OWNER:
	case LOCATION:
	case DESTINATION:
	case HOST:
	case INSTANCE:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "invalid value for attribute type");
	}
	if(o != WITHIN) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "only operator WITHIN allowed with two values");
	}
}


QueryRecord::QueryRecord(const Attr a,
			 const Op o,
			 const int v1,
			 const int v2)
	: attr(a), oper(o), state(EDG_WLL_JOB_UNDEF), int_value(v1), int_value2(v2)
{
	switch(a) {
	case DONECODE:
	case STATUS:
	case SOURCE:
	case EVENT_TYPE:
	case LEVEL:
	case EXITCODE:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of integer type");
	}
	if(o != WITHIN) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "only operator WITHIN allowed with two values");
	}
}


QueryRecord::QueryRecord(const Attr a,
			 const Op o,
			 const struct timeval &v1,
			 const struct timeval &v2)
	: attr(a), oper(o), state(EDG_WLL_JOB_UNDEF), timeval_value(v1), timeval_value2(v2)
{
	switch(a) {
	case TIME:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of timeval type");
	}
	if(o != WITHIN) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "only operator WITHIN allowed with two values");
	}
}


QueryRecord::QueryRecord(const Attr a,
			 const Op o,
			 const int s,
			 const struct timeval &v1,
			 const struct timeval &v2)
	: attr(a), oper(o), state(s), timeval_value(v1), timeval_value2(v2)
{
	switch(a) {
	case TIME:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "attribute is not of timeval type");
	}
	if(o != WITHIN) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "only operator WITHIN allowed with two values");
	}
}


QueryRecord::QueryRecord(const std::string &tag,
			 const Op o,
			 const std::string &val)
	: attr(USERTAG), oper(o), tag_name(tag), state(EDG_WLL_JOB_UNDEF), string_value(val)
{
}


QueryRecord::QueryRecord(const std::string &tag,
			 const Op o,
			 const std::string &v1,
			 const std::string &v2)
	: attr(USERTAG), oper(o), tag_name(tag), state(EDG_WLL_JOB_UNDEF), 
	  string_value(v1), string_value2(v2)
	  
{
	if(o != WITHIN) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "only operator WITHIN allowed with two values");
	}
}


QueryRecord::QueryRecord(const QueryRecord &src)
{
	attr = src.attr;
	oper = src.oper;

	switch (attr) {

	case USERTAG:
		tag_name = src.tag_name;

	case OWNER:
	case LOCATION:
	case DESTINATION:
	case HOST:
	case INSTANCE:
	        string_value = src.string_value;
		if(src.oper == WITHIN)
			string_value2 = src.string_value2;
		break;

	case DONECODE:
	case STATUS:
	case SOURCE:
	case EVENT_TYPE:
	case LEVEL:
	case EXITCODE:
		int_value = src.int_value;
		if(src.oper == WITHIN)
			int_value2 = src.int_value2;
		break;

	case TIME:
		timeval_value = src.timeval_value;
		if(src.oper == WITHIN)
			timeval_value2 = src.timeval_value2;
		state = src.state;
		break;

	case JOBID:
       	        jobid_value = src.jobid_value;
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "query attribute not defined");
	}
}


QueryRecord::QueryRecord() : attr(UNDEF), oper(EQUAL)
{
}


QueryRecord::~QueryRecord()
{
}


QueryRecord&
QueryRecord::operator=(const QueryRecord &src)
{
        if(this == &src)
          return(*this);

	attr = src.attr;
	oper = src.oper;

	switch (attr) {

	case USERTAG:
		tag_name = src.tag_name;

	case OWNER:
	case LOCATION:
	case DESTINATION:
	case HOST:
	case INSTANCE:
	        string_value = src.string_value;
		if(oper == WITHIN)
			string_value2 = src.string_value2;
		break;

	case DONECODE:
	case STATUS:
	case SOURCE:
	case EVENT_TYPE:
	case LEVEL:
	case EXITCODE:
		int_value = src.int_value;
		if(oper == WITHIN)
			int_value2 = src.int_value2;
		break;

	case TIME:
		timeval_value = src.timeval_value;
		state = src.state;
		if(oper == WITHIN)
			timeval_value2 = src.timeval_value2;
		break;

	case JOBID:
       	        jobid_value = src.jobid_value;
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "query attribute not defined");
	}

	return *this;
}


QueryRecord::operator edg_wll_QueryRec() const
{
	edg_wll_QueryRec out;

	out.attr = edg_wll_QueryAttr(attr);
	out.op   = edg_wll_QueryOp(oper);

	switch (attr) {

	case USERTAG:
		out.attr_id.tag = strdup(tag_name.c_str());

	case OWNER:
	case LOCATION:
	case DESTINATION:
	case HOST:
	case INSTANCE:
		out.value.c = strdup(string_value.c_str());
		if(oper == WITHIN)
			out.value2.c = strdup(string_value2.c_str());
		break;


	case DONECODE:
	case STATUS:
	case SOURCE:
	case EVENT_TYPE:
	case LEVEL:
	case EXITCODE:
		out.value.i = int_value;
		if(oper == WITHIN)
			out.value2.i = int_value2;
		break;

	case TIME:
		out.value.t = timeval_value;
		out.attr_id.state = (edg_wll_JobStatCode)state;
		if(oper == WITHIN)
			out.value2.t = timeval_value2;
		break;

	case JOBID:
		out.value.j = jobid_value.getId();
		break;

	case UNDEF:
		break;

	default:
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "query attribute not defined");
	}

	return(out);
}

const std::string QueryRecord::AttrName(const QueryRecord::Attr attr)
{
	char	*an = edg_wll_query_attrToString(edg_wll_QueryAttr(attr));
	std::string	ret(an);
	free(an);
	return ret;
}


/** 
 * definitions of ServerConnection class 
 */
#undef CLASS_PREFIX
#define CLASS_PREFIX "glite::lb::ServerConnection::"

ServerConnection::ServerConnection()
{
	int ret;
	edg_wll_Context tmp_context;

	if((ret=edg_wll_InitContext(&tmp_context)) < 0) {
		STACK_ADD;
		throw OSException(EXCEPTION_MANDATORY, ret, "initializing context");
	}
  
	context = tmp_context;
}


ServerConnection::~ServerConnection()
{
	/* no exceptions should be thrown from destructors */
	edg_wll_FreeContext(context);
}


/********************/
/* BEGIN DEPRECATED */

ServerConnection::ServerConnection(const std::string &in)
{
	STACK_ADD;
	throw Exception(EXCEPTION_MANDATORY, 0, "method deprecated");
}


void 
ServerConnection::open(const std::string & in) 
{
	STACK_ADD;
	throw Exception(EXCEPTION_MANDATORY, 0, "method deprecated");
}


void 
ServerConnection::close(void) 
{
	STACK_ADD;
	throw Exception(EXCEPTION_MANDATORY, 0, "method deprecated");
}

/* END DEPRECATED */
/******************/


void 
ServerConnection::setQueryServer(const std::string& server, int port)
{
	check_result(edg_wll_SetParamString(context, 
					    EDG_WLL_PARAM_QUERY_SERVER,
					    server.c_str()),
		     context,
		     "setting query server address");
	check_result(edg_wll_SetParamInt(context,
					 EDG_WLL_PARAM_QUERY_SERVER_PORT,
					 port),
		     context,
		     "setting query server port");
}


void 
ServerConnection::setQueryTimeout(int timeout)
{
	check_result(edg_wll_SetParamInt(context,
					 EDG_WLL_PARAM_QUERY_TIMEOUT,
					 timeout),
		     context,
		     "setting query timeout");
}


void ServerConnection::setX509Proxy(const std::string& proxy)
{
	check_result(edg_wll_SetParamString(context,
					    EDG_WLL_PARAM_X509_PROXY,
					    proxy.c_str()),
		     context,
		     "setting X509 proxy");
}


void ServerConnection::setX509Cert(const std::string& cert, const std::string& key)
{
	check_result(edg_wll_SetParamString(context,
					    EDG_WLL_PARAM_X509_CERT,
					    cert.c_str()),
		     context,
		     "setting X509 certificate");
	check_result(edg_wll_SetParamString(context,
					    EDG_WLL_PARAM_X509_KEY,
					    key.c_str()),
		     context,
		     "setting X509 key");
}


void 
ServerConnection::setQueryEventsLimit(int max) {
	check_result(edg_wll_SetParamInt(context,
					 EDG_WLL_PARAM_QUERY_EVENTS_LIMIT,
					 max),
		     context,
		     "setting query events limit");
}

void 
ServerConnection::setQueryJobsLimit(int max) {
	check_result(edg_wll_SetParamInt(context,
					 EDG_WLL_PARAM_QUERY_JOBS_LIMIT,
					 max),
		     context,
		     "setting query jobs limit");
}


std::pair<std::string, int>
ServerConnection::getQueryServer() const
{
	char *hostname;
	int  port;

	check_result(edg_wll_GetParam(context, 
				      EDG_WLL_PARAM_QUERY_SERVER,
				      &hostname),
		     context,
		     "getting query server address");
	check_result(edg_wll_GetParam(context,
				      EDG_WLL_PARAM_QUERY_SERVER_PORT,
				      &port),
		     context,
		     "getting query server port");
	return std::pair<std::string,int>(std::string(strdup(hostname)), port);
}


int
ServerConnection::getQueryTimeout() const
{
	int timeout;

	check_result(edg_wll_GetParam(context,
				      EDG_WLL_PARAM_QUERY_TIMEOUT,
				      &timeout),
		     context,
		     "getting query timeout");
	return timeout;
}


std::string
ServerConnection::getX509Proxy() const
{
	char *proxy;

	check_result(edg_wll_GetParam(context,
				      EDG_WLL_PARAM_X509_PROXY,
				      &proxy),
		     context,
		     "getting X509 proxy");
	return std::string(strdup(proxy));
}


std::pair<std::string, std::string>
ServerConnection::getX509Cert() const
{
	char *cert, *key;

	check_result(edg_wll_GetParam(context,
				      EDG_WLL_PARAM_X509_CERT,
				      &cert),
		     context,
		     "getting X509 cert");
	check_result(edg_wll_GetParam(context,
				      EDG_WLL_PARAM_X509_KEY,
				      &key),
		     context,
		     "getting X509 key");

	return std::pair<std::string, std::string>(std::string(strdup(cert)),
						   std::string(strdup(key)));
}

// static
void freeQueryRecVector(edg_wll_QueryRec *v)
{
	for(; v->attr != EDG_WLL_QUERY_ATTR_UNDEF; v++)
		edg_wll_QueryRecFree(v);
}

std::vector<std::vector<std::pair<QueryRecord::Attr,std::string> > >
ServerConnection::getIndexedAttrs(void) {
	edg_wll_QueryRec	**recs;
	int	i,j;
	std::vector<std::vector<std::pair<QueryRecord::Attr,std::string> > >	out;

	check_result(edg_wll_GetIndexedAttrs(context,&recs),context,
		"edg_wll_GetIndexedAttrs()");
	
	if (!recs) return out;

	for (i=0; recs[i]; i++) {
		std::vector<std::pair<QueryRecord::Attr,std::string> >	idx;
		for (j=0; recs[i][j].attr; j++) {
			char	*s = strdup("");
			if (recs[i][j].attr == EDG_WLL_QUERY_ATTR_USERTAG)
				s = strdup(recs[i][j].attr_id.tag);
			else if (recs[i][j].attr == EDG_WLL_QUERY_ATTR_TIME)
				s = edg_wll_StatToString(recs[i][j].attr_id.state);
			idx.push_back(
				std::pair<QueryRecord::Attr,std::string>(
					QueryRecord::Attr(recs[i][j].attr),s)
			);
			free(s);
		}
		freeQueryRecVector(recs[i]);
		out.push_back(idx);
	}
	free(recs);
	return out;
}




edg_wll_QueryRec *
convertQueryVector(const std::vector<QueryRecord> &in)
{
	unsigned i;
	edg_wll_QueryRec *out = new edg_wll_QueryRec[in.size() + 1];
	QueryRecord empty;

	if(out == NULL) {
		STACK_ADD;
		throw OSException(EXCEPTION_MANDATORY, ENOMEM, "allocating vector for conversion");
	}

	try {
		for(i = 0; i < in.size(); i++) {
			out[i] = in[i];
		}
		out[i] = empty;
	} catch (Exception &e) {
		STACK_ADD;
		throw;
	}
	return(out);
}


edg_wll_QueryRec **
convertQueryVectorExt(const std::vector<std::vector<QueryRecord> > &in)
{
	unsigned i;
	edg_wll_QueryRec **out = new edg_wll_QueryRec*[in.size() + 1];

	if(out == NULL) {
		STACK_ADD;
		throw OSException(EXCEPTION_MANDATORY, ENOMEM, "allocating vector for conversion");
	}

	try {
		for(i = 0; i < in.size(); i++) {
			out[i] = convertQueryVector(in[i]);
		}
		out[i] = NULL;
	} catch (Exception &e) {
		STACK_ADD;
		throw;
	}
	return(out);
}

void 
ServerConnection::queryEvents(const std::vector<QueryRecord>& job_cond,
			      const std::vector<QueryRecord>& event_cond,
			      std::vector<Event> & eventList) const
{
	edg_wll_QueryRec    *job_rec = NULL, *event_rec = NULL;
	edg_wll_Event       *events = NULL;
	unsigned	i;
	int		result, qresults_param;
	char		*errstr = NULL;
	
	/* convert input */
	try {
		job_rec = convertQueryVector(job_cond);
		event_rec = convertQueryVector(event_cond);

		/* do the query */

		result = edg_wll_QueryEvents(context, job_rec, event_rec, &events);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_QueryEvents");
			}
		} else {
			check_result(result, context,"edg_wll_QueryEvents");
		}
		
		/* convert output */
		for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++) {
			edg_wll_Event	*ev = (edg_wll_Event *) malloc(sizeof *ev);
			memcpy(ev,events+i,sizeof *ev);
			Event   e(ev);
      
			eventList.push_back(e);
		}

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryEvents");
		}
    
		free(events);
		delete[] job_rec;
		delete[] event_rec;

	} catch(Exception &e) {
		if(job_rec) delete[] job_rec;
		if(event_rec) delete[] event_rec;
		if(events) free(events);
		if(errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}


const std::vector<Event>  
ServerConnection::queryEvents(const std::vector<QueryRecord>& job_cond,
			      const std::vector<QueryRecord>& event_cond) const
{
	std::vector<Event>      eventList;

	queryEvents(job_cond, event_cond,eventList);
	return eventList;
}

const std::list<Event>
ServerConnection::queryEventsList(const std::vector<QueryRecord>& job_cond,
			const std::vector<QueryRecord>& event_cond) const
{
	std::vector<Event> events;

	queryEvents(job_cond, event_cond, events);
	return std::list<Event>(events.begin(),events.end());
}

std::string 
ServerConnection::queryEventsAggregate(const std::vector<QueryRecord>& job_cond,
				       const std::vector<QueryRecord>& event_cond,
				       enum AggOp const op,
				       std::string const attr) const
{
	STACK_ADD;
	throw Exception(EXCEPTION_MANDATORY, 0, "method not implemented");
	return ""; // gcc warning;
}


void 
ServerConnection::queryEvents(const std::vector<std::vector<QueryRecord> >& job_cond,
			      const std::vector<std::vector<QueryRecord> >& event_cond,
			      std::vector<Event>& eventList) const
{
	edg_wll_QueryRec    **job_rec = NULL, **event_rec = NULL;
	edg_wll_Event       *events = NULL;
	unsigned	i;
	
	/* convert input */
	try {
		job_rec = convertQueryVectorExt(job_cond);
		event_rec = convertQueryVectorExt(event_cond);

		/* do the query */

		check_result(edg_wll_QueryEventsExt(context, 
						    (const edg_wll_QueryRec**)job_rec,
						    (const edg_wll_QueryRec**)event_rec, 
						    &events),
			     context,
			     "edg_wll_QueryEvents");
    
		/* convert output */
		for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++) {
			edg_wll_Event	*ev = (edg_wll_Event *) malloc(sizeof *ev);
			memcpy(ev,events+i,sizeof *ev);
			Event   e(ev);
      
			eventList.push_back(e);
		}
    
		free(events);

		for(i = 0 ; job_rec[i]; i++) delete[] job_rec[i];
		for(i = 0 ; event_rec[i]; i++) delete[] event_rec[i];
		delete[] job_rec;
		delete[] event_rec;

	} catch(Exception &e) {

		if(job_rec) {
			for(i = 0 ; job_rec[i]; i++) delete[] job_rec[i];
			delete[] job_rec;
		}
		if(event_rec) { 
			for(i = 0 ; event_rec[i]; i++) delete[] event_rec[i];
			delete[] event_rec;
		}
		if(events) free(events);

		STACK_ADD;
		throw;
	}
}

  
const std::vector<Event> 
ServerConnection::queryEvents(const std::vector<std::vector<QueryRecord> >& job_cond,
			      const std::vector<std::vector<QueryRecord> >& event_cond) const
{
	std::vector<Event>      eventList;

	queryEvents(job_cond, event_cond,eventList);
	return eventList;
}


void ServerConnection::queryJobs(const std::vector<QueryRecord>& query,
				 std::vector<glite::wmsutils::jobid::JobId> & ids) const
{
	edg_wll_QueryRec *cond = NULL;
	edg_wlc_JobId *jobs, *j;
	int	result, qresults_param;
	char	*errstr = NULL;

	try {
		cond = convertQueryVector(query);
    
		result = edg_wll_QueryJobs(context, cond, 0, &jobs, NULL);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_QueryJobs");
			}
		} else {
			check_result(result, context,"edg_wll_QueryJobs");
		}

		for(j = jobs; *j; j++) 
			ids.push_back(glite::wmsutils::jobid::JobId(*j));

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryJobs");
		}

		free(jobs);
		freeQueryRecVector(cond);
		delete[] cond;

	} catch (Exception &e) {
		if(cond) {
			freeQueryRecVector(cond);
			delete[] cond;
		}
		if (errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}


const std::vector<glite::wmsutils::jobid::JobId>
ServerConnection::queryJobs(const std::vector<QueryRecord>& query) const
{
	std::vector<glite::wmsutils::jobid::JobId> jobList;
  
	queryJobs(query, jobList);
	return jobList;
}


void 
ServerConnection::queryJobs(const std::vector<std::vector<QueryRecord> >& query,
			    std::vector<glite::wmsutils::jobid::JobId>& ids) const
{
	edg_wll_QueryRec **cond = NULL;
	edg_wlc_JobId *jobs, *j;
	int	result, qresults_param;
        char    *errstr = NULL;

	try {
		cond = convertQueryVectorExt(query);
    
		result = edg_wll_QueryJobsExt(context, (const edg_wll_QueryRec**)cond, 
					       0, &jobs, NULL);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_QueryJobsExt");
			}
		} else {
			check_result(result, context,"edg_wll_QueryJobsExt");
		}

		for(j = jobs; *j; j++) 
			ids.push_back(glite::wmsutils::jobid::JobId(*j));

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryJobsExt");
		}

		free(jobs);
		{
			unsigned i;

			for(i = 0; cond[i]; i++) {
				freeQueryRecVector(cond[i]);
				delete[] cond[i];
			}
			delete[] cond;
		}

	} catch (Exception &e) {
		unsigned i;
		if(cond) {
			for(i = 0; cond[i]; i++) {
				freeQueryRecVector(cond[i]);
				delete[] cond[i];
			}
			delete[] cond;
		}
		if (errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}

  
const 
std::vector<glite::wmsutils::jobid::JobId>
ServerConnection::queryJobs(const std::vector<std::vector<QueryRecord> >& query) const
{
	std::vector<glite::wmsutils::jobid::JobId> jobList;
  
	queryJobs(query, jobList);
	return jobList;
}


void
ServerConnection::queryJobStates(const std::vector<QueryRecord>& query,
				 int flags,
				 std::vector<JobStatus> & states) const
{
	edg_wll_QueryRec *cond = NULL;
	edg_wll_JobStat *jobs, *j;
	int     result, qresults_param;
	char    *errstr = NULL;

	try {
		cond = convertQueryVector(query);
    
		result = edg_wll_QueryJobs(context, cond, flags, NULL, &jobs);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_QueryJobs");
			}
		} else {
			check_result(result, context,"edg_wll_QueryJobs");
		}

		for(j = jobs; j->state != EDG_WLL_JOB_UNDEF; j++) {
			edg_wll_JobStat *jsep = new edg_wll_JobStat;
			if (jsep != NULL) {
				memcpy(jsep, j, sizeof(*j));
				states.push_back(JobStatus(*jsep));
			}
		}

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryJobs");
		}

		delete jobs;

		freeQueryRecVector(cond);
		delete[] cond;

	} catch (Exception &e) {
		if(cond) { 
			freeQueryRecVector(cond);
			delete[] cond;
		}
		if (errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}


const std::vector<JobStatus>
ServerConnection::queryJobStates(const std::vector<QueryRecord>& query,
				 int flags) const
{
	std::vector<JobStatus> states;

	queryJobStates(query, flags, states);
	return(states);
}

const std::list<JobStatus>
ServerConnection::queryJobStatesList(const std::vector<QueryRecord>& query,
				 int flags) const
{
	std::vector<JobStatus> states;

	queryJobStates(query, flags, states);
	return std::list<JobStatus>(states.begin(),states.end());
}


void
ServerConnection::queryJobStates(const std::vector<std::vector<QueryRecord> >& query,
				 int flags,
				 std::vector<JobStatus> & states) const
{
	edg_wll_QueryRec **cond = NULL;
	edg_wll_JobStat *jobs, *j;
	int     result, qresults_param;
	char    *errstr = NULL;

	try {
		cond = convertQueryVectorExt(query);
    
		result = edg_wll_QueryJobsExt(context, (const edg_wll_QueryRec**)cond, 
					       flags, NULL, &jobs);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_QueryJobsExt");
			}
		} else {
			check_result(result, context,"edg_wll_QueryJobsExt");
		}

		for(j = jobs; j->state != EDG_WLL_JOB_UNDEF; j++) {
			edg_wll_JobStat *jsep = new edg_wll_JobStat;
			if (jsep != NULL) {
				memcpy(jsep, j, sizeof(*j));
				states.push_back(JobStatus(*jsep));
			}
		}

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryJobsExt");
		}

		delete jobs;

		{
			unsigned i;

			for(i = 0; cond[i]; i++) { 
				freeQueryRecVector(cond[i]);
				delete[] cond[i];
			}
			delete[] cond;
		}


	} catch (Exception &e) {
		unsigned i;
		if(cond) { 
			for(i = 0; cond[i]; i++) {
				freeQueryRecVector(cond[i]);
				delete[] cond[i];
			}
			delete[] cond;
		}
		if (errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}


const std::vector<JobStatus>
ServerConnection::queryJobStates(const std::vector<std::vector<QueryRecord> >& query,
				 int flags) const
{
	std::vector<JobStatus> states;

	queryJobStates(query, flags, states);
	return(states);
}


void ServerConnection::userJobs(std::vector<glite::wmsutils::jobid::JobId> & ids) const
{
	edg_wlc_JobId *jobs, *j;
	int     result, qresults_param;
	char    *errstr = NULL;

	try {
		result = edg_wll_UserJobs(context, &jobs, NULL);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_UserJobs");
			}
		} else {
			check_result(result, context,"edg_wll_UserJobs");
		}

		for(j = jobs; *j; j++) 
			ids.push_back(glite::wmsutils::jobid::JobId(*j));

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryJobsExt");
		}

		free(jobs);

	} catch (Exception &e) {
		if (errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}


const std::vector<glite::wmsutils::jobid::JobId>
ServerConnection::userJobs() const
{
	std::vector<glite::wmsutils::jobid::JobId> jobList;
  
	userJobs(jobList);
	return jobList;
}


void
ServerConnection::userJobStates(std::vector<JobStatus> & states) const
{
	edg_wll_JobStat *jobs, *j;
	int     result, qresults_param;
	char    *errstr = NULL;

	try {
		result = edg_wll_UserJobs(context, NULL, &jobs);
		if (result == E2BIG) {
			edg_wll_Error(context, NULL, &errstr);
			check_result(edg_wll_GetParam(context,
						EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
					context,
					"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
			if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
				edg_wll_SetError(context, result, errstr);
				check_result(result, context,"edg_wll_UserJobs");
			}
		} else {
			check_result(result, context,"edg_wll_UserJobs");
		}

		for(j = jobs; j->state != EDG_WLL_JOB_UNDEF; j++) {
			edg_wll_JobStat *jsep = new edg_wll_JobStat;
			if (jsep != NULL) {
				memcpy(jsep, j, sizeof(*j));
				states.push_back(JobStatus(*jsep));
			}
		}

		if (result) {
			edg_wll_SetError(context, result, errstr);
			check_result(result, context,"edg_wll_QueryJobsExt");
		}

		delete jobs;

	} catch (Exception &e) {
		if (errstr) free(errstr);

		STACK_ADD;
		throw;
	}
}


const std::vector<JobStatus>
ServerConnection::userJobStates() const
{
	std::vector<JobStatus> states;

	userJobStates(states);
	return(states);
}


edg_wll_Context
ServerConnection::getContext(void) const
{
	return(context);
}


void ServerConnection::setParam(edg_wll_ContextParam par, int val)
{
	check_result(edg_wll_SetParamInt(context,par,val),
		context,
		"edg_wll_SetParamInt()");
}

void ServerConnection::setParam(edg_wll_ContextParam par, const std::string &val)
{
	check_result(edg_wll_SetParamString(context,par,val.c_str()),
		context,
		"edg_wll_SetParamString()");
}

void ServerConnection::setParam(edg_wll_ContextParam par, const struct timeval & val)
{
	check_result(edg_wll_SetParamTime(context,par,&val),
		context,
		"edg_wll_SetParamTime()");
}

int ServerConnection::getParamInt(edg_wll_ContextParam par) const
{
	int	ret;
	check_result(edg_wll_GetParam(context,par,&ret),
		context,
		"edg_wll_GetParam()");
	return	ret;
}

std::string ServerConnection::getParamString(edg_wll_ContextParam par) const
{
	char	*ret;
	std::string	out;

	check_result(edg_wll_GetParam(context,par,&ret),
		context,
		 "edg_wll_GetParam()");

	out = ret;
	free(ret);
	return out;
}

struct timeval ServerConnection::getParamTime(edg_wll_ContextParam par) const
{
	struct timeval	ret;
	check_result(edg_wll_GetParam(context,par,&ret),
		context,
		"edg_wll_GetParam()");
	return ret;
}

EWL_END_NAMESPACE;
