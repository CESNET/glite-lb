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

extern "C" {
#include "glite/lbu/trio.h"
#include "glite/lb/interlogd.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/context.h"
#include "glite/lbu/escape.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/xml_parse.h"
}

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
#include <sstream>
#include <cstdio>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>

class OutputPlugin {

public:

	OutputPlugin() : session(NULL), destination(NULL), producer(NULL) {};


	cms::Message *createMessage(edg_wll_JobStat &state_out);
	void         connect(const std::string &topic);
	void         send(cms::Message *msg);
	void         close();
	void         cleanup();

	static void   initialize(const std::string &brokerURI);

	static const char *SCHEME;

private:

	static cms::Connection *getConnection();
	static void releaseConnection();

	cms::Session *session;
	cms::Topic   *destination;
	cms::MessageProducer *producer;
	cms::Connection *current_connection;

	static cms::Connection *connection;
	static cms::ConnectionFactory *connectionFactory;
	static pthread_rwlock_t connection_lock;
};


void
OutputPlugin::connect(const std::string &topic)
{
	try {
		current_connection = getConnection();
		if(session == NULL) {
			session = current_connection->createSession(/* TODO: ackMode */);
			destination = session->createTopic(topic);
			producer = session->createProducer(destination);
		}
		current_connection->start();
		releaseConnection();
	} catch (cms::CMSException &e) {
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "OutputPlugin::connect exception: %s", e.what());
		releaseConnection();
		cleanup();
		throw e;
	}
}


static
void timeval2str(struct timeval *t, char **str) {
        struct tm       *tm;

        tm = gmtime(&t->tv_sec);
        if (asprintf(str,"%4d-%02d-%02dT%02d:%02d:%02dZ",tm->tm_year+1900,tm->tm_mon+1,
	    tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec) == -1) {
		*str = NULL;
	}
}


cms::Message *
OutputPlugin::createMessage(edg_wll_JobStat &state_out)
{
	cms::TextMessage *cms_msg = session->createTextMessage();
	char *s;
	unsigned int i;
	std::ostringstream body;
	bool first = true;

	body << "{";
	/* jobid */
	s = glite_jobid_unparse(state_out.jobId);
	if(s) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"jobid\" : \"" << s << "\"";
		free(s);
	}
	/* ownerDn */
	if(state_out.owner) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"ownerDn\" : \"" << state_out.owner << "\"";
		// cms_msg->setStringProperty("ownerDn", val);
	}
	/* voname */
	s = edg_wll_JDLField(&state_out,"VirtualOrganisation");
	if(s) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"VirtualOrganisation\" : \"" << s << "\"";
		free(s);
	}
	/* bkHost */
	glite_jobid_getServerParts(state_out.jobId, &s, &i);
	if(s) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"bkHost\" : \"" << s << "\"";
		free(s);
	}
	/* networkServer */
	/* TODO: XXX cut out hostname */
	if(state_out.network_server) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"networkHost\" : \"" << state_out.network_server << "\"";
	}
	timeval2str(&state_out.lastUpdateTime, &s);
	if(s) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"lastUpdateTime\" : \"" << s << "\"";
		free(s);
	}
	/* stateName */
	s = edg_wll_StatToString(state_out.state);
	if(s) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"stateName\" : \"" << s << "\"";
		free(s);
	}
	timeval2str(&state_out.stateEnterTime, &s);
	if(s) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"stateStartTime\" : \"" << s << "\"";
		free(s);
	}
	/* condorId */
	if(state_out.condorId) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"condorId\" : \"" << state_out.condorId << "\"";
	}
	/* destSite */
	if(state_out.destination) {
		if (first) { first = false; } else { body << ", "; }
		if (trio_asprintf(&s, "%|Js", state_out.destination) == -1) s = NULL;
		body << "\"destSite\" : \"" << s << "\"";
		free(s);
	}
	/* exitCode */
	if (first) { first = false; } else { body << ", "; }
	body << "\"exitCode\" : " << state_out.exit_code;
	/* doneCode */
	if (first) { first = false; } else { body << ", "; }
	body << "\"doneCode\" : " << state_out.done_code;
	/* statusReason */
	if(state_out.reason) {
		if (first) { first = false; } else { body << ", "; }
		if (trio_asprintf(&s, "%|Js", state_out.reason) == -1) s = NULL;
		body << "\"statusReason\" : \"" << s << "\"";
		free(s);
	}
	/* summaries */
	if(state_out.history) {
		if (first) { first = false; } else { body << ", "; }
		body << "\"history\" : " << state_out.history;
	}
	body << "}";

	cms_msg->setText(body.str().c_str());
	cms_msg->setStringProperty("Content-type", "text/javascript");

	return cms_msg;
}


void
OutputPlugin::send(cms::Message *msg)
{
	try {
		if(producer != NULL) {
			producer->send(msg);
		}
	} catch(cms::CMSException &e) {
		cleanup();
		throw e;
	}
}


void
OutputPlugin::close()
{
	if(producer != NULL) {
		delete producer;
		producer = NULL;
	}
	if(destination != NULL) {
		delete destination;
		destination = NULL;
	}
	if(session != NULL) {
		session->close();
		delete session;
		session = NULL;
	}
}


void
OutputPlugin::cleanup()
{
	try {
		close();
	} catch(cms::CMSException &e) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "activemq_cpp_plugin: close exception: %s", e.what());
	}
	pthread_rwlock_wrlock(&connection_lock);
	if(connection && current_connection == connection) {
		try {
			connection->close();
		} catch(cms::CMSException &e) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "activemq_cpp_plugin: connection close  exception: %s", e.what());
		}
		try {
			delete connection;
		} catch(cms::CMSException &e) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "activemq_cpp_plugin: cleanup exception: %s", e.what());
		}
		connection = NULL;
		current_connection = NULL;
	}
	pthread_rwlock_unlock(&connection_lock);
}


