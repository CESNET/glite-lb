// remove protected interface and clear
// remove constructor with string port
// add internal accessors to C struct fields (and use them to avoid unnecessary allocation)
// defaults for the three param constructor (and remove default constructor), make this one explicit
// init variables at definition
// if should be followed by block, & at type
// check operator= for self-assignment
// return ENOMEM and throw std::bad_alloc 
// use unique.empty() rather than unique.size()
// throw exception for JobId(NULL)
// base Exception on runtime_error:
//      <giaco_>	we can then construct the exception with a message of the format:
//	<giaco_>	"JobId: bad argument (<invalid or invalid arguments>)"
//	<giaco_>	the invalid argument/arguments can be
//	<giaco_>	<host> <port> <unique>
//	<giaco_>	<string>
//	<giaco_>	0
//	<giaco_>	I would call the exception JobIdError
// add inline to definitions
 
#ifndef GLITE_JOBID_JOBID_H
#define GLITE_JOBID_JOBID_H

#include <string>
#include <sstream>
#include <stdexcept>

#include "glite/jobid/cjobid.h"


namespace glite { 
namespace jobid {


/**
 * class glite::jobid::Exception
 */

class Exception : public std::runtime_error {
public:
	/** Constructor for mandatory fields.
	 *
	 * Updates all the mandatory fields and names the exception.
	 * \param[in] source 		Source filename where the exception was raised.
	 * \param[in] line_number 	Line in the source that caused the exception.
	 * \param[in] method 		Name of the method that raised the exception.
	 * \param[in] code 		Error code giving the reason for exception.
	 * \param[in] exception 	Error message describing the exception.
	 */
	Exception(const std::string& source,
		  int line_number,
		  const std::string& method,
		  int   code,
		  const std::string& exception)
		: source_file(source), line(line_number), error_code(code),
		std::runtime_error(formatMessage(exception, method, source, line_number))
		{}
		
		
	/** Constructor for mandatory fields and the exception chain.
	 *
	 * Updates all the mandatory fields, names the exception and
	 * adds the original exception's error message to the current
	 * one.
	 * \param[in] source 		Source filename where the exception was raised.
	 * \param[in] line_number 	Line in the source that caused the exception.
	 * \param[in] method 		Name of the method that raised the exception.
	 * \param[in] code 		Error code giving the reason for exception.
	 * \param[in] exception 	Error message describing the exception.
	 * \param[in] exc 		Originally raised exception.
	 */
	 Exception(const std::string& source,
		   int line_number,
		   const std::string& method,
		   int   code,
		   const std::string& exception,
		   const Exception &exc)
		 : source_file(source), line(line_number), error_code(code),
		 std::runtime_error(formatMessage(exception, method, source, line_number) +
				    exc.what())
		 { }
		 
	 virtual ~Exception() throw() 
	 {}

protected:
	/** The name of the file where the exception was raised */
	std::string          source_file;
	/**  line number where the exception was raised */
	int                   line;
	/** the name of the method where the exception was raised */
	std::string          method_name ;
	/**  integer error code representing the cause of the error */
	int                   error_code;

	/** Format message for this particular exception.
	 *
	 * Returns formatted string describing exception.
	 * \param[in] exception 	Error message describing the exception.
	 * \param[in] method 		Name of the method that raised the exception.
	 * \param[in] source 		Source filename where the exception was raised.
	 * \param[in] line_number 	Line in the source that caused the exception.
	 */
	std::string formatMessage(const std::string& exception,
				  const std::string& method,
				  const std::string& source, 
				  int   line) {
		std::ostringstream o;
		
		o << "glite.jobid.Exception: " << exception << std::endl
		  << "\tat " << method << "[" << source << ":" << line << "]" 
		  << std::endl;
		return o.str();
	}
};


/**
 * class glite::jobid::JobId
 */

class JobId 
{
public:
	//@name Constructors/Destructor
	//@{

	/** 
	 * Default constructor; creates a jobid of the form
	 *     https://localhost:<port>/<unique>
	 * useful for internal processing. 
	 */
	JobId();

