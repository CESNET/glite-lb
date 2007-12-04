#ifndef GLITE_JOBID_JOBID_H
#define GLITE_JOBID_JOBID_H

#include <string>
#include <stdexcept>
#include <new>
#include <cerrno>
#include <cassert>

#include "glite/jobid/cjobid.h"


namespace glite { 
namespace jobid {


/**
 * class glite::jobid::JobIdError
 */
	
class JobIdError : public std::runtime_error {
public:
	/** Constructor for mandatory fields.
	 *
	 * Updates all the mandatory fields and names the exception.
	 * \param[in] exception 	Error message describing the exception.
	 */
	JobIdError(std::string const& exception)
	    : std::runtime_error(std::string("JobId: bad argument (") + exception + ")")
	{}
		
	virtual ~JobIdError() throw() 
	{}

};


/**
 * class glite::jobid::JobId
 */

class JobId 
{
public:
	class Hostname {
	public: 
		std::string const& name;
		Hostname(std::string const& n) : name(n) 
		{}
	};

	//@name Constructors/Destructor
	//@{

	/**
	 * Constructor from string format.
	 * @param  job_id_string 
	 * @throws  Exception When a string is passed in a wrong format
	 */
	JobId(std::string const& job_id_string);

	/** 
	 * Constructor from job id components.
	 * \param host
	 * \param port
	 * \param unique
	 */
	explicit JobId(Hostname const& host = Hostname("localhost"), 
		       int port = GLITE_JOBID_DEFAULT_PORT, 
		       std::string const& unique = std::string(""));

	/**
	 * Destructor.
	 */
	~JobId();

	//@}

	//@ Conversions, assignments
	//@{

	/**
	 * Copy constructor.
	 */
	JobId(JobId const&);

	/** 
	 * Constructor from C jobid. 
	 * \param src C API job id 
	 */
	explicit JobId(glite_jobid_const_t src);

	/** 
	 * Assignment operator.
	 * Create a deep copy of the JobId instance.
	 */
	JobId& operator=(JobId const& src);
 
	/** 
	 * Casting operator.
	 */
	glite_jobid_const_t c_jobid() const;

	/** 
	 * Returns the string representing the job id
	 * @return String representation of a JobId
	 */
	std::string toString() const;

	//@}


	/**@name Member access
	 * @{ 
	 */

	/**
	 * Get server:port.
	 * @return hostname and port
	 */
	std::string server() const;

	/**
	 * Get host.
	 * @return hostname
	 */
	std::string host() const;

	/** 
	 * Get port.
	 * @return port
	 */
	int port() const;

	/**
	 * Get unique.
	 * @return unique string
	 */
	std::string unique() const;

	//@}

private:
	glite_jobid_t  m_jobid;
};



// -------------------- implementation ------------------------

inline
JobId::JobId(std::string const& job_id_string) 
{
	int ret = glite_jobid_parse(job_id_string.c_str(), 
				    &m_jobid);
	switch(ret) {
	case EINVAL:
		throw JobIdError(job_id_string);

	case ENOMEM:
	        throw std::bad_alloc();

	default:
	    break;
	}
}


inline
JobId::JobId(JobId::Hostname const& host, int port, std::string const& unique)
{
	if(port < 0) {
		throw JobIdError("negative port");
	}

	int ret = glite_jobid_recreate(host.name.c_str(), port, 
				       unique.empty() ? NULL : unique.c_str(),
				       &m_jobid);
	switch(ret) {
	case EINVAL:
	    throw JobIdError(host.name);
	  
	case ENOMEM:
	    throw std::bad_alloc();

	default:
	    break;
	}
}


inline
JobId::~JobId() {
	glite_jobid_free(m_jobid);
}


inline
JobId::JobId(JobId const& src)
{
	int ret = glite_jobid_dup(src.m_jobid, 
				  &m_jobid);
	if(ret) {
	    // we rely on dup returning only ENOMEM on error
	    assert(ret == ENOMEM);
	    throw std::bad_alloc();
	}
}


inline
JobId::JobId(glite_jobid_const_t src) 
{
	if(src == NULL) {
		throw JobIdError("null");
	}

	int ret = glite_jobid_dup(src,
				  &m_jobid);
	if(ret) {
	    throw std::bad_alloc();
	}
}


inline
JobId& 
JobId::operator=(JobId const& src) 
{
	if(this == &src) {
		return *this;
	}

	glite_jobid_free(m_jobid);
	int ret = glite_jobid_dup(src.m_jobid, 
				  &m_jobid);
	if(ret) {
	    throw std::bad_alloc();
	}
	return *this;
}


inline
glite_jobid_const_t
JobId::c_jobid() const
{
	return m_jobid;
}


inline
std::string
JobId::toString() const
{
	char *out = glite_jobid_unparse(m_jobid);
	std::string res(out);

	free(out);
	return res;
}


inline
std::string
JobId::server() const
{
	char *server = glite_jobid_getServer(m_jobid);
	std::string res(server);

	free(server);
	return res;
}


inline
std::string 
JobId::host() const
{
	char *name;
	unsigned int port;

	glite_jobid_getServerParts_internal(m_jobid, 
					    &name, &port);
	return std::string(name);
}


inline
int
JobId::port() const
{
	char *name;
	unsigned int port;

	glite_jobid_getServerParts_internal(m_jobid,
					    &name, &port);
	return port;
}


inline
std::string
JobId::unique() const
{
	char *unique = glite_jobid_getUnique_internal(m_jobid);
	std::string res(unique);
	
	return res;
}


} // namespace jobid
} // namespace glite

#endif // GLITE_JOBID_JOBID_H
