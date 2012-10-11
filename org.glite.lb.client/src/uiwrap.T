# this file is common for producer.h and uiwrap.c
# it generates either protoypes (set $PRINTPROTOTYPESONLY to 1) or implementation

for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $func = "edg_wll_Log$t";		# name of generated function
	my $funcproxy = $func . "Proxy";
	my $funcc = $func . "CODETOREPLACE";	# name of generated CODE function
	my $funccproxy = $funcc . "Proxy";
	my $funcfile = $func . "File";
	my $funccfile = $funcc . "File";
	my $p = "edg_wll_Context context"; 	# parameters of generated function
	my $pc = $p;				# parameters of generated CODE function
	my $q = "context,EDG_WLL_EVENT_$tu,EDG_WLL_FORMAT_$tu"; # parameters for LogEvent called in generated function
	my $qc = $q;				# parameters for LogEvent called in generated CODE function
	my $text = "";				# whole text for generated function
	my $textproxy = "";			# whole text for generated CODE function
	my $textfile = "";
	my $doc = " * \\param[in,out] context\tcontext to work with, \n"; # parameters description for generated function
	my $docc = $doc;			# parameters description for generated CODE function

	my $decl = "\tint ret;\n";		
	my $free = "";

	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->getName;
		my $sfn = $fn;
		my $ft = "const ";
		my $fc = $f->getComment;
		if ($f->{type} eq 'timeval') {
			$decl .= "\tchar s_$fn".'[ULM_DATE_STRING_LENGTH + 1]; '.$f->toString($fn, "s_$fn")."\n";
			$sfn = "s_$fn";
		} elsif ($f->{type} eq 'jobid' or $f->{type} eq 'notifid' or $ULMasString{$f->{type}}) {
			$decl .= "\tchar *s_$fn; ".$f->toString($fn, "s_$fn")."\n";
			$free .= "\tfree(s_$fn);\n";
			$sfn = "s_$fn";
		}
                if ($f->{codes}) {
# XXX: obsolete         $ft = "enum edg_wll\_$t" . ucfirst $fn;
                        $ft = $ft . "char *";
			$qc = $qc . ", CODETOREPLACE";
                } else {
                        $ft = $ft . $f->getType;
			$pc = $pc . ", $ft $fn";
			$qc = $qc . ", $sfn";
			$docc = $docc . " * \\param[in] $fn\t$fc\n";
                }
		$p = $p . ", $ft $fn";
		$q = $q . ", $sfn";
		$doc = $doc . " * \\param[in] $fn\t$fc\n";
	}
	$text =	qq{
/**
 * \\brief $func
 *
 * $func - simple wrapper around edg_wll_LogEvent for event $t
 *
};
	$textproxy = qq{
/**
 * \\brief $funcproxy
 *
 * $funcproxy - simple wrapper around edg_wll_LogEventProxy for event $t
 *
};
	$textfile = qq{
/**
 * \\brief $funcfile
 *
 * $funcfile - simple wrapper around edg_wll_LogEventFile for event $t
 *
};
	if ($PRINTDOC) {
		$text = $text . $doc . " * \\see edg_wll_LogEvent()\n";
		$textproxy = $textproxy . $doc . "* \\see edg_wll_LogEventProxy()\n";
		$textfile = $textfile . $doc . " *  \\see edg_wll_LogEventFile()\n";
	}
	$text = $text . " */\n";
	$textproxy = $textproxy . " */\n";
	$textfile = $textfile . " */\n";
	if ($PRINTPROTOTYPESONLY) {
		$text = $text . "\nextern int $func($p);\n";
		$textproxy = $textproxy . "\nextern int $funcproxy($p);\n";
		$textfile = $textfile . "\nextern int $funcfile($p);\n";
	} else {
		$text = $text . qq{
int $func($p)
\{
$decl
	ret = edg_wll_LogEvent($q);
$free
	return ret;
\}
};
		$textproxy = $textproxy . qq{
int $funcproxy($p)
\{
$decl
	ret = edg_wll_LogEventProxy($q);
$free
	return ret;
\}
};
		$textfile = $textfile . qq{
int $funcfile($p)
\{
$decl
	ret = edg_wll_LogEventFile($q);
$free
	return ret;
\}
};
	}
	gen "$text";
	gen "$textproxy";
	gen "$textfile";

	#
	# generate also CODE functions:
	#
	for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = $f->getName;
	if ($f->{codes}) {
	for (@{$f->{codes}}) {
		my $code = uc $_->{name};
		my $funccc = $funcc; $funccc =~ s/CODETOREPLACE/$code/g;
		my $funcccproxy = $funccproxy; $funcccproxy =~ s/CODETOREPLACE/$code/g;
		my $funcccfile = $funccfile; $funcccfile =~ s/CODETOREPLACE/$code/g;
		my $qcc = $qc; $qcc =~ s/CODETOREPLACE/"$code"/g;
		my $textc = qq{
/**
 * \\brief $funccc
 *
 * $funccc - simple wrapper around edg_wll_LogEvent for event $t, $fn $code
 *
};
		my $textcproxy = qq{
/**
 * \\brief $funcccproxy
 *
 * $funcccproxy - simple wrapper around edg_wll_LogEventProxy for event $t, $fn $code
 *
};
		my $textcfile = qq{
/**
 * \\brief $funcccfile
 *
 * $funcccfile - simple wrapper around edg_wll_LogEventFile for event $t, $fn $code
 *
};
		if ($PRINTDOC) {
			$textc = $textc . $docc . " * \\see edg_wll_LogEvent()\n";
			$textcproxy = $textcproxy . $docc . " * \\see edg_wll_LogEventProxy()\n";
			$textcfile = $textcfile . $docc . " * \\see edg_wll_LogEventFile()\n";
		}
		$textc = $textc . " */\n";
		$textcproxy = $textcproxy . " */\n";
		$textcfile = $textcfile . " */\n";
		if ($PRINTPROTOTYPESONLY) {
			$textc = $textc . "\nextern int $funccc($pc);\n";
			$textcproxy = $textcproxy . "\nextern int $funcccproxy($pc);\n";
			$textcfile = $textcfile . "\nextern int $funcccfile($pc);\n";
		} else {
			$textc = $textc . qq{
int $funccc($pc)
\{
$decl
	ret = edg_wll_LogEvent($qcc);
$free
	return ret;
\}
};
			$textcproxy = $textcproxy . qq{
int $funcccproxy($pc)
\{
$decl
	ret = edg_wll_LogEventProxy($qcc);
$free
	return ret;
\}
};
			$textcfile = $textcfile . qq{
int $funcccfile($pc)
\{
$decl
	ret = edg_wll_LogEventFile($qcc);
$free
	return ret;
\}
};
		}
		gen "$textc";
		gen "$textcproxy";
		gen "$textcfile";
	} # for codes
	} # if
	} # for getFields
}

1;
