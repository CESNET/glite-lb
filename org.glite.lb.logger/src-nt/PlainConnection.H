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

#ifndef _PLAIN_CONNECTION_H
#define _PLAIN_CONNECTION_H

#include "Connection.H"
#include "Singleton.H"

class PlainConnection: 
	public Connection
{
public:
	class Factory: public Connection::Factory, 
		       public Singleton<PlainConnection::Factory> {
	public:
 		virtual Connection *newConnection(int fd) const {
			return new PlainConnection(fd);
		}

		virtual Connection *accept(int fd) const;

		virtual ~Factory() {}
	};

	PlainConnection(int a_fd): Connection(a_fd)
		{}

	virtual ~PlainConnection();

	// from Connection
	virtual int read(char *buf, unsigned int len);
	virtual int write(char *buf, unsigned int len);

};

#endif
