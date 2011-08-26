/*
@@@AUTO
*/
@@@LANG: C

#include <assert.h>
#include <string.h>
#include <errno.h>

#include "glite/lbu/trio.h"
#include "context-int.h"
#include "events.h"
#include "ulm_parse.h"


@@@{
	gen "#define MAX_COUNT ".(getMaxSize $event + 1)."\n"; # max fields count plus type field
@@@}


typedef struct json_item_s {
	size_t len;
	char *s;
} json_item_t;

typedef struct {
	json_item_t items[MAX_COUNT];
	size_t len, count;
} json_t;


static const struct timeval null_timeval = {0,0};


static int json_add_raw(json_t *json, char *s);
static int json_add_rawconst(json_t *json, const char *value);
static int json_add_string(json_t *json, const char *key, char *value);


int edg_wll_UnparseEventJSON(edg_wll_Context ctx, edg_wll_Event *event, char **result) {
	char *out, *tmp_out, *str = NULL;
	char ulm_time[ULM_DATE_STRING_LENGTH + 1];
	json_t json;
	size_t len, i;

	memset(&json, 0, sizeof json);

	if (!event) {
		*result = NULL;
		return 0;
	}
	if (event->type == EDG_WLL_EVENT_UNDEF) {
		*result = strdup("{}");
		if (*result) return 0;
		else return edg_wll_SetError(ctx, ENOMEM, NULL);
	}

	//
	// common fields
	//

	str = edg_wll_EventToString(event->type);
	if (!str || json_add_string(&json, "type", str)) goto err;
	free(str);
@@@{
	my ($t, $Tname, $tname, $tstruct, $source);
	my ($f, $fname, $Fname);
	my ($noNull, $indent);

	for $t ('_common_', sort { $event->{order}->{$a} <=> $event->{order}->{$b} } $event->getTypes) {
		selectType $event $t;
		$Tname = $t eq '_common_' ? '' : $t;
		$tname = lcfirst $t;
		$TNAME = uc $t;
		if ($tname =~ m/^pBS/) { $tname = ucfirst $tname; }
		if ($tname =~ m/^cREAM/) { $tname = ucfirst $tname; }
		if ($tname =~ m/^condor/) { $tname = ucfirst $tname; }
		$tstruct = $t eq '_common_' ? '->any' : "->$tname";

		if ($t ne '_common_') {
			gen "	case EDG_WLL_EVENT_$TNAME:\n";
		}

		for ($event->getFieldsOrdered) {
			$f = selectField $event $_;
			$fname = $f->{name};
			$Fname = ucfirst $f->{name};
			$source = "event$tstruct.$fname";

			$noNull = 0;
			$indent = $t eq '_common_' ? "" : "\t";
			if (not exists $f->{codes} and (
			    $f->{type} eq 'int'
			 or $f->{type} eq 'float'
			 or $f->{type} eq 'double'
			 or $f->{type} eq 'bool'
			 or $f->{type} eq 'string'
			)) {
				$noNull = 1;
			} else {
				gen "$indent	if (";
				gen isNULL $f "$source";
				gen ") {\n";
				gen "$indent		if (json_add_rawconst(&json, \"$fname: null\")) goto err;\n";
				gen "$indent	} else {\n";
				$indent = "$indent\t";
			}

			if ($f->{codes}) {
				gen "$indent	str = edg_wll\_".($t eq '_common_' ? '' : $t)."${Fname}ToString($source);\n";
				gen "$indent	if (!str || json_add_string(&json, \"$fname\", str)) goto err;\n";
				gen "$indent	free(str);\n";
			}
			elsif ($f->{type} eq 'bool') {
				gen "$indent	str = $source ? \"true\" : \"false\";";
				gen "$indent	if (asprintf(&str, \"$fname: %s\", str) == -1 || json_add_raw(&json, str)) goto err;\n";
			}
			elsif ($f->{type} eq 'int'
			    or $f->{type} eq 'float'
			    or $f->{type} eq 'double'
			    or $f->{type} eq 'port'
			) {
				gen "$indent	if (asprintf(&str, \"$fname: ".(toFormatString $f)."\", $source) == -1 || json_add_raw(&json, str)) goto err;\n";
			}
			elsif ($f->{type} eq 'string') {
				gen "$indent	if (json_add_string(&json, \"$fname\", $source)) goto err;\n";
			}
			elsif ($f->{type} eq 'timeval') {
				gen "$indent	".(toString $f "$source", "ulm_time")."\n";
				gen "$indent	if (json_add_string(&json, \"$fname\", ulm_time)) goto err;\n";
			}
			else {
				gen "$indent	".(toString $f "$source", "str")."\n";
				gen "$indent	if (!str || json_add_string(&json, \"$fname\", str)) goto err;\n";
				gen "$indent	free(str);\n";
			}
			if (not $noNull) {
				$indent = $t eq '_common_' ? "" : "\t";
				gen "$indent	}\n";
			}
		}
		if ($t eq '_common_') {
			gen "\n";
			gen "	//\n";
			gen "	// event specific fields\n";
			gen "	//\n";
			gen "\n";
			gen "	switch (event->type) {\n";
			gen "\n";
		} else {
			gen "		break;\n";
			gen "\n";
		}
	}
	gen "	default:\n";
	gen "		break;\n";
	gen "\n";
	gen "	}\n";
@@@}

	//
	// generate result JSON string
	//

	len = 3               // '{', new line, '}'
	    + 4 * json.count  // space, space, PAIR, comma, new line
	    + json.len;

	if ((out = malloc(len + 1)) == NULL) goto err;
	sprintf(out, "{\n");
	tmp_out = out + 2;
	for (i = 0; i < json.count; i++) {
		sprintf(tmp_out, "  %s%s\n", json.items[i].s, (i + 1 < json.count) ? "," : "");
		tmp_out += (4 + json.items[i].len);
		free(json.items[i].s);
	
	}
	//assert((tmp_out - out) == (json.len + 4 * json.count + 2));
	sprintf(tmp_out - 1, "}");
	*result = out;

	return 0;

err:
	free(str);
	for (i = 0; i < json.count; i++) free(json.items[i].s);
	*result = NULL;
	return edg_wll_SetError(ctx, ENOMEM, NULL);
}


static int json_add_raw(json_t *json, char *s) {
	json_item_t *item;

	item = &json->items[json->count++];

	assert(json->count <= MAX_COUNT);

	item->s = s;
	item->len = strlen(s);

	json->len += item->len;

	return 0;
}


static int json_add_rawconst(json_t *json, const char *value) {
	char *s;

	if ((s = strdup(value)) == NULL) return ENOMEM;
	return json_add_raw(json, s);
}

static int json_add_string(json_t *json, const char *key, char *value) {
	char *s = NULL;
	int ret;

	if (value) ret = trio_asprintf(&s, "%s: \"%|Js\"", key, value);
	else ret = asprintf(&s, "%s: null", key);
	return s && ret != -1 ? json_add_raw(json, s) : -1;
}
