#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "Connection.H"
#include "Message.H"

// Transport implements transport protocol 
//  - reads/writes messages using Connection interface
//  - 
class Transport {
public:

	// 
	class Factory {
	public:
		virtual Transport *newTransport() const = 0;

		virtual ~Factory() 
			{}
	};

	//
	Transport() 
		{}

	virtual ~Transport();

	// 
	virtual int receive(Connection *conn, Message* &msg) = 0;
	virtual int send(Connection *conn, Message* msg) = 0;
	virtual void reset() = 0;
};
#endif