cms::Connection *
OutputPlugin::getConnection() 
{
	pthread_rwlock_rdlock(&connection_lock);
	if(connection) {
		return connection;
	}

	pthread_rwlock_unlock(&connection_lock);

	pthread_rwlock_wrlock(&connection_lock);
	if(!connection) {
		connection = connectionFactory->createConnection();
	}
	pthread_rwlock_unlock(&connection_lock);

	pthread_rwlock_rdlock(&connection_lock);
	return connection;
}


void
OutputPlugin::releaseConnection() 
{
	pthread_rwlock_unlock(&connection_lock);
}


void
OutputPlugin::initialize(const std::string &brokerURI) 
{
	pthread_rwlock_init(&connection_lock, NULL);
	try {
		activemq::library::ActiveMQCPP::initializeLibrary();

		connectionFactory = cms::ConnectionFactory::createCMSConnectionFactory(brokerURI);
	} catch (cms::CMSException &e) {
		try {
			if(connectionFactory != NULL) {
				delete connectionFactory;
				connectionFactory = NULL;
			}
		} catch(cms::CMSException &d) {
		}
		throw e;
	}
}



extern "C"
int
event_queue_connect(struct event_queue *eq, struct queue_thread *me)
{
	OutputPlugin *output;
	std::string topicName(eq->dest_name);

	if(eq->plugin_data == NULL) {
		output = new OutputPlugin();
		eq->plugin_data = (void*)output;
	} else {
		output = (OutputPlugin*)eq->plugin_data;
	}

	assert(output != NULL);

	try {
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
				 "    trying to connect to %s", 
				 eq->dest_name);
		output->connect(topicName);
	} catch(cms::CMSException &e) {
		set_error(IL_DL, 0, e.what());
		me->timeout = TIMEOUT;
		return 0;
	}
	me->first_event_sent = 0;
	eq->last_connected= time(NULL);
	return 1;
}


extern "C"
int
event_queue_send(struct event_queue *eq, struct queue_thread *me)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;
	edg_wll_Context context;
	edg_wll_Event *notif_event;
	edg_wll_JobStat state_out;
	il_octet_string_t event;
	char *jobstat_char;
	int ret;

	assert(output != NULL);

	edg_wll_InitContext(&context);

	while(!event_queue_empty(eq)) {
	    struct server_msg *msg;
	    cms::Message *cms_msg = NULL;

	    if(event_queue_get(eq, me, &msg) == 0) {
		    break;
	    }

	    if(0 == msg->len) {
		    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG,
				     "    not sending empty message at offset %ld for job %s",
				     msg->offset, msg->job_id_s);
		    if(event_store_commit(msg->es, msg->ev_len, 0, msg->generation) < 0) {
			    /* failure committing message, this is bad */
			    goto err;
		    }
		    event_queue_remove(eq, me);
		    continue;
	    }

	    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
			     "    trying to deliver event at offset %ld for job %s",
			     msg->offset, msg->job_id_s);
	    
	    if(decode_il_msg(&event, msg->msg + 17) < 0) {
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
	    if ( edg_wll_ParseJobStat(context, jobstat_char, strlen(jobstat_char), &state_out)) {
		    set_error(IL_LBAPI, EINVAL, "event_queue_send: error parsing job status");
		    fprintf(stderr, "Status string: %s\n", jobstat_char);
		    goto err;
	    }
	    
	    try {
		    cms_msg = output->createMessage(state_out);
		    
		    free(event.data); event.data = NULL;
		    edg_wll_FreeEvent(notif_event);
		    free(notif_event); notif_event = NULL;
		    edg_wll_FreeStatus(&state_out);
		    free(jobstat_char); jobstat_char = NULL;
		    
		    output->send(cms_msg); 
		    delete cms_msg;
		    if(event_store_commit(msg->es, msg->ev_len, 0, msg->generation) < 0) {
			    /* failure committing message, this is bad */
			    goto err;
		    }
		    glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
				     "    event sent to %s", eq->dest_name);
		    
	    } catch(cms::CMSException &e) {
		    if(cms_msg) {
			    delete cms_msg;
		    }
		    me->timeout = TIMEOUT;
		    edg_wll_FreeContext(context);
		    set_error(IL_DL, 0, e.what());
		    return 0;
	    }
	    event_queue_remove(eq, me);
	    me->first_event_sent = 1;
	    eq->last_sent = time(NULL);
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
	edg_wll_FreeStatus(&state_out);

	return -1;
}


extern "C"
int
event_queue_close(struct event_queue *eq, struct queue_thread *me)
{
	OutputPlugin *output = (OutputPlugin*)eq->plugin_data;

	if(output == NULL) {
		// nothing to close
		return 0;
	}

	try { 
		output->close();
	} catch(cms::CMSException &e) {
		set_error(IL_DL, 0, e.what());
		return -1;
	}
	me->first_event_sent = 0;
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
		s = p + 1;
	}
	if(brokerURI.length() == 0) {
		set_error(IL_DL, ENOENT, "plugin_init: broker uri not configured\n");
		return -1;
	}

	try {
		OutputPlugin::initialize(brokerURI);
	} catch(cms::CMSException &e) {
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
const char *OutputPlugin::SCHEME = "x-msg://";
pthread_rwlock_t OutputPlugin::connection_lock;
