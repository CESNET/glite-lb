#include "HTTPTransport.H"

#include <iostream>


HTTPTransport::Factory HTTPTransport::theFactory;


HTTPTransport::~HTTPTransport()
{
}


void
HTTPTransport::onReady()
{
	char buffer[256];
	int len;

	len = conn->read(buffer, sizeof(buffer));
	if(len < 0) {
		std::cout << "error on receive - closing connection" << std::endl;
	} else if ( len > 0) {
		std::cout.write(buffer, len);
		std::cout.flush();
		ThreadPool::theThreadPool.queueWorkRead(this);
	} else {
		std::cout << "no more data" << std::endl;
	}
}


void 
HTTPTransport::onTimeout()
{
}


void 
HTTPTransport::onError()
{
}
