#ifndef __GLITE_LB_PLAIN_IO_H__
#define __GLITE_LB_PLAIN_IO_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _edg_wll_PlainConnection {
	int sock;
	char *buf;
	size_t bufSize;
	size_t bufUse;
} edg_wll_PlainConnection;

	
int edg_wll_plain_accept(
	int sock,
	edg_wll_PlainConnection *conn);

int edg_wll_plain_close(
	edg_wll_PlainConnection *conn);

int edg_wll_plain_read(
	edg_wll_PlainConnection *conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_read_full(
	edg_wll_PlainConnection *conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_write_full(
	edg_wll_PlainConnection *conn,
	const void *buf,
	size_t bufsz,
	struct timeval *timeout);

#ifdef __cplusplus
} 
#endif
	
#endif /* __GLITE_LB_PLAIN_IO_H__ */
