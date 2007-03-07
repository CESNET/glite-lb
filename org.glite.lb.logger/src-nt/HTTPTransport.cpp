#include "HTTPTransport.H"

#include <iostream>
#include <string.h>


HTTPTransport::Factory HTTPTransport::theFactory;


HTTPTransport::~HTTPTransport()
{
	if(body) free(body);
}


void
HTTPTransport::onReady()
{
	switch(state) {
	case NONE:
		state = IN_REQUEST;
		pos = buffer;

	case IN_REQUEST:
	case IN_HEADERS:
		len = conn->read(buffer, sizeof(buffer));
		if(len < 0) {
			// error during request
		} else if(len == 0) {
			// other side closed connection
		} else {
			char *cr;

			// parse buffer, look for CRLF
			cr = memchr(buffer, '\r', len);
			if(cr) {
				if((cr < buffer + len - 1) &&
				   (cr[1] == '\n')) {
					// found CRLF
					
				} else 
					cr = buffer + len;
			}
			// append everything up to the cr into appropriate variable
			if(state == IN_REQUEST) 
				request.append(buffer, cr - buffer);
			else
				headers.append(buffer, cr - buffer);
		}

	case IN_BODY:
		break;
	}

}


void 
HTTPTransport::onTimeout()
{
}


void 
HTTPTransport::onError()
{
}


