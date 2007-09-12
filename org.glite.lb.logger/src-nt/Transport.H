#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "Connection.H"
#include "Properties.H"

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
	class Message {
	public:

		Message();

		int getContent(char* &data) const;
		int getContentLength() const;
		int getHeader(const char* name, char* &val) const;
		Properties& getProperties();
		char *path() const;

	private:
		
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
