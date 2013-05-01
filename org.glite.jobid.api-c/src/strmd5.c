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



#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/md5.h>


static char mbuf[33];
static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static char *b64r;

int base64_encode(const void *enc, int enc_size, char *out, int out_max_size)
{

    unsigned char* enc_buf = (unsigned char*)enc;
    int		  out_size = 0;
    unsigned int  bits = 0;
    unsigned int  shift = 0;

    while ( out_size < out_max_size ) {
	if ( enc_size>0 ) {
	    // Shift in byte
	    bits <<= 8;
	    bits |= *enc_buf;
	    shift += 8;
	    // Next byte
	    enc_buf++;
	    enc_size--;
	} else if ( shift>0 ) {
	    // Pad last bits to 6 bits - will end next loop
	    bits <<= 6 - shift;
	    shift = 6;
	} else {
	    // Terminate with Mime style '='
	    *out = '=';
	    out_size++;

	    return out_size;
	}

	// Encode 6 bit segments
	while ( shift>=6 ) {
	    shift -= 6;
	    *out = b64[ (bits >> shift) & 0x3F ];
	    out++;
	    out_size++;
	}
    }

    // Output overflow
    return -1;
}

int base64_decode(const char *enc,char *out,int max_out_size)
{
	unsigned int	bits = 0;
	int	shift = 0;
	int	out_size = 0;

	if (!b64r) {
		int	i;
		b64r = calloc(128,1);

		for (i=0; b64[i]; i++) b64r[(int)b64[i]] = i;
	}

	while (*enc && *enc != '=') {
		bits <<= 6;
		bits |= b64r[(int)*enc++];
		shift += 6;

		while (shift >= 8) {
			if (out_size >= max_out_size) return -1;
			shift -= 8;
			*out++ = (bits >> shift) & 0xff;
			out_size++;
		}
	}

	return out_size;
}

char *strmd5(const char *s, unsigned char *digest)
{
    MD5_CTX md5;
    unsigned char d[16];
    int	i;

    MD5_Init(&md5);
    MD5_Update(&md5,s,strlen(s));
    MD5_Final(d,&md5);

    if (digest) memcpy(digest,d,sizeof(d));

    for (i=0; i<16; i++) {
	int	dd = d[i] & 0x0f;
	mbuf[2*i+1] = dd<10 ? dd+'0' : dd-10+'a';
	dd = d[i] >> 4;
	mbuf[2*i] = dd<10 ? dd+'0' : dd-10+'a';
    }
    mbuf[32] = 0;
    return (char *) mbuf;
}

char *str2md5(const char *s)
{
    MD5_CTX md5;
    unsigned char d[16];
    char* ret = malloc(33);
    int	i;

    if (!ret)
	return NULL;

    MD5_Init(&md5);
    MD5_Update(&md5, s, strlen(s));
    MD5_Final(d, &md5);

    for (i=0; i<16; i++) {
	int  dd = d[i] & 0x0f;
	ret[2*i+1] = dd<10 ? dd+'0' : dd-10+'a';
	dd = d[i] >> 4;
	ret[2*i] = dd<10 ? dd+'0' : dd-10+'a';
    }
    ret[32] = 0;
    return ret;
}

char *str2md5base64(const char *s)
{
    MD5_CTX md5;
    unsigned char d[16];
    char buf[50];
    int l;

    MD5_Init(&md5);
    MD5_Update(&md5, s, strlen(s));
    MD5_Final(d, &md5);

    l = base64_encode(d, 16, buf, sizeof(buf) - 1);
    if (l < 1)
	return NULL;
    buf[l - 1] = 0;
    return strdup(buf);
}
