#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_LOGGING_EXCEPTIONS_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_LOGGING_EXCEPTIONS_HPP__

#ident "$Header$"

/** @file LoggingExceptions.h
 *  @version $Revision$
 */

#include <stdexcept>
#include <sstream>
#include <string>

#include <pthread.h>

EWL_BEGIN_NAMESPACE

/** Base class for all exceptions thrown by the L&B C++ classes.
 *
 * This class serves as a common base for all exceptions thrown by the
 * L&B C++ API classes. In case when the exception is constructed from
 * another exception (creating chained exception list), the error
 * message is created by concatenating the error message of the
 * original exception and the new error message. All the other
 * functionality (printing error message, logging it, printing stack
 * trace) is inherited from the base class glite::wmsutils::exception::Exception.
 */
class Exception: public std::runtime_error {
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
		
		o << "glite.lb.Exception: " << exception << std::endl
		  << "\tat " << method << "[" << source << ":" << line << "]" 
		  << std::endl;
		return o.str();
	}
};


/** Exception encapsulating error states originating in the L&B.
 *
 * This class is simple child of the base Exception class, adding no
 * new functionality. Its purpose is to differentiate the error
 * conditions originating in the L&B subsystem from other errors (such
 * as system ones).
 */
class LoggingException: public Exception {
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
	LoggingException(const std::string& source,
			 int line_number,
			 const std::string& method,
			 int   code,
			 const std::string& exception) 
		: Exception(source, line_number, method, code, exception)
		{};
	
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
	LoggingException(const std::string& source,
			 int line_number,
			 const std::string& method,
			 int   code,
			 const std::string& exception, 
			 const Exception &exc)
		: Exception(source, line_number, method, code, exception)
		{};
};


/** Exceptions caused by system errors.
 *
 * This class represents error conditions caused by failing system
 * calls. The error message is augmented with the system error message
 * obtained by calling strerror().
 */
class OSException: public Exception {
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
	OSException(const std::string& source,
		    int line_number,
		    const std::string& method,
		    int   code,
		    const std::string& exception)
		: Exception(source, 
			    line_number, 
			    method, 
			    code, 
			    exception + ": " + strerror(code))
		{};
	
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
	OSException(const std::string& source,
		    int line_number,
		    const std::string& method,
		    int   code,
		    const std::string& exception,
		    const Exception &exc)
		: Exception(source, 
			    line_number, 
			    method, 
			    code, 
			    exception + ": " + strerror(code))
		{};
};


/** Mandatory exception fields.
 *
 * This defines the mandatory parameters for all exception
 * constructors (filename, line, method name).
 */
#define EXCEPTION_MANDATORY                           \
	__FILE__,                                     \
        __LINE__,                                     \
        std::string(CLASS_PREFIX) + __FUNCTION__         

/** Stacking exceptions.
 *
 * This was originally used for creating the exception chain; now the
 * same result is achieved by adding the nested exception to the
 * constructor parameter list.
 */
#define STACK_ADD                                     

/** Utility macro to throw LoggingException.
 *
 * This macro is used to obtain the L&B error message and throw the
 * appropriate exception.
 * Note: we can use __LINE__ several times in macro, it is expanded into
 * one row.
 */
#define throw_exception(context, exception)           \
{ STACK_ADD;                                          \
  {                                                   \
     char *text, *desc;                               \
     int  code;                                       \
     std::string exc;                                      \
                                                      \
     code = edg_wll_Error((context), &text, &desc);   \
     exc = exception;                                 \
     if (text) {				      \
     	exc += ": ";                                  \
    	exc += text;                                  \
     }						      \
     if (desc) {				      \
	exc += ": ";                                  \
	exc += desc;                                  \
     }						      \
     free(text);                                      \
     free(desc);                                      \
     throw LoggingException(EXCEPTION_MANDATORY,      \
	                    code,                     \
		            exc);                     \
  }						      \
}
 
/** Utility macro to check result of L&B calls.
 *
 * Checks return value of L&B calls and throws exception if the code
 * failed.
 */
#define check_result(code, context, desc)             \
  if((code)) throw_exception((context), desc)



EWL_END_NAMESPACE

#endif
