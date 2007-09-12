#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <map>

class Properties {
public:

	// default constructor
	Properties() 
		: properties()
		{}

	// accessors
	string& getProperty(string &key) 
		{ return properties[key]; }

	void setProperty(string &key, string &val) 
		{ properties[key] = val; }

	// iterators
	typedef std::map<string,string>::iterator  iterator;

	iterator begin() 
		{ return properties.begin(); }

	iterator end()
		{ return properties.end(); }

	
private:
	std::map<string,string> properties;
};

#endif
