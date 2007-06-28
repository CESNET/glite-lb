#include "PluginManager.H"
#include "ThreadPool.H"
#include "SocketInput.H"
#include "PlainConnection.H"
#include "HTTPTransport.H"

const int num_threads = 2;
const char *sock_path = "/tmp/il_sock";

int main(int argc, char *argv[])
{
	SocketInput *input;
	
	// initialize plugins
	PluginManager::instance()->initialize();

	// create unix socket with plain IO and HTTP transport
	input = new SocketInput(sock_path, 
				PlainConnection::Factory::instance(), 
				HTTPTransport::Factory::instance());
	// and add the socket to pool
	ThreadPool::instance()->setWorkAccept(input);

	// start worker threads
	ThreadPool::instance()->startWorkers(num_threads);

	// run the main loop
	ThreadPool::instance()->run();

	// cleanup & exit
	delete input;
	PluginManager::instance()->cleanup();

	return 0;
}
