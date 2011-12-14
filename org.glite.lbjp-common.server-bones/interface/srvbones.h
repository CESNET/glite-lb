/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
	GLITE_SBPARAM_LOG_REQ_CATEGORY,		/**< common logging requests category (char*) */
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

/**
 * helper common function to daemonize server
 *
 * \returns 1 OK, 0 error writtten to stderr
 */
int glite_srvbones_daemonize(const char *servername, const char *custom_pidfile, const char *custom_logfile);

/**
 * Create listening socket.
 *
 * \return 0 OK, non-zero error
 */
int glite_srvbones_daemon_listen(const char *name, char *port, int *conn_out);

#ifdef __cplusplus
}
#endif

#endif /* __ORG_GLITE_LB_SERVER_BONES_BONES_H__ */
