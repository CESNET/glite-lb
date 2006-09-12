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

#include "glite/lb/context.h"
#include "glite/lb/jobstat.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/backend.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/known_attr.h"
#include "glite/jp/attr.h"
#include "glite/lb/jp_job_attrs.h"


typedef int init_f(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data);
typedef void done_f(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data);

static const char rcsid[] = "@(#)$Id$";
static int verbose = 0;
static char *infilename = NULL;
static char *outfilename = NULL;
static int jdl = 0;

static struct option const long_options[] = {
	{ "file", required_argument, 0, 'f' },
	{ "outfile", required_argument, 0, 'o' },
	{ "jdl", no_argument, 0, 'j' },
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
		"-j, --jdl                  prit also JDL in the XML\n\n",
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


int glite_jp_stack_error(glite_jp_context_t ctx, const glite_jp_error_t *jperror) {
	if (verbose) fprintf(stderr,"lb_statistics: JP backend error %d: %s\n", jperror->code, jperror->desc);
	return 0;
}


int glite_jp_clear_error(glite_jp_context_t ctx) {
	return 0;
}


/*
 * free the array of JP attr
 */
static void free_attrs(glite_jp_attrval_t *av) {
	glite_jp_attrval_t *item;

	item = av;
	while (item->name) {
		glite_jp_attrval_free(item++, 0);
	}
	free(av);
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
	glite_jp_attrval_t *attrval;
	char *err;
	init_f *plugin_init;
	done_f *plugin_done;
	int opt;

	/* get arguments */
	while ((opt = getopt_long(argc,argv,
		"f:" /* input file */
		"o:" /* output file */
		"j"  /* jdl */
		"h"  /* help */
		"v"  /* verbose */
		"V",  /* version */
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); return(0);
			case 'v': verbose = 1; break;
			case 'f': infilename = optarg; break;
			case 'o': outfilename = optarg; break;
			case 'j': jdl = 1; break;
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
	}
	if ((plugin_init = dlsym(lib_handle, "init")) == NULL ||
	    (plugin_done = dlsym(lib_handle, "done")) == NULL) {
		err = dlerror() ? : "unknown error";
		fprintf(stderr,"lb_statistics: can't find symbol 'init' or 'done' (%s)\n", err);
		dlclose(lib_handle);
		return 1;
	}

	/* dump file with events */
	if ((infile = fopen(infilename, "rt")) == NULL) {
		fprintf(stderr,"lb_statistics: Error opening file %s: %s\n", infilename, strerror(errno));
		dlclose(lib_handle);
		return 1;
	}

	/* output filename */
	if (outfilename) {
		if ((outfile = fopen(outfilename, "wt")) == NULL) {
			fprintf(stderr,"lb_statistics: Error opening file %s: %s\n", outfilename, strerror(errno));
			dlclose(lib_handle);
			fclose(infile);
		}
	} else {
		outfile = stdout;
	}

	/* use the plugin */
	plugin_init(jpctx, &plugin_data);
	plugin_data.ops.open(jpctx, infile, "uri://", &data_handle);

	if (data_handle) {
		/* <header> */
		fprintf(outfile,"<?xml version=\"1.0\"?>\n\n");
		fprintf(outfile,"<lbd:jobRecord\n");
		fprintf(outfile,"\txmlns:lbd=\"http://glite.org/wsdl/types/lbdump\"\n");
		
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobId, &attrval);
		if (attrval) {
			fprintf(outfile,"\tjobid=\"%s\"\n", attrval->value);
			free_attrs(attrval);
		} else {
			fprintf(outfile,"\tjobid=\"default\"\n");
		}
		fprintf(outfile,">\n");
		/* </header> */
		
		/* <body> */
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_user, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<user>%s</user>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_parent, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<parent>%s</parent>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_VO, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<VO>%s</VO>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_aTag, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<aTag>%s</aTag>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_rQType, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<rQType>%s</rQType>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eDuration, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<eDuration>%s</eDuration>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eNodes, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<eNodes>%s</eNodes>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eProc, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<eProc>%s</eProc>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_RB, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<RB>%s</RB>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CE, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<CE>%s</CE>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_host, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<host>%s</host>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_UIHost, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<UIHost>%s</UIHost>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CPUTime, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<CPUTime>%s</CPUTime>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_NProc, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<NProc>%s</NProc>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatus, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<finalStatus>%s</finalStatus>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusDate, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<finalStatusDate>%s</finalStatusDate>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusReason, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<finalStatusReason>%s</finalStatusReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSDoneStatus, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<LRMSDoneStatus>%s</LRMSDoneStatus>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSStatusReason, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<LRMSStatusReason>%s</LRMSStatusReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_retryCount, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<retryCount>%s</retryCount>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_additionalReason, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<additionalReason>%s</additionalReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobType, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<jobType>%s</jobType>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_nsubjobs, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<nsubjobs>%s</nsubjobs>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_subjobs, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<subjobs>\n%s\t</subjobs>\n", attrval->value);
			free_attrs(attrval);
		}

/* FIXME: */
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_lastStatusHistory, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<lastStatusHistory>\n%s\t</lastStatusHistory>\n",attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_fullStatusHistory, &attrval);
		if (attrval) {
			fprintf(outfile,"\t<fullStatusHistory>\n%s\t</fullStatusHistory>\n",attrval->value);
			free_attrs(attrval);
		}

		if (jdl) {
			plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_JDL, &attrval);
			if (attrval) {
				fprintf(outfile,"\t<JDL>%s</JDL>\n", attrval->value);
				free_attrs(attrval);
			}
		}

		fprintf(outfile,"</lbd:jobRecord>\n");

		/* </body> */
		plugin_data.ops.close(jpctx, data_handle);
	}
	plugin_done(jpctx, &plugin_data);

	fclose(infile);
	if (outfile) fclose(outfile);
	dlclose(lib_handle);
	return 0;
}
