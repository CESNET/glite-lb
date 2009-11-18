#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <map>
#include <string>

class Properties {
public:

	// default constructor
	Properties() 
		: properties()
		{}

	// accessors
	std::string& getProperty(const std::string &key) 
		{ return properties[key]; }

	void setProperty(const std::string &key, std::string &val) 
		{ properties[key] = val; }

	// iterators
	typedef std::map<std::string,std::string>::iterator  iterator;

	iterator begin() 
		{ return properties.begin(); }

	iterator end()
		{ return properties.end(); }

	
private:
	std::map<std::string,std::string> properties;
};

#endif
