#include "PlainConnection.H"
#include "ThreadPool.H"

#include <sys/types.h>
#include <sys/socket.h>

PlainConnection::~PlainConnection()
{
}


Connection *
PlainConnection::Factory::accept(int fd) const
{
	int nfd;

	nfd = ::accept(fd, NULL, NULL);
	return newConnection(nfd);
}


int
PlainConnection::read(char *buf, unsigned int len)
{
	int ret;

	ret = ::recv(fd, buf, len, MSG_NOSIGNAL);
	return ret;
}


int 
PlainConnection::write(char *buf, unsigned int len)
{
	int ret;

	ret = ::write(fd, buf, len);
	return ret;
}
