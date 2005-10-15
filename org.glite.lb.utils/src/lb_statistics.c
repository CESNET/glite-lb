/*
 * load and test L&B plugin
 *
 * (requires -rdynamic to use fake JP backend symbols)
 */

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>
#include <malloc.h>
#include <unistd.h>
#include <getopt.h>

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

static struct option const long_options[] = {
	{ "file", required_argument, 0, 'f' },
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
                "-v, --verbose              print extensive debug output\n"
                "-f, --file <file>          port to listen\n\n",
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
	printf("JP backend error %d: %s\n", jperror->code, jperror->desc);
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
	int opt;

	/* get arguments */
	while ((opt = getopt_long(argc,argv,
		"f:" /* file */
		"h"  /* help */
		"v"  /* verbose */
		"V",  /* version */
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); return(0);
			case 'v': verbose = 1; break;
			case 'f': file = optarg; break;
			case 'h':
			default:
				usage(argv[0]); return(0);
		}
	}

	/* load L&B plugin and its 'init' symbol */
	if ((lib_handle = dlopen("glite_lb_plugin.so", RTLD_LAZY)) == NULL) {
		err = dlerror() ? :"unknown error";
		printf("can't load L&B plugin (%s)\n", err);
		return 1;
	}
	if ((plugin_init = dlsym(lib_handle, "init")) == NULL ||
	    (plugin_done = dlsym(lib_handle, "done")) == NULL) {
		err = dlerror() ? :"unknown error";
		printf("can't find symbol 'init' or 'done' (%s)\n", err);
		goto err;
	}

	/* dump file with events */
	if ((f = fopen(file, "rt")) == NULL) {
		printf("Error: %s\n", strerror(errno));
		goto err;
	}

	/* use the plugin */
	plugin_init(jpctx, &plugin_data);
	plugin_data.ops.open(jpctx, f, "uri://", &data_handle);

	if (data_handle) {
		/* header */
		printf("<?xml version=\"1.0\"?>");
		printf("<lbd:jobRecord");
		printf("\txmlns:lbd=\"http://glite.org/wsdl/types/lbdump\"\n");
		
		/* jobid */
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_ATTR_JOBID, &attrval);
		if (attrval) {
			printf("\tjobid=\"%s\"\n>\n", attrval->value);
			free_attrs(attrval);
		} else {
			printf("\tjobid=\"default\"\n>\n");
		}
		printf(">\n");
		
		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_ATTR_OWNER, &attrval);
		if (attrval) {
			printf("\t<user>%s</user>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_aTag, &attrval);
		if (attrval) {
			printf("\t<aTag>%s</aTag>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_rQType, &attrval);
		if (attrval) {
			printf("\t<rQType>%s</rQType>\n", ctime(&attrval->timestamp));
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eDuration, &attrval);
		if (attrval) {
			printf("\t<eDuration>%s</eDuration>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eNodes, &attrval);
		if (attrval) {
			printf("\t<eNodes>%s</eNodes>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eProc, &attrval);
		if (attrval) {
			printf("\t<eProc>%s</eProc>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_RB, &attrval);
		if (attrval) {
			printf("\t<RB>%s</RB>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CE, &attrval);
		if (attrval) {
			printf("\t<CE>%s</CE>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_host, &attrval);
		if (attrval) {
			printf("\t<host>%s</host>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_UIHost, &attrval);
		if (attrval) {
			printf("\t<UIHost>%s</UIHost>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CPUTime, &attrval);
		if (attrval) {
			printf("\t<CPUTime>%s</CPUTime>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_NProc, &attrval);
		if (attrval) {
			printf("\t<NProc>%s</NProc>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatus, &attrval);
		if (attrval) {
			printf("\t<finalStatus>%s</finalStatus>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusDate, &attrval);
		if (attrval) {
			printf("\t<finalStatusDate>%s</finalStatusDate>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusReason, &attrval);
		if (attrval) {
			printf("\t<finalStatusReason>%s</finalStatusReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSDoneStatus, &attrval);
		if (attrval) {
			printf("\t<LRMSDoneStatus>%s</LRMSDoneStatus>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSStatusReason, &attrval);
		if (attrval) {
			printf("\t<LRMSStatusReason>%s</LRMSStatusReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_retryCount, &attrval);
		if (attrval) {
			printf("\t<retryCount>%s</retryCount>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_additionalReason, &attrval);
		if (attrval) {
			printf("\t<additionalReason>%s</additionalReason>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobType, &attrval);
		if (attrval) {
			printf("\t<jobType>%s</jobType>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_nsubjobs, &attrval);
		if (attrval) {
			printf("\t<nsubjobs>%s</nsubjobs>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_lastStatusHistory, &attrval);
		if (attrval) {
			printf("\t<lastStatusHistory>%s</lastStatusHistory>\n", attrval->value);
			free_attrs(attrval);
		}

		plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_fullStatusHistory, &attrval);
		if (attrval) {
			printf("\t<fullStatusHistory>%s</fullStatusHistory>\n", attrval->value);
			free_attrs(attrval);
		}

		printf("</lbd:jobRecord>\n");

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
