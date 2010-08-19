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

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

#include "ThreadPool.H"
#include "SocketInput.H"
#include "InputChannel.H"
#include "Exception.H"


// create unix domain socket for input
SocketInput::SocketInput(const char *path, 
			 const Connection::Factory *a_cfactory,
			 const Transport::Factory *a_tfactory)
	: ThreadPool::WorkDescription(0),
	  cFactory(a_cfactory),
	  tFactory(a_tfactory)
{
	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, path);
	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) throw new Exception;
	if(connect(fd, (struct sockaddr*)&saddr, sizeof(saddr.sun_path)) < 0) {
		if(errno == ECONNREFUSED) {
			unlink(saddr.sun_path);
		}
	} else {
		// another instance running
		// throw new Exception
	}
	if(bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) 
		throw new Exception;
	if(listen(fd, SOCK_QUEUE_MAX) < 0)
		throw new Exception;
}


// remove the socket
SocketInput::~SocketInput()
{
	if(fd >= 0)
		close(fd);
	unlink(saddr.sun_path);
}


void
SocketInput::onReady()
{
	Connection *conn = cFactory->accept(fd);
	Transport  *trans = tFactory->newTransport();
	InputChannel *channel = new InputChannel(conn, trans);
	channel->start();
}


void
SocketInput::onTimeout()
{
	// nothing special, just sit around
}


void
SocketInput::onError()
{
	// should report an error?
}
