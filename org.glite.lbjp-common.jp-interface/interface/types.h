#ifndef GLITE_JP_TYPES_H
#define GLITE_JP_TYPES_H

#include <sys/time.h>

typedef struct _glite_jp_error_t {
	int	code;
	const char	*desc;
	const char	*source;
	struct _glite_jp_error_t *reason;
} glite_jp_error_t;

typedef struct _glite_jp_context {
	glite_jp_error_t *error;
	int	(**deferred_func)(struct _glite_jp_context *,void *);
	void	**deferred_arg;
	void	*feeds;
	struct soap	*other_soap;
	char	*peer;
	void	**plugins;
	void	**type_plugins;
	void	*dbhandle;
	char	**trusted_peers;
	char	*myURL;
	int	noauth;
} *glite_jp_context_t;

typedef enum {
	GLITE_JP_ATTR_ORIG_ANY,		/**< for queries: don't care about origin */
	GLITE_JP_ATTR_ORIG_SYSTEM,	/**< JP internal, e.g. job owner */
	GLITE_JP_ATTR_ORIG_USER,	/**< inserted by user explicitely */
	GLITE_JP_ATTR_ORIG_FILE		/**< coming from uploaded file */
} glite_jp_attr_orig_t;

typedef struct {
	char	*name; 		/**< including namespace */
	char	*value;
	int	binary;		/**< value is binary */
	size_t	size;		/**< in case of binary value */
	glite_jp_attr_orig_t	origin;	
	char	*origin_detail;	/**< where it came from, i.e. file URI:name */
	time_t	timestamp;
} glite_jp_attrval_t;


typedef enum {
	GLITE_JP_QUERYOP_UNDEF,
	GLITE_JP_QUERYOP_EQUAL,
	GLITE_JP_QUERYOP_UNEQUAL,
	GLITE_JP_QUERYOP_LESS,
	GLITE_JP_QUERYOP_GREATER,
	GLITE_JP_QUERYOP_WITHIN,
	GLITE_JP_QUERYOP_EXISTS,
	GLITE_JP_QUERYOP__LAST,
} glite_jp_queryop_t;

typedef struct {
	char	*attr;
	glite_jp_queryop_t op;
	char	*value, *value2;
	int	binary;
	size_t	size,size2;
	glite_jp_attr_orig_t origin;
} glite_jp_query_rec_t;

#endif /* GLITE_JP_TYPES_H */
