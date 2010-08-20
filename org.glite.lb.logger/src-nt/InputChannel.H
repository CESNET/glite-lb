#ifndef _INPUT_CHANNEL_H_
#define _INPUT_CHANNEL_H_

#include "ThreadPool.H"
#include "Connection.H"
#include "Transport.H"

class InputChannel 
	: public ThreadPool::WorkDescription {
public:
	
	InputChannel(Connection *conn, Transport *trans)
		:  ThreadPool::WorkDescription(conn->getFD()),
		   m_connection(conn), m_transport(trans)
		{}

	void start();

protected:
	virtual void onReady();
	virtual void onTimeout();
	virtual void onError();

private:
	Connection *m_connection;
	Transport  *m_transport;
};

#endif
