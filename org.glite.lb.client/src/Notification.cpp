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

#include "Notification.h"
#include "JobStatus.h"
#include "glite/lb/LoggingExceptions.h"
#include "ServerConnection.h"

#include "glite/lb/notifid.h"
#include "notification.h"

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
	int ret = edg_wll_InitContext(&this->ctx);
	check_result(ret,this->ctx,"edg_wll_InitContext");
	this->notifId = NULL;
	this->valid = 0;
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

Notification::Notification(const std::string notifid_str)
{
   try {
	char *host;
	unsigned int port;
	int ret = edg_wll_InitContext(&this->ctx);
	check_result(ret,this->ctx,"edg_wll_InitContext");
	ret = edg_wll_NotifIdParse(notifid_str.c_str(),&this->notifId);
	check_result(ret,this->ctx,"edg_wll_NotifIdParse");
	edg_wll_NotifIdGetServerParts(this->notifId,&host,&port);
	edg_wll_SetParam(this->ctx, EDG_WLL_PARAM_NOTIF_SERVER, host);
	edg_wll_SetParam(this->ctx, EDG_WLL_PARAM_NOTIF_SERVER_PORT, port);
	free(host);
	this->valid = 0;
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
	this->notifId = NULL;
	this->valid = 0;
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
	edg_wll_FreeContext(this->ctx);
	edg_wll_NotifIdFree(this->notifId);
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
	std::string notifid_str;
	if (this->notifId != NULL) {
		notifid_str = edg_wll_NotifIdUnparse(this->notifId);
		return(notifid_str);
	} else {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "notifId not known at the moment");
	}
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

time_t
Notification::getValid(void) const
{
   return(this->valid);
}

int
Notification::getFd(void) const
{
   try {
	int ret = edg_wll_NotifGetFd(this->ctx);
	check_result(ret,this->ctx,"edg_wll_NotifGetFd");
	return(ret);
   } catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

void
Notification::addJob(const glite::jobid::JobId &jobId)
{
   std::vector<glite::jobid::JobId>::iterator it;

   try {
	if (this->notifId != NULL) {
	STACK_ADD;
	throw Exception(EXCEPTION_MANDATORY, EINVAL, "adding jobs allowed only before registering");
	}
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
Notification::removeJob(const glite::jobid::JobId &jobId)
{
   std::vector<glite::jobid::JobId>::iterator it;
   int removed = 0;

   try {
	if (this->notifId != NULL) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "removing jobs allowed only before registering");
	}
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

/* XXX: obsolete, used only for debugging purposes */

std::string 
Notification::getJobs(void)
{
   std::vector<glite::jobid::JobId>::iterator it;
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
	if (this->notifId != NULL) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "removing jobs allowed only before registering");
	}
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
   std::vector<glite::jobid::JobId>::iterator it;
   std::vector<glite::lb::JobStatus::Code>::iterator its;
   std::vector<std::vector<glite::lb::QueryRecord> > queryExt;
   edg_wll_QueryRec **conditions = NULL;
   unsigned i;

   try {
	if (this->notifId != NULL) {
		STACK_ADD;
		throw Exception(EXCEPTION_MANDATORY, EINVAL, "registering job allowed only once");
	}
	/* fill in the query: */
	std::vector<glite::lb::QueryRecord> query;
	for( it = jobs.begin(); it != jobs.end(); it++ ) {
		QueryRecord r0(QueryRecord::JOBID,QueryRecord::EQUAL,*it);
		query.push_back(r0);
	}
	queryExt.push_back(query);
	query.clear();

	for( its = states.begin(); its != states.end(); its++ ) {
		QueryRecord r(QueryRecord::STATUS,QueryRecord::EQUAL,*its);
		query.push_back(r);
	}
	queryExt.push_back(query);

	/* convert query to conditions */
	conditions = convertQueryVectorExt(queryExt);
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

void
Notification::Bind(const std::string address_override)
{
	try {
		if (this->notifId == NULL) {
			STACK_ADD;
			throw Exception(EXCEPTION_MANDATORY, EINVAL, "binding allowed only for given notifId");
		}
		int ret = edg_wll_NotifBind(this->ctx,this->notifId,-1,address_override.c_str(),&this->valid);
		check_result(ret,this->ctx,"edg_wll_NotifBind");
	}
	catch (Exception &e) {
		STACK_ADD;
		throw;
	}
}


		

int Notification::receive(glite::lb::JobStatus &jobStatus,timeval &timeout)
{
   try {
	int ret = 0;
	edg_wll_JobStat *status = (edg_wll_JobStat *) calloc(1,sizeof(edg_wll_JobStat));
	if (status == NULL) {
			STACK_ADD;
			throw OSException(EXCEPTION_MANDATORY, ENOMEM, "allocating jobStatus");
	}
	ret = edg_wll_NotifReceive(ctx,-1,&timeout,status,&notifId);
	if ( ret == ETIMEDOUT )
		return 1;
	check_result(ret,ctx,"edg_wll_NotifReceive");
	jobStatus = JobStatus(*status);
	return 0;
   }
   catch (Exception &e) {
	STACK_ADD;
	throw;
   }
}

EWL_END_NAMESPACE;
