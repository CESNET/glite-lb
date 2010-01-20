#ident "$Header$"

/*
 * activemq_cpp_plugin.cpp
 *
 *  Created on: Jan 20, 2010
 *      Author: michal
 */

#include "interlogd.h"

#include <cms/ConnectionFactory.h>
#include <string>

class OutputPlugin {

public:

	OutputPlugin() : connection(NULL) {};

	virtual void onException(const cms::CMSException &ex);

public:

	cms::Session *session;
	cms::Topic   *destination;
	cms::MessageProducer *producer;

	static cms::Connection *connection;
	static cms::ConnectionFactory *connectionFactory = NULL;

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

	output->session = OutputPlugin::connection->createSession(/* TODO: ackMode */);
	output->destination = output->session->createTopic(topicName);
	output->producer = output->session->createProducer(output->destination);
	OutputPlugin::connection->start();

	return 0;
}


extern "C"
int
event_queue_send(struct event_queue *eq)
{
	return 0;
}


extern "C"
int
event_queue_close(struct event_queue *eq)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;

	assert(output != NULL);

	OutputPlugin::connection->stop();

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
	} catch (cms::CMSException e) {
		return -1;
	}

	return 0;
}


extern "C"
int
plugin_supports_scheme(const char *scheme)
{

}
