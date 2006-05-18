#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "glite/lb/lb_perftest.h"

/*
extern char *optarg;
extern int opterr,optind;

Vzorem by mel byt examples/stresslog.c, ovsem s mirnymi upravami:

1) nacte udalosti jednoho jobu z daneho souboru, rozparsuje a ulozi do pameti
2) je-li pozadan, zaregistruje dany pocet jobid
3) vypise timestamp
4) odesle rozparsovane udalosti pod ruznymi jobid (tolika, kolik ma
   byt jobu - argument) na dany cil (localloger, interlogger, bkserver, proxy)
5) odesle specialni ukoncovaci udalost
6) exit

        read_job("small_job",&event_lines);
        gettimeoday(&zacatek)
        for (i=0; i<1000; i++) {
                for (e=0; event_lines[e]; e++)
                        edg_wll_ParseEvent(event_lines[e],&event[e]);
        }
        gettimeofday(&konec);

        printf("parsovani",konec-zacatek/1000);
                        
        gettimeofday(&zacatek);
                switch (event[e].type) {
                        ...
                        edg_wll_LogBlaBla()
                

*/

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "dst", required_argument, NULL, 'd' },
	{ "test", required_argument, NULL, 't' },
	{ "file", required_argument, NULL, 'f'},
	{ "num", required_argument, NULL, 'n'},
	{ "lbproxy", no_argument, 0, 'x'},
	{ NULL, 0, NULL, 0}
};


void
usage(char *program_name)
{
	fprintf(stderr, "Usage: %s [-x] [-d destname] [-t testname] -f filename -n numjobs \n"
		"-h, --help            display this help and exit\n"
                "-d, --dst <destname>  destination host name\n"
		"-t, --test <testname> name of the test\n"
		"-f, --file <filename> name of the file with prototyped job events\n"
		"-n, --num <numjobs>   number of jobs to generate\n"
		"-x, --lbproxy         feed to LB Proxy\n",
	program_name);
}


int
main(int argc, char *argv[])
{

	char 	*destname= NULL,*testname = NULL,*filename = NULL;
	int 	lbproxy = 0, num_jobs = 1;
	int 	opt;

	opterr = 0;

	while ((opt = getopt_long(argc,argv,"hd:t:f:n:x",
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'd': destname = (char *) strdup(optarg); break;
			case 't': testname = (char *) strdup(optarg); break;
			case 'f': filename = (char *) strdup(optarg); break;
			case 'n': num_jobs = atoi(optarg); break;
			case 'x': lbproxy = 1; break;
			case 'h': 
			default:
				usage(argv[0]); exit(0);
		}
	} 

	if (num_jobs <= 0) {
		fprintf(stderr,"%s: wrong number of jobs\n",argv[0]);
		usage(argv[0]);
		exit(1);
	}

	if (!filename) {
		fprintf(stderr,"%s: -f required\n",argv[0]);
		usage(argv[0]);
		exit(1);
	}

	if (glite_wll_perftest_init(destname, NULL, testname, filename, num_jobs) < 0) {
		fprintf(stderr,"%s: glite_wll_perftest_init failed\n",argv[0]);
	}

	return 0;

}
