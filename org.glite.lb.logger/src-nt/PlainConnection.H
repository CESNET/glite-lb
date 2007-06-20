#ifndef _PLAIN_CONNECTION_H
#define _PLAIN_CONNECTION_H

#include "Connection.H"


class PlainConnection: 
	public Connection
{
public:
	class Factory: public Connection::Factory {
	public:
 		virtual Connection *newConnection(int fd) const {
			return new PlainConnection(fd);
		}

		virtual Connection *accept(int fd) const;

		virtual ~Factory() {}
	};

	static Factory theFactory;

	PlainConnection(int a_fd): Connection(a_fd)
		{}

	virtual ~PlainConnection();

	// from Connection
	virtual int read(char *buf, unsigned int len);
	virtual int write(char *buf, unsigned int len);

};

#endif
