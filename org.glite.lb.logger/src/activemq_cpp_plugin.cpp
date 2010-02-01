#ident "$Header$"

/*
 * activemq_cpp_plugin.cpp
 *
 *  Created on: Jan 20, 2010
 *      Author: michal
 */

#include "interlogd.h"

#include <cms/ConnectionFactory.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/Topic.h>
#include <cms/MessageProducer.h>

#include <string>

class OutputPlugin {

public:

	OutputPlugin() : session(NULL), destination(NULL), producer(NULL) {};

	virtual void onException(const cms::CMSException &ex);

public:

	cms::Session *session;
	cms::Topic   *destination;
	cms::MessageProducer *producer;

	static cms::Connection *connection = NULL;
	static cms::ConnectionFactory *connectionFactory = NULL;

	static const char *SCHEME = "x-msg:";
};


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
		output->session = OutputPlugin::connection->createSession(/* TODO: ackMode */);
		output->destination = output->session->createTopic(topicName);
		output->producer = output->session->createProducer(output->destination);
		OutputPlugin::connection->start();
	} catch(cms::CMSException &e) {
		try {
			if(output->producer != NULL) {
				delete output->producer;
				output->producer = NULL;
			}
			if(output->destination != NULL) {
				delete output->destination;
				output->destination = NULL;
			}
			if(output->session != NULL) {
				output->session->close();
				delete output->session;
				output->session = NULL;
			}
		} catch(cms::CMSException &e) {
			return -1;
		}
	}

	return 0;
}


extern "C"
int
event_queue_send(struct event_queue *eq)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;

	assert(output != NULL);

	while(!event_queue_empty(eq)) {
	    struct server_msg *msg;
	    std::string val;

	    if(event_queue_get(eq, &msg) < 0)
	      return(-1);

	    try {
	    	cms::TextMessage *cms_msg = output->session->createTextMessage();
	    	cms_msg->setStringProperty('ownerDn', val);
	    	cms_msg->setStringProperty('voname', val);
	    	cms_msg->setStringProperty('bkHost', val);
	    	cms_msg->setStringProperty('networkHost', val);
	    	cms_msg->setStringProperty('lastUpdateTime', val);
	    	cms_msg->setStringProperty('stateName', val);
	    	cms_msg->setStringProperty('stateStartTime', val);
	    	cms_msg->setStringProperty('condorId', val);
	    	cms_msg->setStringProperty('destSite', val);
	    	cms_msg->setStringProperty('exitCode', val);
	    	cms_msg->setStringProperty('doneCode', val);
	    	cms_msg->setStringProperty('statusReason', val);
	    	output->producer->send(cms_msg);
	    	// output->session->commit();
	    	delete cms_msg;
	    	if(event_store_commit(msg->es, msg->ev_len, queue_list_is_log(eq), msg->generation) < 0) {
	    		/* failure committing message, this is bad */
	    		return(-1);
	    	}
	        event_queue_remove(eq);
	        eq->first_event_sent = 1;
	    } catch(cms::CMSException &e) {

	    }

	}
	return 1;
}


extern "C"
int
event_queue_close(struct event_queue *eq)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;

	assert(output != NULL);

	try {
		if(output->session != NULL) {
			output->session->close();
			delete output->session;
			output->session = NULL;
		}
		OutputPlugin::connection->stop();
	} catch(cms::CMSException &e) {
		return -1;
	}

	return 0;
}


extern "C"
int
plugin_init()
{
	std::string brokerURI;

	try {
		activemq::library::ActiveMQCPP::initializeLibrary();

		OutputPlugin::connectionFactory = cms::ConnectionFactory::createCMSConnectionFactory(brokerURI);
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
