#ifndef GLITE_JP_FILEPLUGIN_H
#define GLITE_JP_FILEPLUGIN_H

/** Methods of the file plugin. */

typedef struct _glite_jpps_fplug_op_t {

/** Open a file.
\param[in] fpctx	Context of the plugin, returned by its init.
\param[in] bhandle	Handle of the file via JPPS backend.
\param[in] uri		URI (type) of the opened file.
\param[out] handle	Handle to the opened file structure, to be passed to other plugin functions.
*/
	int	(*open)(void *fpctx,void *bhandle,const char *uri,void **handle);
/** Open from a string.
\param[in] fpctx	Context of the plugin, returned by its init.
\param[in] str		The string to use.
\param[in] uri		URI (type) of the string
\param[in] ns		namespace to handle 
\param[out] handle      Handle to the opened file structure, to be passed to other plugin functions.
*/

	int	(*open_str)(void *fpctx,const char *str,const char *uri,const char *ns,void **handle);

/** Close the file. Free data associated to a handle */
	int	(*close)(void *fpctx,void *handle);

/** "Preprocess" the file -- this function is called once after the file is commited */
	int 	(*filecom)(void *fpctx,void *handle);

/** Retrieve value(s) of an attribute.
\param[in] fpctx	Plugin context.
\param[in] handle	Handle of the opened file.
\param[in] ns		Namespace of queried attribute.
\param[in] attr		Queried attribute.
\param[out] attrval	GLITE_JP_ATTR_UNDEF-terminated list of value(s) of the attribute.
			If there are more and there is an interpretation of their order
			they must be sorted, eg. current value of tag is the last one.
\retval	0 success
\retval ENOSYS	this attribute is not defined by this type of file
\retval ENOENT	no value is present 
*/
	int	(*attr)(void *fpctx,void *handle, const char *attr,glite_jp_attrval_t **attrval);

/** File type specific operation. 
\param[in] fpctx	Plugin context.
\param[in] handle	Handle of the opened file.
\param[in] oper		Code of the operation, specific for a concrete plugin.
*/
	int	(*generic)(void *fpctx,void *handle,int oper,...);
	
} glite_jpps_fplug_op_t;

/** Data describing a plugin. */
typedef struct _glite_jpps_fplug_data_t {
	void	*fpctx;		/**< Context passed to plugin operations. */
	char	**uris;		/**< NULL-terminated list of file types (URIs)
					handled by the plugin. */
	char	**classes;	/**< The same as uris but filesystem-friendly
					(can be used to construct file names).*/
	char	**namespaces;	/**< Which attribute namespaces this plugin handles. */

	glite_jpps_fplug_op_t ops; 	/**< Plugin operations. */
} glite_jpps_fplug_data_t;
	
/** Initialisation function of the plugin. 
  Called after dlopen(), must be named "init".
\param[in] ctx		JPPS context
\param[out] data	filled-in plugin data
*/
  
typedef int (*glite_jpps_fplug_init_t)(
	glite_jp_context_t ctx,
	glite_jpps_fplug_data_t *plugin_data
);




/* XXX: not really public interface follows */

int glite_jpps_fplug_load(glite_jp_context_t ctx,int argc,char **argv);
int glite_jpps_fplug_lookup(glite_jp_context_t ctx,const char *uri, glite_jpps_fplug_data_t ***plugin_data);
int glite_jpps_fplug_lookup_byclass(glite_jp_context_t, const char *class,glite_jpps_fplug_data_t ***plugin_data);

#endif /* GLITE_JP_FILEPLUGIN_H */
