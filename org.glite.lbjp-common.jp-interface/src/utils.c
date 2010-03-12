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

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glite/jobid/strmd5.h"
#include "types.h"
#include "context.h"
#include "known_attr.h"
#include "attr.h"

/*
#include "feed.h"
#include "tags.h"
*/

#include "backend.h"


typedef struct _rl_buffer_t {
        char                    *buf;
        size_t                  pos, size;
        off_t                   offset;
} rl_buffer_t;

/*
 * realloc the line to double size if needed
 *
 * \return 0 if failed, did nothing
 * \return 1 if success
 */
int check_realloc_line(char **line, size_t *maxlen, size_t len) {
        void *tmp;

        if (len > *maxlen) {
                *maxlen <<= 1;
                tmp = realloc(*line, *maxlen);
                if (!tmp) return 0;
                *line = tmp;
        }

        return 1;
}


char* glite_jpps_get_namespace(const char* attr){
        char* namespace = strdup(attr);
        char* colon = strrchr(namespace, ':');
        if (colon)
                namespace[strrchr(namespace, ':') - namespace] = 0;
        else
                namespace[0] = 0;
        return namespace;
}

