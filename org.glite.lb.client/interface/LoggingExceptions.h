#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_LOGGING_EXCEPTIONS_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_LOGGING_EXCEPTIONS_HPP__

#include "edg/workload/common/utilities/Exceptions.h"

#include <pthread.h>

#ident "$Header$"

/** @file LoggingExceptions.h
 *  @version $Revision$
 */

EWL_BEGIN_NAMESPACE;

class Exception: public edg::workload::common::utilities::Exception {
public:
	
	/* constructor for mandatory fields */
	Exception(const std::string& source,
		  int line_number,
		  const std::string& method,
		  int   code,
		  const std::string& exception) 
		: edg::workload::common::utilities::Exception(source, 
							      line_number, 
							      method, 
							      code, 
							      "edg::workload::logging::Exception")
		{ error_message = exception; };
	
	/* constructor for mandatory fields AND exception chain */
	Exception(const std::string& source,
		  int line_number,
		  const std::string& method,
		  int   code,
		  const std::string& exception,
		  const edg::workload::common::utilities::Exception &exc)
		: edg::workload::common::utilities::Exception(source, 
							      line_number, 
							      method, 
							      code, 
							      "edg::workload::logging::Exception")
		{ error_message = exception + ": " + exc.what(); };
};


class LoggingException: public Exception {
public:
	
	/* constructor for mandatory fields */
	LoggingException(const std::string& source,
			 int line_number,
			 const std::string& method,
			 int   code,
			 const std::string& exception) 
		: Exception(source, line_number, method, code, exception)
		{};
	
	/* constructor for mandatory fields AND exception chain */
	LoggingException(const std::string& source,
			 int line_number,
			 const std::string& method,
			 int   code,
			 const std::string& exception, 
			 const edg::workload::common::utilities::Exception &exc)
		: Exception(source, line_number, method, code, exception)
		{};
};


class OSException: public Exception {
public:
	
	/* constructor for mandatory fields */
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
	
	/* constructor for mandatory fields AND exception chain */
	OSException(const std::string& source,
		    int line_number,
		    const std::string& method,
		    int   code,
		    const std::string& exception,
		    const edg::workload::common::utilities::Exception &exc)
		: Exception(source, 
			    line_number, 
			    method, 
			    code, 
			    exception + ": " + strerror(code))
		{};
};


#define EXCEPTION_MANDATORY                           \
	__FILE__,                                     \
        __LINE__,                                     \
        std::string(CLASS_PREFIX) + __FUNCTION__         

#define STACK_ADD                                     

/* note: we can use __LINE__ several times in macro, it is expanded into one row */
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
 
#define check_result(code, context, desc)             \
  if((code)) throw_exception((context), desc)



EWL_END_NAMESPACE;

#endif
