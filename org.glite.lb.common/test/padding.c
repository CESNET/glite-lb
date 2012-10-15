#define CC /*
gcc -W -Wall -g -O2 padding.c -o padding -I../../stage/usr/include
exit $?
*/

/*
 * quick analysis of padding usage
 */

#include <stdio.h>

#include "glite/lb/connpool.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events.h"
#include "glite/lb/lb_plain_io.h"


void out(const char *name, size_t size, size_t padding) {
	printf("%s:\n", name);
	printf("	sizeof %lu	padding %lu	padno %d	use %0.02f %%\n", size, padding, size/(sizeof(void *)), (double)(size - padding)/size*100);
	printf("\n");
}


int main() {
	struct _edg_wll_ConnPool connpool;
	struct _edg_wll_ConnProxy connproxy;
	struct _edg_wll_Context context;
	union _edg_wll_Event event;
	struct _edg_wll_PlainConnection plainconnection;

	out("struct _edg_wll_ConnPool", sizeof(connpool), sizeof(connpool._padding));
	out("struct _edg_wll_ConnProxy", sizeof(connproxy),  sizeof(connproxy._padding));
	out("struct _edg_wll_Context", sizeof(context), sizeof(context._padding));
	out("union _edg_wll_Event", sizeof(event), sizeof(event._pad) - sizeof(union _edg_wll_Event_to_pad__dont_use));
	out("struct _edg_wll_PlainConnection", sizeof(plainconnection), sizeof(plainconnection._padding));

	return 0;
}
