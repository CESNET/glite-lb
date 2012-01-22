#ifndef GLITE_LB_CRYPTO_H
#define GLITE_LB_CRYPTO_H

#include <glite/lb/context.h>

int
sha256_salt(edg_wll_Context ctx, const char *string,
	    const char *salt, char **hash);

int
generate_salt(edg_wll_Context ctx, char **new_salt);

#endif
