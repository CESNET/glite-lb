/*
 * load and test L&B plugin
 *
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

static const char rcsid[] = "@(#)$$";
static int verbose = 0;
static char *file = NULL;
static int jdl = 0;

static struct option const long_options[] = {
	{ "file", required_argument, 0, 'f' },
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
                "- reads a dump file (one job only) \n"
                "- and outputs an XML with statistics to stdout \n\n"
                "Usage: %s [option]\n"
                "-h, --help                 display this help and exit\n"
                "-V, --version              output version information and exit\n"
                "-v, --verbose              print extensive debug output to stderr\n"
                "-f, --file <file>          dump file to process\n"
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
	FILE *f;
	glite_jp_attrval_t *attrval;
	char *err;
	init_f *plugin_init;
	done_f *plugin_done;
	int i,opt;

	/* get arguments */
	while ((opt = getopt_long(argc,argv,
		"f:" /* file */
		"j"  /* jdl */
		"h"  /* help */
		"v"  /* verbose */
		"V",  /* version */
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); return(0);
			case 'v': verbose = 1; break;
			case 'f': file = optarg; break;
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
		goto err;
	}

	/* dump file with events */
	if ((f = fopen(file, "rt")) == NULL) {
		fprintf(stderr,"lb_statistics: Error: %s\n", strerror(errno));
		goto err;
	}

	/* use the plugin */
	plugin_init(jpctx, &plugin_data);
	plugin_data.ops.open(jpctx, f, "uri://", &data_handle);

	if (data_handle) {
		/* header */
		fprintf(stdout,"<?xml version=\"1.0\"?>\n\n");
		fprintf(stdout,"<lbd:jobRecord\n");
		fprintf(stdout,"\txmlns:lbd=\"http://glite.org/wsdl/types/lbdump\"\n");
		
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobId, &attrval);
		if (attrval) {
			fprintf(stdout,"\tjobid=\"%s\"\n", attrval->value);
			free_attrs(attrval);
		} else {
			fprintf(stdout,"\tjobid=\"default\"\n");
		}
		fprintf(stdout,">\n");
		/* /header */
		
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_user, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<user>%s</user>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_VO, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<VO>%s</VO>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_aTag, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<aTag>%s</aTag>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_rQType, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<rQType>%s</rQType>\n", ctime(&attrval->timestamp));
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eDuration, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<eDuration>%s</eDuration>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eNodes, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<eNodes>%s</eNodes>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eProc, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<eProc>%s</eProc>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_RB, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<RB>%s</RB>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CE, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<CE>%s</CE>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_host, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<host>%s</host>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_UIHost, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<UIHost>%s</UIHost>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CPUTime, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<CPUTime>%s</CPUTime>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_NProc, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<NProc>%s</NProc>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatus, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<finalStatus>%s</finalStatus>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusDate, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<finalStatusDate>%s</finalStatusDate>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusReason, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<finalStatusReason>%s</finalStatusReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSDoneStatus, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<LRMSDoneStatus>%s</LRMSDoneStatus>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSStatusReason, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<LRMSStatusReason>%s</LRMSStatusReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_retryCount, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<retryCount>%s</retryCount>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_additionalReason, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<additionalReason>%s</additionalReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobType, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<jobType>%s</jobType>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_nsubjobs, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<nsubjobs>%s</nsubjobs>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_lastStatusHistory, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<lastStatusHistory>\n");
/* 
			for (i = 1; i < EDG_WLL_NUMBER_OF_STATCODES; i++) {
				char *stat = edg_wll_StatToString(i);
				fprintf(stdout,"\t<status>\n");
				fprintf(stdout,"\t\t<status>%s</status>\n", stat);
				fprintf(stdout,"\t\t<timestamp>%ld.%06ld</timestamp>\n", attrval[i].timestamp,0);
				fprintf(stdout,"\t\t<reason>%s</reason>\n", attrval[i].value ? attrval[i].value : "");
				fprintf(stdout,"\t</status>\n");
				if (stat) free(stat);
			}
*/
                        i = 1;
                        while (attrval[i].name) {
                                fprintf(stdout,"\t<status>\n");
                                fprintf(stdout,"\t\t<status>%s</status>\n", attrval[i].name);
                                fprintf(stdout,"\t\t<timestamp>%ld.%06ld</timestamp>\n", attrval[i].timestamp,0);
                                fprintf(stdout,"\t\t<reason>%s</reason>\n", attrval[i].value ? attrval[i].value : "");
                                fprintf(stdout,"\t</status>\n");
                                i++;
                        }
			fprintf(stdout,"\t</lastStatusHistory>\n");
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_fullStatusHistory, &attrval);
		if (attrval) {
			fprintf(stdout,"\t<fullStatusHistory>\n");
			i = 1;
			while (attrval[i].name) {
				fprintf(stdout,"\t<status>\n");
				fprintf(stdout,"\t\t<status>%s</status>\n", attrval[i].name);
				fprintf(stdout,"\t\t<timestamp>%ld.%06ld</timestamp>\n", attrval[i].timestamp,0);
				fprintf(stdout,"\t\t<reason>%s</reason>\n", attrval[i].value ? attrval[i].value : "");
				fprintf(stdout,"\t</status>\n");
				i++;
			}
			fprintf(stdout,"\t</fullStatusHistory>\n");
			free_attrs(attrval);
		}
		if (jdl) {
			plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_JDL, &attrval);
			if (attrval) {
				fprintf(stdout,"\t<JDL>%s</JDL>\n", attrval->value);
				free_attrs(attrval);
			}
		}

		fprintf(stdout,"</lbd:jobRecord>\n");

		plugin_data.ops.close(jpctx, data_handle);
	}
	plugin_done(jpctx, &plugin_data);

	fclose(f);
	dlclose(lib_handle);
	return 0;

err:
	dlclose(lib_handle);
	return 1;
}
