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
