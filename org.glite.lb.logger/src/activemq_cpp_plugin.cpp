#ident "$Header$"

/*
 * activemq_cpp_plugin.cpp
 *
 *  Created on: Jan 20, 2010
 *      Author: michal
 */

#include "interlogd.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/context.h"
#include "glite/lbu/escape.h"
#include "glite/lb/jobstat.h"

#include <activemq/library/ActiveMQCPP.h>
#include <cms/ConnectionFactory.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/Topic.h>
#include <cms/MessageProducer.h>
#include <cms/ExceptionListener.h>
#include <cms/TextMessage.h>
#include <cms/Message.h>

#include <string>

class OutputPlugin : public cms::ExceptionListener {

public:

	OutputPlugin() : session(NULL), destination(NULL), producer(NULL) {};

	virtual void onException(const cms::CMSException &ex);

	void connect(const std::string &topic);
	void send(cms::Message &msg);
	void close();
	void cleanup();

public:

	cms::Session *session;
	cms::Topic   *destination;
	cms::MessageProducer *producer;

	static cms::Connection *connection = NULL;
	static cms::ConnectionFactory *connectionFactory = NULL;

	static const char *SCHEME = "x-msg:";
};


void
OutputPlugin::onException(const cms::CMSException &ex)
{
	this->cleanup();
}


void
OutputPlugin::connect(const std::string &topic)
{
	if(this->session == NULL) {
		this->session = self::connection->createSession(/* TODO: ackMode */);
		this->destination = this->session->createTopic(topic);
		this->producer = this->session->createProducer(this->destination);
	}
	self::connection->start();
	self::connection->setExceptionListener(this);
}


void
OutputPlugin::send(cms::Message &msg)
{

}


void
OutputPlugin::close()
{
	this->cleanup();
	self::connection->stop();
}


void
OutputPlugin::cleanup()
{
	if(this->producer != NULL) {
		delete this->producer;
		this->producer = NULL;
	}
	if(this->destination != NULL) {
		delete this->destination;
		this->destination = NULL;
	}
	if(this->session != NULL) {
		this->session->close();
		delete this->session;
		this->session = NULL;
	}
}


extern "C"
int
event_queue_connect(struct event_queue *eq)
{
	OutputPlugin *output;
	std::string topicName;

	if(eq->plugin_data == NULL) {
		output = new OutputPlugin();
		eq->plugin_data = (void*)output;
	} else {
		output = (OutputPlugin*)eq->plugin_data;
	}

	try {
		output->connect(topicName);
	} catch(cms::CMSException &e) {
			output->cleanup();
			eq->timeout = TIMEOUT;
			return 0;
	}
	eq->first_event_sent = 0;
	return 1;
}


