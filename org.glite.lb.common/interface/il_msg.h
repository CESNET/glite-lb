#ifndef __GLITE_LB_IL_MSG_H__
#define __GLITE_LB_IL_MSG_H__

#include "glite/lb/il_string.h"

#ident "$Header$"

/*
 * Communication protocol of the interlogger.
 *
 * The "philosophy" behind is:
 *   - when sending messages, use 'encode_il_*' to create
 *     data packet, which can be shipped out at once using write 
 *     (or similar function). The length of the data packet is returned.
 * 
 *   - when receiving messages, call read_il_data() supplying pointer to 
 *     function, which can be used to get data from input stream.
 *     The resulting data packet may be decoded by calling appropriate
 *     'decode_il_*' functions. 
 *
 * This is completely independent of the underlying transport protocol.
 * By rewriting this (part of) library, you can change the protocol IL 
 * uses to communicate with event destinations.
 *
 * Yes, for clean design there should be send_il_data(), which would 
 * send the 17 byte header first and the data second; in my opinion it would be 
 * too complicated, so the 17 byte header is included by the encoding functions.
 *
 * Return values - length of the output data or error code < 0 in case of error.
 * No context is used (except by the supplied reader function itself).
 *
 */

int encode_il_msg(char **, const il_octet_string_t *);
int encode_il_reply(char **, int, int, const char *);
int decode_il_msg(il_octet_string_t *, const char *);
int decode_il_reply(int *, int *, char **, const char *);
int read_il_data(void *user_data,
		 char **buffer,  
		 int (*reader)(void *user_data, char *buffer, const int));

enum {
  LB_OK    = 0,
  LB_NOMEM = 200,
  LB_PROTO = 400,
  LB_AUTH  = 500,
  LB_PERM  = 600,
  LB_DBERR = 700,
  LB_SYS   = 800,
  LB_TIME  = 900
};

#endif /* __GLITE_LB_IL_MSG_H__ */
