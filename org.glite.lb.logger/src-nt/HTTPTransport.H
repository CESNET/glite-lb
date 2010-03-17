/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

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

#ifndef _HTTP_TRANSPORT_H
#define _HTTP_TRANSPORT_H

#include "ThreadPool.H"
#include "Transport.H"
#include "Singleton.H"

#include <string>

class HTTPTransport: 
	public Transport
{
public:

	// factory class
	class Factory: public Transport::Factory, 
		       public Singleton<HTTPTransport::Factory>  {
	public:
		virtual Transport *newTransport() const {
			return(new HTTPTransport());
		}
	};

	HTTPTransport() 
		: Transport(),
		  state(NONE),
		  request(), headers(), body(NULL), pos(NULL),
		  content_length(0)
		{}

	virtual ~HTTPTransport();

	virtual int receive(Connection *conn, Message* &msg);
	virtual int send(Connection *conn, Message* msg);
	virtual void reset();

private:
	enum { NONE, 
	       IN_REQUEST,
	       IN_HEADERS,
	       IN_BODY } state;
	std::string request;
	std::string headers;
	char *body;
	char buffer[256];
	char *pos;
	unsigned int content_length;

	int parseHeader(const char *s, unsigned int len);
	void serializeHeaders(Message *msg);
};


#endif
