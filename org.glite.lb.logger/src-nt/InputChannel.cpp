#include "InputChannel.H"
#include "ThreadPool.H"
#include "EventManager.H"

extern EventManager theEventManager;

void
InputChannel::start()
{
	ThreadPool::instance()->queueWorkRead(this);
}

void
InputChannel::onReady()
{
	Transport::Message *msg = NULL;
	int ret = m_transport->receive(m_connection, msg);
	if(ret <= 0) {
		// no new data read
	} else if(msg) {
		// we have a new message
		
	} else {
		// still need more data
		ThreadPool::instance()->queueWorkRead(this);
	}
}

void
InputChannel::onTimeout()
{
}

void
InputChannel::onError()
{
}
