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

#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "Connection.H"
#include "Message.H"

// Transport implements transport protocol 
//  - reads/writes messages using Connection interface
//  - 
class Transport {
public:

	// 
	class Factory {
	public:
		virtual Transport *newTransport() const = 0;

		virtual ~Factory() 
			{}
	};

	//
	Transport() 
		{}

	virtual ~Transport();

	// 
	virtual int receive(Connection *conn, Message* &msg) = 0;
	virtual int send(Connection *conn, Message* msg) = 0;
	virtual void reset() = 0;
};
#endif
