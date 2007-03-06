#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "Connection.H"


class Transport: public ThreadPool::WorkDescription {
public:
	class Factory {
	public:
		virtual Transport *newTransport(Connection *conn) const = 0;
	};

	Transport(Connection *a_conn) 
		: conn(a_conn),
		  ThreadPool::WorkDescription(a_conn ? a_conn->getFD() : -1)
		{}

	virtual ~Transport() 
		{ if(conn) delete conn; }

protected:
	Connection *conn;

};
#endif
