#ifndef _HTTP_TRANSPORT_H
#define _HTTP_TRANSPORT_H

#include "ThreadPool.H"
#include "Transport.H"

#include <string>

class HTTPTransport: 
	public Transport
{
public:
	class Factory: public Transport::Factory {
	public:
		virtual Transport *newTransport(Connection *conn) const {
			if(conn) 
				return(new HTTPTransport(conn));
			else 
				return NULL;
		}
	};

	static Factory theFactory;

	HTTPTransport(Connection *conn) 
		: Transport(conn),
		  state(NONE),
		  request(), headers(), body(NULL), pos(NULL)
		{}

	virtual ~HTTPTransport();

	// from ThreadPool::WorkDescription
	virtual void onReady();
	virtual void onTimeout();
	virtual void onError();

protected:
	enum { NONE, 
	       IN_REQUEST,
	       IN_HEADERS,
	       IN_BODY } state;
	string request;
	string headers;
	char *body;
	char buffer[256];
	char *pos;
};


#endif
