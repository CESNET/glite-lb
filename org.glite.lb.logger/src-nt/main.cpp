/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "PluginManager.H"
#include "ThreadPool.H"
#include "SocketInput.H"
#include "PlainConnection.H"
#include "HTTPTransport.H"
#include "EventManager.H"

const int num_threads = 2;
const char *sock_path = "/tmp/il_sock";

EventManager theEventManager();

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
