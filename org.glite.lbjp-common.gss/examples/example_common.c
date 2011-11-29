#include "example_locl.h"

void
print_gss_err(int ret, edg_wll_GssStatus *gss_ret, char *msg)
{
    const char *err;

    if (ret == EDG_WLL_GSS_ERROR_GSS) {
	char *gss_err;
	edg_wll_gss_get_error(gss_ret, msg, &gss_err);
	fprintf(stderr, "%s\n", gss_err);
	free(gss_err);
	return;
    }

    switch (ret) {
	case EDG_WLL_GSS_ERROR_ERRNO:
	    err = strerror(errno);
	    break;
	case EDG_WLL_GSS_ERROR_HERRNO:
	    err = hstrerror(errno);
	    break;
	case EDG_WLL_GSS_ERROR_EOF:
	    err = "Peer has closed the connection";
	    break;
	case EDG_WLL_GSS_ERROR_TIMEOUT:
	    err = "Connection timed out";
	    break;
	default:
	    err = "Unknown error";
	    break;
    }
    fprintf(stderr, "%s: %s\n", msg, err);
    return;
}
