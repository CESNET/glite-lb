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
	PluginManager::thePluginManager.initialize();

	// create unix socket with plain IO and HTTP transport
	input = new SocketInput(sock_path, 
				&PlainConnection::theFactory, 
				&HTTPTransport::theFactory);

	// start worker threads
	ThreadPool::theThreadPool.startWorkers(num_threads);

	// run the main loop
	ThreadPool::theThreadPool.run();

	// cleanup & exit
	delete input;
	PluginManager::thePluginManager.cleanup();

	return 0;
}