extern "C"
int
event_queue_send(struct event_queue *eq)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;
	edg_wll_Context context;
	edg_wll_Event *notif_event;
    edg_wll_JobStat *state_out;
	int ret;

	assert(output != NULL);

	edg_wll_InitContext(&context);

	while(!event_queue_empty(eq)) {
	    struct server_msg *msg;
	    char *s;
	    int i;
	    std::string val;

	    if(event_queue_get(eq, &msg) < 0) {
	    	goto err;
	    }

	    try {
	    	ret=edg_wll_ParseNotifEvent(context, event->data, &notif_event);
	    	if(ret) {
	    		set_error(IL_LBAPI, ret, "event_queue_send: error parsing notification event");
	    		goto err;
	    	}
	        jobstat_char = glite_lbu_UnescapeXML((const char *) notif_event->notification.jobstat);
	        if (jobstat_char == NULL) {
	            set_error(IL_LBAPI, EINVAL, "event_queue_send: error unescaping job status");
	            goto err;
	        }
	        if ( edg_wll_ParseJobStat(context, jobstat_char, strlen(jobstat_char), state_out)) {
	        	set_error(IL_LBAPI, EINVAL, "event_queue_send: error parsing job status");
	        	goto err;
	        }


	    	cms::TextMessage *cms_msg = output->session->createTextMessage();
	    	/* ownerDn */
	    	val.assign(state_out->owner);
	    	cms_msg->setStringProperty('ownerDn', val);
	    	/* voname */
	    	s = edg_wll_JDLField(state_out,"VirtualOrganisation");
	    	val.assign(s);
	    	free(s);
	    	cms_msg->setStringProperty('voname', val);
	    	/* bkHost */
	    	glite_jobid_getServerParts(state_out->jobId, &s, &i);
	    	val.assign(s);
	    	free(s);
	    	cms_msg->setStringProperty('bkHost', val);
	    	/* networkServer */
	    	/* TODO: XXX cut out hostname */
	    	val.assign(state_out->network_server);
	    	cms_msg->setStringProperty('networkHost', val);

	    	cms_msg->setStringProperty('lastUpdateTime', val);
	    	/* stateName */
	    	s = edg_wll_StatToString(state_out->state);
	    	val.assign(s);
	    	free(s);
	    	cms_msg->setStringProperty('stateName', val);

	    	cms_msg->setStringProperty('stateStartTime', val);
	    	/* condorId */
	    	val.assign(state_out->condorId);
	    	cms_msg->setStringProperty('condorId', val);
	    	/* destSite */
	    	val.assign(state_out->destination);
	    	cms_msg->setStringProperty('destSite', val);
	    	/* exitCode */
	    	cms_msg->setIntProperty('exitCode', state_out->exitCode);
	    	/* doneCode */
	    	cms_msg->setIntProperty('doneCode', state_out->doneCode);
	    	/* statusReason */
	    	val.assign(state_out->reason);
	    	cms_msg->setStringProperty('statusReason', val);

	    	edg_wll_FreeEvent(notif_event);
	    	free(notif_event);
	    	edg_wll_FreeStatus(state_out);
	    	free(state_out);
	    	free(jobstat_char);
	    } catch(cms::CMSException &e) {
	    	goto err;
	    }

	    try {
	    	output->send(cms_msg);
		    delete cms_msg;
	    	if(event_store_commit(msg->es, msg->ev_len, queue_list_is_log(eq), msg->generation) < 0) {
	    		/* failure committing message, this is bad */
	    		goto err;
	    	}
	        event_queue_remove(eq);
	        eq->first_event_sent = 1;
	    } catch(cms::CMSException &e) {
		    delete cms_msg;
	    	output->cleanup();
	    	eq->timeout = TIMEOUT;
	    	edg_wll_FreeContext(context);
	    	return 0;
	    }
	}
	edg_wll_FreeContext(context);
	return 1;

err:
	if(notif_event) {
    	edg_wll_FreeEvent(notif_event);
    	free(notif_event);
	}
	if(jobstat_char) {
		free(jobstat_char);
	}
	if(state_out) {
		edg_wll_FreeStatus(state_out);
		free(state_out);
	}
	return -1;
}


extern "C"
int
event_queue_close(struct event_queue *eq)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;

	assert(output != NULL);

	try {
		output->close();
	} catch(cms::CMSException &e) {
		return -1;
	}
	eq->first_event_sent = 0;
	return 0;
}


extern "C"
int
plugin_init()
{
	std::string brokerURI;

	try {
		activemq::library::ActiveMQCPP::initializeLibrary();

		OutputPlugin::connectionFactory =
				cms::ConnectionFactory::createCMSConnectionFactory(brokerURI);
		OutputPlugin::connection = output->connectionFactory->createConnection();
	} catch (cms::CMSException &e) {
		try {
			if(OutputPlugin::connection != NULL) {
				delete OutputPlugin::connection;
				OutputPlugin::connection = NULL;
			}
			if(OutputPlugin::connectionFactory != NULL) {
				delete OutputPlugin::connectionFactory;
				OutputPlugin::connectionFactory = NULL;
			}
		} catch(cms::CMSException &e) {

		}
		return -1;
	}

	return 0;
}


extern "C"
int
plugin_supports_scheme(const char *scheme)
{
	return strncmp(scheme, OutputPlugin::SCHEME, strlen(OutputPlugin::SCHEME)) == 0;
}