	/**
	 * Constructor from string format.
	 * @param  job_id_string 
	 * @throws  Exception When a string is passed in a wrong format
	 */
	JobId(const std::string& job_id_string);

	/** 
	 * Constructor from job id components.
	 * \param host
	 * \param port
	 * \param unique
	 */
	JobId(const std::string &host, unsigned int port, const std::string &unique);
	JobId(const std::string &host, const std::string &port, const std::string &unique);

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
	JobId(const JobId&);

	/** 
	 * Constructor from C jobid. 
	 * \param src C API job id 
	 */
	explicit JobId(glite_jobid_const_t src);

	/** 
	 * Assignment operator.
	 * Create a deep copy of the JobId instance.
	 */
	JobId& operator=(const JobId &src);
 
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
	unsigned int port() const;

	/**
	 * Get unique.
	 * @return unique string
	 */
	std::string unique() const;

	//@}

protected:
	
	/**
	 * Clear all members.
	 */
	void clear();

private:
	glite_jobid_t  m_jobid;
};



// -------------------- implementation ------------------------

JobId::JobId() 
{
	int ret; 

	ret = glite_jobid_create("localhost", GLITE_JOBID_DEFAULT_PORT, 
				 &m_jobid);
	if(ret) throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
				    "Failed to create new jobid");
}


JobId::JobId(const std::string &job_id_string) 
{
	int ret;

	ret = glite_jobid_parse(job_id_string.c_str(), 
				&m_jobid);
	if(ret) {
		switch(ret) {
		case EINVAL:
			throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
					    "Failed to parse jobid string");
		default:
			throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
					    "Failed to create new jobid");
		}
	}
}


JobId::JobId(const std::string &host, unsigned int port, const std::string &unique)
{
	int ret;

	ret = glite_jobid_create(host.c_str(), port, unique.size() ? unique.c_str() : NULL,
				 &m_jobid);
	if(ret) throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
				    "Failed to create new jobid");
}


JobId::JobId(const std::string &host, const std::string &port, const std::string &unique)
{
	unsigned int i_port;
	int ret;
	istringstream is(port);

	i_port << is;
	ret = glite_jobid_create(host.c_str(), i_port, unique.size() ? unique.c_str() : NULL,
				 &m_jobid);
	if(ret) throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
				    "Failed to create new jobid");
}


JobId::~JobId() {
	clear();
}


JobId::JobId(const JobId &src)
{
	int ret;

	ret = glite_jobid_dup(src.m_jobid, 
			      &m_jobid);
	if(ret) throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
				    "Failed to create new jobid");
}


JobId::JobId(glite_jobid_const_t src) 
{
	int ret;

	ret = glite_jobid_dup(src,
			      &m_jobid);
	if(ret) throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
				    "Failed to create new jobid");
}


JobId& 
	JobId::operator=(const JobId &src) 
{
	int ret;
	
	clear();
	ret = glite_jobid_dup(src.m_jobid, 
			      &m_jobid);
	if(ret) throw new Exception(__FILE__, __LINE__, __FUNCTION__, ret,
				    "Failed to create new jobid");
	return(*this);
}


glite_jobid_const_t
	JobId::c_jobid() const
{
	return(m_jobid);
}


std::string
	JobId::toString() const
{
	char *out;

	out = glite_jobid_unparse(m_jobid);
	return(std::string(out));
}


std::string
	JobId::server() const
{
	char *server;

	server = glite_jobid_getServer(m_jobid);
	return(std::string(server));
}


std::string 
	JobId::host() const
{
	char *name;
	unsigned int port;

	glite_jobid_getServerParts(m_jobid, 
				   &name, &port);
	return(std::string(name));
}


unsigned int
	JobId::port() const
{
	char *name;
	unsigned int port;

	glite_jobid_getServerParts(m_jobid.
				   &name, &port);
	return(port);
}


std::string
	JobId::unique() const
{
	char *unique;

	unique = glite_jobid_getUnique(m_jobid);
	return(std::string(unique));
}


void
	JobId::clear()
{
	glite_jobid_free(m_jobid);
}


} // namespace jobid
} // namespace glite

#endif // GLITE_JOBID_JOBID_H
