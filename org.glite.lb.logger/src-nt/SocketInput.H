#ifndef _SOCKET_INPUT_H
#define _SOCKET_INPUT_H

#include "ThreadPool.H"
#include "Connection.H"
#include "Transport.H"

#include "sys/un.h"


class SocketInput: public ThreadPool::WorkDescription
{
public:
	SocketInput(const char *path, 
		    const Connection::Factory *a_cfactory,
		    const Transport::Factory *a_tfactory);
	virtual ~SocketInput();

	// from WorkDescription
	virtual void onReady();
	virtual void onTimeout();
	virtual void onError();

private:
	static const int SOCK_QUEUE_MAX = 5;

	struct sockaddr_un saddr;
	const Connection::Factory *cFactory;
	const Transport::Factory *tFactory;
};

#endif
