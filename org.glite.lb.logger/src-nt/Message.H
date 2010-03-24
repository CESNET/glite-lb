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

#ifndef _MESSAGE_H_
#define _MESSAGE_H

#include "Properties.H"
#include "MessageStore.H"

#include <string>

class Message: public MessageStore::Storable {
public:
	
	/** class that holds message state
	 *
	 */
	class State : public MessageStore::Storable {
	public:

		/** Get size needed for storage (from Storable).
		 */
		virtual int getStorageSize() const;

		/** Save State (from Storable)
		 */
		virtual int save(void* data, int len) const;

		/** Load State (from Storable)
		 */
		virtual int load(void* data, int len);
	};


	Message();

	Message(void * data, unsigned int length) 
		: m_length(length), 
		  m_data(data)
		{}


	int getContent(void* &data) const
		{ data = m_data; return m_length; }

	int getContentLength() const 
		{ return m_length; }

	std::string getProperty(const std::string &name, std::string &val)
		{ return m_properties.getProperty(name); }
	
	void setProperty(const std::string &name, std::string &val) 
		{ m_properties.setProperty(name, val); }
	
	Properties& getProperties() 
		{ return m_properties; }
	
	void setProperties(Properties &)
		{}

private:
	MessageStore::ID        m_id;
	unsigned int            m_length;
	void *                  m_data;
	Properties              m_properties;
};


#endif
