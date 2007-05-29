#ifndef __ORG_GLITE_LB_SERVER_BONES_BONES_H__
#define __ORG_GLITE_LB_SERVER_BONES_BONES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _glite_srvbones_param_t {
	GLITE_SBPARAM_SLAVES_COUNT,		/**< number of slaves */ 
	GLITE_SBPARAM_SLAVE_OVERLOAD,		/**< queue items per slave */
	GLITE_SBPARAM_SLAVE_CONNS_MAX,		/**< commit suicide after that many connections */

/* NULL for timeouts means infinity */
	GLITE_SBPARAM_IDLE_TIMEOUT,		/**< keep idle connection that long (timeval) */
	GLITE_SBPARAM_CONNECT_TIMEOUT,		/**< timeout for establishing a connection (timeval) */
	GLITE_SBPARAM_REQUEST_TIMEOUT,		/**< timeout for a single request (timeval)*/
} glite_srvbones_param_t;

typedef int (*slave_data_init_hnd)(void **);

struct glite_srvbones_service {
	char	*id;			/**< name of the service */
	int	conn;			/**< listening socket */

/** Handler called by slave on a newly established connection, 
 * i.e. after accept(2).
 * \param[in] conn 		the accepted connection
 * \param[inout] timeout	don't consume more, update with the remaining time
 * \param[inout] user_data	arbitrary user data passed among the functions
 */
	int	(*on_new_conn_hnd)(	
		int conn,		
		struct timeval *timeout,
		void *user_data
	);


/** Handler called by slave to serve each request.
  * \param[in] conn 		connection to work with
  * \param[inout] timeout	don't consume more, update with the remaining time
  * \param[inout] user_data	arbitrary user data passed among the functions
  *
  * \retval	0	OK, connection remains open
  * \retval	ENOTCON	terminated gracefully, bones will clean up
  * \retval	>0	other POSIX errno, non-fatal error
  * \retval	<0	fatal error, terminate slave
  */
	int	(*on_request_hnd)(
			int conn,
			struct timeval *timeout,
			void *user_data
	);

/** Handler called by master to reject connection on server overload.
  * Should kick off the client quickly, not imposing aditional load
  * on server or blocking long time.
  */
	int	(*on_reject_hnd)(int conn);

/** Handler called by slave before closing the connection.
  * Perform server-side cleanup, and terminate the connection gracefully
  * if there is a way to do so (the disconnect is server-initiated).
  * close(conn) is called by bones then.
  * \param[in] conn 		connection to work with
  * \param[inout] timeout	don't consume more time
  * \param[inout] user_data	arbitrary user data passed among the functions
  */
	int	(*on_disconnect_hnd)(
		int conn,
		struct timeval *timeout,
		void *user_data
	);
};

extern int glite_srvbones_set_param(glite_srvbones_param_t param, ...);


/** Main server function. 
 * 
 * \param[in] slave_data_init_hnd 	callback initializing user data on every slave
 */
extern int glite_srvbones_run(
	slave_data_init_hnd		slave_data_init,
	struct glite_srvbones_service  *service_table,
	size_t				table_sz,
	int				dbg);

#ifdef __cplusplus
}
#endif

#endif /* __ORG_GLITE_LB_SERVER_BONES_BONES_H__ */
