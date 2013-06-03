#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>

#include <glite/lbu/log.h>
#include <glite/lb/context-int.h>

#include "crypto.h"

int
sha256_salt(edg_wll_Context ctx, const char *string,
	    const char *salt, char **hash)
{
    SHA256_CTX context;
    unsigned char md[SHA256_DIGEST_LENGTH];
    char output[2*SHA256_DIGEST_LENGTH+1];
    char *input;
    int ret, i;

    SSL_library_init();
    OpenSSL_add_all_algorithms();

    ret = asprintf(&input, "%s%s", string, salt);
    if (ret < 0)
	return edg_wll_SetError(ctx, ENOMEM, "Computing hash");

    SHA256_Init(&context);
    SHA256_Update(&context, input, strlen(input));
    SHA256_Final(md, &context);
    free(input);

    for (i=0; i < SHA256_DIGEST_LENGTH; i++)
	sprintf(output + i*2, "%02x", md[i]);
    output[SHA256_DIGEST_LENGTH*2] = '\0';

    *hash = strdup(output);
    if (*hash == NULL)
	return edg_wll_SetError(ctx, ENOMEM, "Computing hash");

    return 0;
}

int
generate_salt(edg_wll_Context ctx, char **new_salt)
{
    int ret, i;
    unsigned char rand_bytes[16];
    char salt[sizeof(rand_bytes)*2 + 1];

    ret = RAND_bytes(rand_bytes, sizeof(rand_bytes));
    if (ret != 1) {
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_ERROR,
		"Failed to generate random seed");
	return edg_wll_SetError(ctx,EINVAL,"Anonymization failed");
    }

    for (i = 0; i < sizeof(rand_bytes); i++)
	sprintf(salt + i*2, "%02x", rand_bytes[i]);
    salt[sizeof(salt) - 1] = '\0';
 
    *new_salt = strdup(salt);
    if (*new_salt == NULL)
	return edg_wll_SetError(ctx,ENOMEM,NULL);

    return 0;
}
