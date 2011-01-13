#include <stdio.h>
#include <stdlib.h>

#include "log.h"

int main() {
	char *line = NULL;
	size_t i, n;
	const char *testcat = "miaow";

	n = 10000;
	line = malloc(n);
	for (i = 0; i < n; i++) line[i] = (i % 64) ? 'A' : '\n';
	line[n - 3] = '#';
	line[n - 2] = '\n';
	line[n - 1] = '\0';
	

	if (glite_common_log_init()) {
		fprintf(stderr,"glite_common_log_init() failed, exiting.");
		return 2;
	}

	glite_common_log(testcat, LOG_PRIORITY_ERROR, "%s", line);
	glite_common_log(testcat, LOG_PRIORITY_DEBUG, "%s", line);
	printf("%s\n", line);

	glite_common_log_fini();

	free(line);
	return 0;
}
