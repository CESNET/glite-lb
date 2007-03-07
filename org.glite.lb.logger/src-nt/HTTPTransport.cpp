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
	int len;

	switch(state) {
	case NONE:
		state = IN_REQUEST;
		pos = buffer;

	case IN_REQUEST:
	case IN_HEADERS:
		len = conn->read(pos, sizeof(buffer) - (pos - buffer));
		if(len < 0) {
			// error during request
		} else if(len == 0) {
			// other side closed connection
		} else {
			char *cr = NULL, *p = buffer, *s = buffer;
			bool crlf_seen = false;

			// parse buffer, look for CRLFs
			//   s - start scan position
			//   p - start of current token
			//   cr - current CRLF position
			//   crlf_seen <=> previous block ends with CRLF
			while((state != IN_BODY) &&
			      (s < buffer + len) && 
			      (cr = (char*)memchr(s, '\r', len - (s - buffer)))) {
				if((cr < buffer + len - 1) && 
				   (cr[1] == '\n')) {
					// found CRLF
					if(state == IN_REQUEST) {
						// found end of request
						request.append(p, cr - p);
						// change state
						state = IN_HEADERS;
						// start new tokens
						p = cr + 2;
					} else {
						// headers continue. parse the current one
						parseHeader(s, cr - s);
					}
					if(crlf_seen && (s == cr)) {
						// found CRLFCRLF
						state = IN_BODY;
					} 
					// next scan starts after CRLF
					s = cr + 2; 
					// we have seen CRLF
					crlf_seen = true;
				} else {
					if(crlf_seen && (s == cr)) {
						if(cr < buffer + len - 1) {
							// found CRLFCRx
							// continue scan behind this
							s = cr + 2;
						} else {
							// found CRLFCR at the end of buffer
							// s points behind buffer => scan ends
							s = cr + 1;
							// cr points at the CRLFCR => it will be left for next pass
							cr = cr - 2;
						}
					} else {
						// single '\r'  - skip it, 
						// or '\r' at the end of buffer - skip it, that ends scanning
						s = cr + 1;
					}	
					crlf_seen = false;
				}
			}
			// copy the current token into appropriate variable,
			// but leave the trailing \r[\n] in buffer
			if(!cr) cr = buffer + len;
			if(state == IN_REQUEST) 
				request.append(p, cr - p);
			else 
				headers.append(p, cr - p);
			if(state == IN_BODY) {
				// we found body
				// content-length should be set at the moment
				if(content_length > 0) {
					body = (char*)malloc(content_length);
					if(body == NULL) {
						// chyba alokace
					}
					// move rest of buffer to body
					if(s < buffer + len) {
						memmove(body, s, buffer + len - s);
						pos = body + (buffer + len - s);
					}
				} else {
					// report error
				}
			} else {
				// move the trailing characters to the front
				if(cr < buffer + len) 
					memmove(buffer, cr, buffer + len - cr);
				// and set pos to point at the first free place
				pos = buffer + (buffer + len - cr);
			}
		}
		break;

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


int
HTTPTransport::parseHeader(const char *s, unsigned int len)
{
	std::cout.write(s, len);
	return(0);
}
