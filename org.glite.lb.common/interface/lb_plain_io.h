#ifndef __EDG_WORKLOAD_LOGGING_COMMON_LB_PLAIN_IO_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_LB_PLAIN_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _edg_wll_Connection {
	int	sock;
	char   *buffer;
	size_t	bufsz;
	size_t	bufuse;
} edg_wll_Connection;


int edg_wll_plain_accept(
	int sock,
	edg_wll_Connection *conn);

int edg_wll_plain_read(
	edg_wll_Connection *conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_read_full(
	edg_wll_Connection *conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_write_full(
	edg_wll_Connection *conn,
	const void *buf,
	size_t bufsz,
	struct timeval *timeout);

#ifdef __cplusplus
} 
#endif
	
#endif /* __EDG_WORKLOAD_LOGGING_COMMON_LB_PLAIN_IO_H__ */
