%types = (
	C=>{
		bool=>'"int"',
		string=>'"char *"',
		strlist=>'"char **"',
		intlist=>'"int *"',
		taglist=>'"edg_wll_TagValue *"',
		stslist=>'"struct _edg_wll_JobStat *"',
		timeval=>'"struct timeval"',
		jobid=>'"edg_wlc_JobId"',
		notifid=>'"edg_wll_NotifId"',
		logsrc=>'"edg_wll_Source"',
		port=>'"uint16_t"',
#		level=>'"enum edg_wll_Level"',
		int=>'"int"'
	},
	'C++'=>{
		string=>'"std::string"',
		timeval=>'"struct timeval"',
		jobid=>'"edg::workload::common::jobid::JobId"',
	        bool=>'"int"',
                intlist=>'"std::vector<int>"',
                strlist=>'"std::vector<std::string>"',
                taglist=>'"std::vector<std::pair<std::string>>"',
	        stslist=>'"std::vector<JobStatus>"',
	        logsrc=>'"int"',
	        port=>'"int"',
	        int=>'"int"'
	}
);

%toString = (
	C=>{
		int=>'qq{asprintf(&$dst,"%d",$src);}',
		port=>'qq{asprintf(&$dst,"%d",(int) $src);}',
		bool=>'qq{asprintf(&$dst,"%d",$src);}',
		string=>'qq{$dst = $src?strdup($src):NULL;}',
		timeval=>'qq{edg_wll_ULMTimevalToDate(($src).tv_sec,($src).tv_usec,$dst);}',
		jobid=>'qq{$dst = edg_wlc_JobIdUnparse($src);}',
		notifid=>'qq{$dst = edg_wll_NotifIdUnparse($src);}',
#		level=>'qq{$dst = edg_wll_LevelToString($src);}',
		logsrc=>'qq{$dst = edg_wll_SourceToString($src);}',
#	strlist, intlist, stslist are used only in consumer API, they don't need toString method
	}
);

%ULMasString = (
	logsrc=>1
);

%fromString = (
	C=>{
		int=>'qq{$dst = atoi($src);}',
		port=>'qq{$dst = (uint16_t) atoi($src);}',
		bool=>'qq{$dst = atoi($src);}',
		string=>'qq{$dst = strdup($src);}',
		timeval=>'qq{edg_wll_ULMDateToTimeval($src,&$dst);}',
		jobid=>'qq{edg_wlc_JobIdParse($src,&$dst);}',
		notifid=>'qq{edg_wll_NotifIdParse($src,&$dst);}',
#		level=>'qq{$dst = edg_wll_StringToLevel($src);}',
		logsrc=>'qq{$dst = edg_wll_StringToSource($src);}',
#	strlist, intlist, stslist are used only in consumer API, they don't need fromString method
	}
);

%DefaultNullValue = (
	int=>0,
	port=>0,
#	level=>'EDG_WLL_LEVEL_UNDEFINED',
	bool=>0,
	string=>'NULL',
	jobid=>'NULL',
	notifid=>'NULL',
	logsrc=>'EDG_WLL_SOURCE_NONE',
	timeval=>'null_timeval',
	strlist=>'NULL',
	intlist=>'NULL',
	taglist=>'NULL',
	stslist=>'NULL',
);

%compare = (
	C=>{
		int=>'"($a == $b)"',
		port=>'"($a == $b)"',
#		level=>'"($a == $b)"',
		bool=>'"(($a || !$b) && ($b || !$a))"',
		string=>'"(($a) == NULL && ($b) == NULL) || (($a)&&($b)&& !strcmp($a,$b))"',
		jobid=>'"(($a) == NULL && ($b) == NULL) || (($a)&&($b)&& !strcmp(edg_wlc_JobIdUnparse($a),edg_wlc_JobIdUnparse($b)))"',
		notifid=>'"($a) == ($b)"',
		logsrc=>'"($a) == ($b)"',
		timeval=>'"($a).tv_sec == ($b).tv_sec && ($a).tv_usec == ($b).tv_usec"',
	}
);

%toFormatString = (
	C=>{
		int=>'"%d"',
		port=>'"%d"',
		bool=>'"%d"',
#		level=>'"%s"',
		string=>'"%|Us"',
		jobid=>'"%s"',
		notifid=>'"%s"',
		logsrc=>'"%s"',
		timeval=>'"%s"',
	}
);
