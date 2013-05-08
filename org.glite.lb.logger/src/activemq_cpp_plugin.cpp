#ident "$Header$"
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
#include "glite/lb/xml_parse.h"

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

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>

class OutputPlugin : public cms::ExceptionListener {

public:

	OutputPlugin() : session(NULL), destination(NULL), producer(NULL) {};

	virtual void onException(const cms::CMSException &ex);

	void connect(const std::string &topic);
	void send(cms::Message *msg);
	void close();
	void cleanup();

public:

	cms::Session *session;
	cms::Topic   *destination;
	cms::MessageProducer *producer;

	static cms::Connection *connection;
	static cms::ConnectionFactory *connectionFactory;

	static const char *SCHEME;
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
		this->session = connection->createSession(/* TODO: ackMode */);
		this->destination = this->session->createTopic(topic);
		this->producer = this->session->createProducer(this->destination);
	}
	connection->start();
	connection->setExceptionListener(this);
}


void
OutputPlugin::send(cms::Message *msg)
{
	if(this->producer != NULL) {
		this->producer->send(this->destination, msg);
	}
}


void
OutputPlugin::close()
{
	this->cleanup();
	connection->stop();
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


static
void timeval2str(struct timeval *t, char **str) {
        struct tm       *tm;

        tm = gmtime(&t->tv_sec);
        asprintf(str,"%4d-%02d-%02dT%02d:%02d:%02dZ",tm->tm_year+1900,tm->tm_mon+1,
                tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
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
    il_octet_string_t event;
    char *jobstat_char;
	int ret;

	assert(output != NULL);

	edg_wll_InitContext(&context);

	while(!event_queue_empty(eq)) {
	    struct server_msg *msg;
	    cms::TextMessage *cms_msg;
	    char *s;
	    unsigned int i;
	    std::string val;

	    if(event_queue_get(eq, &msg) < 0) {
	    	goto err;
	    }

	    try {
	    	if(decode_il_msg(&event, msg->msg) < 0) {
	    		set_error(IL_LBAPI, EINVAL, "event_queue_send: error parsing notification event data");
	    		goto err;
	    	}
	    	ret=edg_wll_ParseNotifEvent(context, event.data, &notif_event);
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


	    	cms_msg = output->session->createTextMessage();
	    	/* ownerDn */
	    	val.assign(state_out->owner);
	    	cms_msg->setStringProperty("ownerDn", val);
	    	/* voname */
	    	s = edg_wll_JDLField(state_out,"VirtualOrganisation");
	    	val.assign(s);
	    	free(s);
	    	cms_msg->setStringProperty("voname", val);
	    	/* bkHost */
	    	glite_jobid_getServerParts(state_out->jobId, &s, &i);
	    	val.assign(s);
	    	free(s);
	    	cms_msg->setStringProperty("bkHost", val);
	    	/* networkServer */
	    	/* TODO: XXX cut out hostname */
	    	val.assign(state_out->network_server);
	    	cms_msg->setStringProperty("networkHost", val);
	    	timeval2str(&state_out->lastUpdateTime, &s);
	    	val.assign(s);
	    	if(s) free(s);
	    	cms_msg->setStringProperty("lastUpdateTime", val);
	    	/* stateName */
	    	s = edg_wll_StatToString(state_out->state);
	    	val.assign(s);
	    	if(s) free(s);
	    	cms_msg->setStringProperty("stateName", val);
	    	timeval2str(&state_out->stateEnterTime, &s);
	    	val.assign(s);
	    	if(s) free(s);
	    	cms_msg->setStringProperty("stateStartTime", val);
	    	/* condorId */
	    	val.assign(state_out->condorId);
	    	cms_msg->setStringProperty("condorId", val);
	    	/* destSite */
	    	val.assign(state_out->destination);
	    	cms_msg->setStringProperty("destSite", val);
	    	/* exitCode */
	    	cms_msg->setIntProperty("exitCode", state_out->exit_code);
	    	/* doneCode */
	    	cms_msg->setIntProperty("doneCode", state_out->done_code);
	    	/* statusReason */
	    	val.assign(state_out->reason);
	    	cms_msg->setStringProperty("statusReason", val);

	    	free(event.data);
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
	if(event.data) {
		free(event.data);
	}
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
plugin_init(char *config)
{
	char *s, *p;
	char key[MAXPATHLEN], val[MAXPATHLEN];
	int ret;
	std::string brokerURI;

	s = strstr(config, "[msg]");
	if(s == NULL) {
		set_error(IL_DL, ENOENT, "plugin_init: missing required configuration section [msg]\n");
		return -1;
	}
	s = strchr(s, '\n');
	if(s) s++;
	while(s) {
		if(*s == 0 || *s == '[')
			break;
		p = strchr(s, '\n');
		if(p) *p = 0;
		ret = sscanf(s, " %s =%s", key, val);
		if(p) *p = '\n';
		if(ret == 2) {
			if(strcmp(key, "broker") == 0) {
				brokerURI.assign(val);
			}
		}
		s = p;
	}
	if(brokerURI.length() == 0) {
		set_error(IL_DL, ENOENT, "plugin_init: broker uri not configured\n");
		return -1;
	}

	try {
		activemq::library::ActiveMQCPP::initializeLibrary();

		OutputPlugin::connectionFactory =
				cms::ConnectionFactory::createCMSConnectionFactory(brokerURI);
		OutputPlugin::connection = OutputPlugin::connectionFactory->createConnection();
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
		set_error(IL_DL, 0, e.what());
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


cms::Connection *OutputPlugin::connection = NULL;
cms::ConnectionFactory *OutputPlugin::connectionFactory = NULL;
const char *OutputPlugin::SCHEME = "x-msg:";
