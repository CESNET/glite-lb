#ident "$Header$"

/**
 * @file Notification.cpp
 * @version $Revision$
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "glite/lb/Notification.h"
#include "glite/lb/JobStatus.h"
#include "glite/lb/LoggingExceptions.h"
#include "glite/lb/ServerConnection.h"

#include "glite/lb/notifid.h"
#include "glite/lb/notification.h"

EWL_BEGIN_NAMESPACE;

#define CLASS_PREFIX "glite::lb::Notification::"

/* external prototypes */
extern edg_wll_QueryRec **
convertQueryVectorExt(const std::vector<std::vector<glite::lb::QueryRecord> > &);

extern void
freeQueryRecVector(edg_wll_QueryRec *);

/* Constructors */
Notification::Notification(void) 
{
   try {
	int ret = edg_wll_InitContext(&ctx);
	check_result(ret,ctx,"edg_wll_InitContext");
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

Notification::Notification(const std::string notifid_str)
{
   try {
	int ret = edg_wll_InitContext(&ctx);
	check_result(ret,ctx,"edg_wll_InitContext");
	ret = edg_wll_NotifIdParse(notifid_str.c_str(),&notifId);
	check_result(ret,ctx,"edg_wll_NotifIdParse");
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

Notification::Notification(const std::string host,const u_int16_t port)
{
   try {
	int ret = edg_wll_InitContext(&ctx);
	check_result(ret,ctx,"edg_wll_InitContext");
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER, host.c_str());
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER_PORT, port);
	ret = edg_wll_NotifIdCreate(host.c_str(),port,&notifId);
	check_result(ret,ctx,"edg_wll_NotifIdCreate");
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

/* Destructor */
Notification::~Notification(void) 
{ 
   try {
	edg_wll_FreeContext(ctx);
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

/* Methods */
std::string
Notification::getNotifId(void) const
{
   try {
	std::string notifid_str = edg_wll_NotifIdUnparse(notifId);
	return(notifid_str);
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

time_t
Notification::getValid(void) const
{
   return(valid);
}

int
Notification::getFd(void) const
{
   try {
	int ret = edg_wll_NotifGetFd(ctx);
	check_result(ret,ctx,"edg_wll_NotifGetFd");
	return(ret);
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

void
Notification::addJob(const glite::wmsutils::jobid::JobId &jobId)
{
   std::vector<glite::wmsutils::jobid::JobId>::iterator it;

   try {
	for( it = jobs.begin(); it != jobs.end(); it++ ) {
		if ( (*it).toString() == jobId.toString() ) {
			STACK_ADD;
			throw Exception(EXCEPTION_MANDATORY, EINVAL, "job already exists");
		}
	}
	jobs.push_back(jobId);

   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

void
Notification::removeJob(const glite::wmsutils::jobid::JobId &jobId)
{
   std::vector<glite::wmsutils::jobid::JobId>::iterator it;
   int removed = 0;

   try {
	for( it = jobs.begin(); it != jobs.end(); it++ ) {
		if ( (*it).toString() == jobId.toString() ) {
			jobs.erase(it);
			removed += 1;
//			break;
		}
	}
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }

   if (removed == 0) {
	STACK_ADD;
	throw Exception(EXCEPTION_MANDATORY, EINVAL, "no job to remove");
   }
}

std::string 
Notification::getJobs(void)
{
   std::vector<glite::wmsutils::jobid::JobId>::iterator it;
   std::string ret="";

   try {
	for( it = jobs.begin(); it != jobs.end(); it++ ) {
		ret += (*it).toString();
		ret += "\n";
	}
	return ret;

   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

void
Notification::setStates(const std::vector<glite::lb::JobStatus::Code> &jobStates)
{
	states = jobStates;	
}

std::string 
Notification::getStates(void)
{
   std::vector<glite::lb::JobStatus::Code>::iterator it;
   JobStatus js;
   std::string ret="";

   try {
	for( it = states.begin(); it != states.end(); it++ ) {
		js.status = (*it);
		ret += js.name();
		ret += "\n";
	}
	return ret;

   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

void
Notification::Register(void)
{
   int ret = 0;
   std::vector<glite::wmsutils::jobid::JobId>::iterator it;
   std::vector<glite::lb::JobStatus::Code>::iterator its;
   std::vector<std::vector<glite::lb::QueryRecord> > query;
   edg_wll_QueryRec **conditions = NULL;
   unsigned i;

   try {
	/* fill in the query: */
	for( it = jobs.begin(); it != jobs.end(); it++ ) {
		std::vector<glite::lb::QueryRecord> queryjob;

		QueryRecord r0(QueryRecord::JOBID,QueryRecord::EQUAL,*it);
		queryjob.push_back(r0);
		
		for( its = states.begin(); its != states.end(); its++ ) {
			QueryRecord r(QueryRecord::STATUS,QueryRecord::EQUAL,*its);
			queryjob.push_back(r);
		}

		query.push_back(queryjob);
	}
	/* convert query to conditions */
	conditions = convertQueryVectorExt(query);
	/* register */
	ret = edg_wll_NotifNew(ctx,conditions,-1,NULL,&notifId,&valid);
	check_result(ret,ctx,"edg_wll_NotifNew");
	/* clean */
	if (conditions) {
		for( i = 0; conditions[i]; i++ ) {
// FIXME: not working :o(
//			freeQueryRecVector(conditions[i]);
			delete[] conditions[i];
		}
		delete[] conditions;
	}
   } catch (Exception &e) {
	/* clean */
	if (conditions) {
		for( i = 0; conditions[i]; i++ ) {
// FIXME: not working :o(
//			freeQueryRecVector(conditions[i]);
			delete[] conditions[i];
		}
		delete[] conditions;
	}
	STACK_ADD;
	throw;
   }
}

int Notification::receive(glite::lb::JobStatus &jobStatus,timeval &timeout)
{
	int ret = 0;
	edg_wll_JobStat *status = (edg_wll_JobStat *) calloc(1,sizeof(edg_wll_JobStat));
	if (status == NULL) {
			STACK_ADD;
			throw OSException(EXCEPTION_MANDATORY, ENOMEM, "allocating jobStatus");
	}
	ret = edg_wll_NotifReceive(ctx,-1,&timeout,status,&notifId);
	check_result(ret,ctx,"edg_wll_NotifReceive");
	jobStatus = JobStatus(*status);
	return(ret);
}

EWL_END_NAMESPACE;
