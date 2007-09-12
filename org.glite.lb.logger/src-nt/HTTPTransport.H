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
