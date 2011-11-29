#include "example_locl.h"

int
main(int argc, char *argv[])
{
    edg_wll_GssCred cred = NULL;
    edg_wll_GssConnection conn;
    edg_wll_GssStatus gss_ret;
    int ret;
    char buf[1024];
    struct addrinfo hints;
    struct addrinfo *ai;
    int sock;

    ret = edg_wll_gss_initialize();
    if (ret) {
	fprintf(stderr, "Failed to initialize\n");
	return 1;
    }

    memset (&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo (NULL, "1234", &hints, &ai);
    if (ret) {
	fprintf(stderr, "getaddrinfo() failed: %s\n",
		gai_strerror(ret));
	return 1;
    }

    sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) {
	perror("Can't create socket");
	return 1;
    }

    ret = bind(sock, ai->ai_addr, ai->ai_addrlen);
    if (ret) {
	perror("Can't bind socket");
	return 1;
    }

    ret = listen(sock, 10);
    if (ret) {
	perror("Can't bind socket");
	return 1;
    }

    ret = edg_wll_gss_acquire_cred_gsi(NULL, NULL, &cred, &gss_ret);
    if (ret) {
	print_gss_err(ret, &gss_ret, "Failed to load credentials");
	return 1;
    }

    while (1) {
	int client;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_len;
	edg_wll_GssPrincipal princ;
	
	client_addr_len = sizeof(client_addr);
	memset(&client_addr, 0, client_addr_len);
	client = accept(sock, (struct sockaddr *) &client_addr,
			&client_addr_len);
	if (client < 0) {
	    perror("Failed to accept incomming connection\n");
	    continue;
	}

	ret = edg_wll_gss_accept(cred, client, NULL, &conn, &gss_ret);
	if (ret) {
	    print_gss_err(ret, &gss_ret, "Failed to accept client");
	    edg_wll_gss_close(&conn, NULL);
	    continue;
	}

	ret = edg_wll_gss_get_client_conn(&conn, &princ, &gss_ret);
	if (ret) {
	    print_gss_err(ret, &gss_ret, "Failed to get client's name");
	    edg_wll_gss_close(&conn, NULL);
	    continue;
	}

	printf("Connection from %s\n", princ->name);

	ret = edg_wll_gss_read(&conn, buf, sizeof(buf), NULL, &gss_ret);
	edg_wll_gss_close(&conn, NULL);
	if (ret < 0) {
	    print_gss_err(ret, &gss_ret, "Failed to read message");

	    continue;
	}

	printf("Client sent: %s\n", buf);
    }

    return 0;
}
