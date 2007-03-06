#ifndef _CONNECTION_H
#define _CONNECTION_H


class Connection {
public:
	class Factory {
	public:
		virtual Connection *newConnection(int fd) const = 0;
		virtual Connection *accept(int fd) const = 0;
	};

	class Endpoint {
	};

	Connection(int a_fd) : fd(a_fd)
		{}

	virtual ~Connection() 
		{}

	virtual int getFD() const 
		{ return fd; }

	virtual int read(char *buf, unsigned int len) = 0;
	virtual int write(char *buf, unsigned int len) = 0;

protected:
	int fd;
};

#endif
