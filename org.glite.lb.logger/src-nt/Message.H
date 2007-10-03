#ifndef _MESSAGE_H_
#define _MESSAGE_H

#include "Properties.H"

#include <string>

class Message {
public:
	class ID {
	public:
	private:
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

	std::string getProperty(const std::string &name, std::string &val) const
		{ return m_properties.getProperty(name); }

	void setProperty(const std::string &name, std::string &val) 
		{ m_properties.setProperty(name, val); }

	Properties& getProperties() 
		{ return m_properties; }

private:
	ID                      m_id;
	unsigned int            m_length;
	void *                  m_data;
	Properties              m_properties;
};


#endif
