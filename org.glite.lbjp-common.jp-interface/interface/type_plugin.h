#ifndef GLITE_JP_TYPEPLUGIN_H
#define GLITE_JP_TYPEPLUGIN_H

typedef struct _glite_jp_tplug_data_t {
	
	char	*namespace;
	void	*pctx;

/** Compare attribute values. 
  * \param[in] a value to compare
  * \param[in] b value to compare
  * \param[out] result like strcmp()
  * \param[out] err set if the values cannot be compared
  * \retval 0 OK
  * \retval other error
  */
	int (*cmp)(
		void	*ctx,
		const glite_jp_attrval_t *a,
		const glite_jp_attrval_t *b,
		int	*result);

/** Convert to database string representation.
 * It is guaranteed the returned value can be converted back with
 * from_db().
 * The resulting value may not be suitable for indexing with db engine.
 *
 * \param[in] attr the attribute value to convert
 * \retval NULL can't be converted
 * \retval other the string representation.
 * */
	char * (*to_db_full)(void *ctx,const glite_jp_attrval_t *attr);

/** Convert to a database string representation suitable for indexing.
 * The function is non-decreasing (wrt. cmp() above and strcmp()), however it
 * is not guaranteed to be one-to-one.
 *
 * \param[in] attr the value to convert
 * \param[in] len maximum length of the converted value.
 * \retval NULL can't be converted
 * \retval other the string representation
 */
	char * (*to_db_index)(void *ctx,const glite_jp_attrval_t *attr,int len);

/** Convert from the database format.
 * \param[in] str the string value
 * \param[inout] attr name contains the name of the attribute to be converted
 * 		the rest of attr is filled in.
 */
	int (*from_db)(void *ctx,const char *str,glite_jp_attrval_t *attr);

/** Query for database types suitable to store values returned by
 * to_db_full() and to_db_index(). 
 * Useful for db column dynamic creation etc.
 * Return pointer to internal static data, non-reentrant.
 */
	const char * (*db_type_full)(void *ctx,const char *attr);
	const char * (*db_type_index)(void *ctx,const char *attr,int len);

} glite_jp_tplug_data_t;

/** Plugin init function.
    Must be called init, supposed to be called as many times as required
    for different param's (e.g. xsd files).
    Registers the plugin in ctx.
 */

typedef int (*glite_jp_tplug_init_t)(
	glite_jp_context_t	ctx,
	const char		*param,
	glite_jp_tplug_data_t	*plugin_data
);

#endif /* GLITE_JP_TYPEPLUGIN_H */
