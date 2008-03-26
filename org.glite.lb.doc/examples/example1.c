#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>

#include "client_headers.h"

#include "glite/lb/xml_conversions.h"

extern void print_jobs(edg_wll_JobStat *);

int main(int argc,char **argv)
{

#include "example1_code.c"

	return 0;
}
