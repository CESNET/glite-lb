#ifndef __EDG_WORKLOAD_LOGGING_COMMON_LB_PLAIN_IO_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_LB_PLAIN_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

int edg_wll_plain_read_fullbuf(
	int conn,
	void *outbuf,
	size_t outbufsz,
	struct timeval *timeout);

int edg_wll_plain_read_full(
	int conn,
	void **out,
	size_t outsz,
	struct timeval *timeout);

int edg_wll_plain_write_full(
	int conn,
	const void *buf,
	size_t bufsz,
	struct timeval *timeout);

#ifdef __cplusplus
} 
#endif
	
#endif /* __EDG_WORKLOAD_LOGGING_COMMON_LB_PLAIN_IO_H__ */
