/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

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
