#ifndef _HTTP_TRANSPORT_H
#define _HTTP_TRANSPORT_H

#include "ThreadPool.H"
#include "Transport.H"

class HTTPTransport: 
	public Transport
{
public:
	class Factory: public Transport::Factory {
	public:
		virtual Transport *newTransport(Connection *conn) const {
			return(new HTTPTransport(conn));
		}
	};

	static Factory theFactory;

	HTTPTransport(Connection *conn) 
		: Transport(conn)
		{}

	virtual ~HTTPTransport();

	// from ThreadPool::WorkDescription
	virtual void onReady();
	virtual void onTimeout();
	virtual void onError();

};


#endif
