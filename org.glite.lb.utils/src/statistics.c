#ident "$Header$"
/*
 * loads L&B plugin to obtain statistics data from a dump file
 * (requires -rdynamic to use fake JP backend symbols)
 */

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <dlfcn.h>
#include <malloc.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "glite/lb/context.h"
#include "glite/lb/jobstat.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/backend.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/known_attr.h"
#include "glite/jp/attr.h"
#include "glite/lb/job_attrs.h"


typedef int init_f(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data);
typedef void done_f(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data);

void process_attrs(glite_jp_context_t jpctx, glite_jpps_fplug_data_t plugin_data, void *data_handle, FILE *outfile);
void process_attrs2(glite_jp_context_t jpctx, glite_jpps_fplug_data_t plugin_data, void *data_handle, FILE *outfile);

static const char rcsid[] = "@(#)$Id$";
static int verbose = 0;
static char *infilename = NULL;
static char *outfilename = NULL;
static int versionone = 0;

static struct option const long_options[] = {
	{ "file", required_argument, 0, 'f' },
	{ "outfile", required_argument, 0, 'o' },
	{ "versionone", no_argument, 0, '1' },
        { "help", no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' },
        { "version", no_argument, 0, 'V' },
        { NULL, 0, NULL, 0}
};

/*
 * usage
 */
static void
usage(char *program_name) {
        fprintf(stdout,"LB statistics\n"
		"- reads a dump file (one job only!) \n"
		"- and outputs an XML with statistics to stdout \n\n"
		"Usage: %s [option]\n"
		"-h, --help                 display this help and exit\n"
		"-V, --version              output version information and exit\n"
		"-v, --verbose              print extensive debug output to stderr\n"
		"-f, --file <file>          dump file to process\n"
		"-o, --outfile <file>       output filename\n"	
		"-1, --versionone           use version 1 of the attributes (obsolete now)\n\n",
		program_name);
}

/* 
 * substitute implementatin of JP backend 
 */

int glite_jppsbe_pread(glite_jp_context_t ctx, void *handle, void *buf, size_t nbytes, off_t offset, ssize_t *nbytes_ret) {
	FILE *f;

	f = (FILE *)handle;
	if (fseek(f, offset, SEEK_SET) != 0) {
		*nbytes_ret = 0;
		return 0;
	}
	*nbytes_ret = fread(buf, 1, nbytes, f);

	return ferror(f) ? 1 : 0;
}

/*
 * main
 */
int main(int argc, char *argv[]) 
{
	glite_jp_context_t jpctx;
	glite_jpps_fplug_data_t plugin_data;
	void *data_handle, *lib_handle;
	FILE *infile,*outfile = NULL;
	char *err;
	init_f *plugin_init;
	done_f *plugin_done;
	int opt;

	/* get arguments */
	while ((opt = getopt_long(argc,argv,
		"f:" /* input file */
		"o:" /* output file */
		"1"  /* version one of the attributes */
		"h"  /* help */
		"v"  /* verbose */
		"V",  /* version */
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); return(0);
			case 'v': verbose = 1; break;
			case 'f': infilename = optarg; break;
			case 'o': outfilename = optarg; break;
			case '1': versionone = 1; break;
			case 'h':
			default:
				usage(argv[0]); return(0);
		}
	}

	/* load L&B plugin and its 'init' symbol */
	if ((lib_handle = dlopen("glite_lb_plugin.so", RTLD_LAZY)) == NULL) {
		err = dlerror() ? :"unknown error";
		fprintf(stderr,"lb_statistics: can't load L&B plugin (%s)\n", err);
		return 1;
	} else if (verbose) fprintf(stdout,"lb_statistics: loaded L&B plugin\n");

	if ((plugin_init = dlsym(lib_handle, "init")) == NULL ||
	    (plugin_done = dlsym(lib_handle, "done")) == NULL) {
		err = dlerror() ? : "unknown error";
		fprintf(stderr,"lb_statistics: can't find symbol 'init' or 'done' (%s)\n", err);
		dlclose(lib_handle);
		return 1;
	} else if (verbose) fprintf(stdout,"lb_statistics: L&B plugin check o.k.\n");

	/* dump file with events */
	if ((infile = fopen(infilename, "rt")) == NULL) {
		fprintf(stderr,"lb_statistics: Error opening file %s: %s\n", infilename, strerror(errno));
		dlclose(lib_handle);
		return 1;
	} else if (verbose) fprintf(stdout,"lb_statistics: opened input file %s\n", infilename);

	/* output filename */
	if (outfilename) {
		if ((outfile = fopen(outfilename, "wt")) == NULL) {
			fprintf(stderr,"lb_statistics: Error opening file %s: %s\n", outfilename, strerror(errno));
			dlclose(lib_handle);
			fclose(infile);
		} else if (verbose) fprintf(stdout,"lb_statistics: opened output file %s\n", outfilename);
	} else {
		outfile = stdout;
		if (verbose) fprintf(stdout,"lb_statistics: output will go to stdout\n");
	}

	/* use the plugin */
	jpctx = calloc(1,sizeof *jpctx);
	plugin_init(jpctx, &plugin_data);

	jpctx->plugins = calloc(2,sizeof(*jpctx->plugins));
	jpctx->plugins[0] = &plugin_data;

	plugin_data.ops.open(jpctx, infile, "uri://", &data_handle);

	if (data_handle) {

		if (versionone) {
			process_attrs(jpctx, plugin_data, data_handle, outfile);
		} else {
			process_attrs2(jpctx, plugin_data, data_handle, outfile);
		}

		plugin_data.ops.close(jpctx, data_handle);
	}
	plugin_done(jpctx, &plugin_data);

	fclose(infile);
	if (verbose) fprintf(stdout,"lb_statistics: closed input file %s\n", infilename);

	if (outfile) {
		fclose(outfile);
		if (verbose) fprintf(stdout,"lb_statistics: closed output file %s\n", outfilename);
	}

	dlclose(lib_handle);
	if (verbose) fprintf(stdout,"lb_statistics: L&B plugin closed\n");

	return 0;
}

