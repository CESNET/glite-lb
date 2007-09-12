#ifndef _MESSAGE_H_
#define _MESSAGE_H

class Message {
public:
	class ID {
	public:
	private:
		unsigned char m_uuid[16];
	};

	Message(void * data, unsigned int length) 
		: m_length(length), 
		  m_data(data)
		{}

private:
	ID                      m_id;
	unsigned int            m_length;
	void *                  m_data;
};

#endif
