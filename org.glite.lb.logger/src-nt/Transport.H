#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "Connection.H"
#include "ThreadPool.H"

class Transport: public ThreadPool::WorkDescription {
public:
	class Factory {
	public:
		virtual Transport *newTransport(Connection *conn) const = 0;

		virtual ~Factory();
	};

	Transport(Connection *a_conn) 
		: ThreadPool::WorkDescription(a_conn ? a_conn->getFD() : -1),
		  conn(a_conn)
		{}

	virtual ~Transport() 
		{ if(conn) delete conn; }

protected:
	Connection *conn;

};
#endif
