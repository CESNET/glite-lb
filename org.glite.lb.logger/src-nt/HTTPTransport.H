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
	class Factory: public Transport::Factory, 
		       public Singleton<HTTPTransport::Factory>  {
	public:
		virtual Transport *newTransport(Connection *conn) const {
			if(conn) 
				return(new HTTPTransport(conn));
			else 
				return NULL;
		}
	};

	HTTPTransport(Connection *conn) 
		: Transport(conn),
		  state(NONE),
		  request(), headers(), body(NULL), pos(NULL),
		  content_length(0)
		{}

	virtual ~HTTPTransport();


protected:
	// from ThreadPool::WorkDescription
	virtual void onReady();
	virtual void onTimeout();
	virtual void onError();

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
};


#endif
