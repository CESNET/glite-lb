#include "example_locl.h"

int
main(int argc, char *argv[])
{
    edg_wll_GssCred cred = NULL;
    edg_wll_GssConnection conn;
    edg_wll_GssStatus gss_ret;
    struct timeval tv = {60, 0};
    const char *hostname;
    const char *msg = "Hello";
    int port = 1234;
    int ret;

    if (argc < 2) {
	fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
	return 1;
    }

    hostname = argv[1];

    ret = edg_wll_gss_initialize();
    if (ret) {
	fprintf(stderr, "Failed to initialize\n");
	return 1;
    }

#if 0
    ret = edg_wll_gss_acquire_cred_gsi(NULL, NULL, &cred, &gss_ret);
    if (ret) {
	print_gss_err(ret, &gss_ret, "Failed to load credentials");
	return 1;
    }
#endif

    ret = edg_wll_gss_connect(cred, hostname, port,
			//	  NULL, GSS_C_NO_OID_SET,
				  &tv, &conn, &gss_ret);
    if (ret) {
	print_gss_err(ret, &gss_ret, "Failed to connect to server");
	return 1;
    }

    ret = edg_wll_gss_write(&conn, msg, strlen(msg)+1, &tv, &gss_ret);
    if (ret) {
	print_gss_err(ret, &gss_ret, "Failed to send message");
	return 1;
    }

    edg_wll_gss_close(&conn, &tv);

    return 0;
}
