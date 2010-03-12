#ifndef _GLITE_STRMD5_H
#define _GLITE_STRMD5_H

#ifdef __cplusplus
extern "C" {
#endif

#ident "$Header$"
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


/* Compute MD5 sum of the first argument.
 * The sum is returned in the 16-byte array pointed to by 2nd argument
 * 	(if not NULL)
 *
 * Return value: ASCII string of the sum, i.e. 32 characters [0-9a-f]
 * 	(pointer to static area, changed by subsequent calls)
 */

char *strmd5(const char *src, unsigned char *dst);

/**
 * Returns: allocated 32bytes long ASCII string with md5 sum
 * of the first argument
 */
char *str2md5(const char *src);

/**
 * Returns: allocated 22bytes long ASCII string with md5 sum in base64
 * format of the source argument
 */
char *str2md5base64(const char *src);

int base64_encode(const void *enc, int enc_size, char *out, int out_max_size);
int base64_decode(const char *enc,char *out,int out_size);

#ifdef __cplusplus
};
#endif

#endif /* _GLITE_STRMD5_H */
