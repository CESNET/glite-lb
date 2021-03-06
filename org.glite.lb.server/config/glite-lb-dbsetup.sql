create table jobs (
	jobid		char(32)	binary not null,
	dg_jobid	varchar(255)	binary not null,
	userid		char(32)	binary not null,
	aclid		char(32)	binary null,
	proxy		bool		not null,
	server		bool		not null,
	grey		bool		not null,
	nevents		int		not null,

	primary key (jobid),
	unique (dg_jobid),
	index (userid)
) engine=innodb;

create table users (
	userid		char(32)	binary not null,
	cert_subj	varchar(255)	binary not null,

	primary key (userid),
	unique (cert_subj)
) engine=innodb;

create table events (
	jobid		char(32)	binary not null,
	event		int		not null,
	code		int		not null,
	prog		varchar(255)	binary not null,
	host		varchar(255)	binary not null,
	time_stamp	datetime	not null,
	userid		char(32)	binary null,
	usec		int		null,
	level		int		null,

	arrived		datetime	not null,
	seqcode		varchar(255)	binary not null,

	primary key (jobid,event),
	index (time_stamp),
	index (host),
	index (arrived)
) engine=innodb;

create table events_flesh (
	jobid		char(32)	binary not null,
	event		int		not null,
	ulm		mediumblob	not null,
	
	primary key (jobid,event)
) engine=innodb;

-- for compatibility
create table short_fields (
	jobid		char(32)	binary not null,
	event		int		not null,
	name		varchar(200)	binary not null,
	value		varchar(255)	binary null,

	primary key (jobid,event,name)
) engine=innodb;

-- for compatibility
create table long_fields (
	jobid		char(32)	binary not null,
	event		int		not null,
	name		varchar(200)	binary not null,
	value		mediumblob	null,

	primary key (jobid,event,name)
) engine=innodb;

create table states (
	jobid		char(32)	binary not null,
	status		int		not null,
	seq		int		not null,
	int_status	mediumblob	not null,
	version		varchar(32)	not null,
	parent_job	varchar(32)	binary not null,

	primary key (jobid),
	index (parent_job)
	
) engine=innodb;

create table status_tags (
	jobid		char(32)	binary not null,
	seq		int		not null,
	name		varchar(200)	binary not null,
	value		varchar(255)	binary null,

	primary key (jobid,seq,name)
) engine=innodb;

create table server_state (
	prefix		varchar(100)	not null,
	name		varchar(100)	binary not null,
	value		varchar(255)	binary not null,

	primary key (prefix,name)
) engine=innodb;

create table acls (
	aclid		char(32)	binary not null,
	value		mediumblob	not null,
	refcnt		int		not null,

	primary key (aclid)
) engine=innodb;

create table notif_registrations (
	notifid		char(32)	binary not null,
	destination	varchar(200)	not null,
	valid		datetime	not null,
	userid		char(32)	binary not null,
	conditions	mediumblob	not null,
	flags		int		not null,

	`STD_owner`	varchar(200)	null,
	`STD_network_server`	varchar(200)	null,
	`JDL_VirtualOrganisation`	varchar(200)	null,

	primary key (notifid),
	index (`STD_owner`),
	index (`STD_network_server`),
	index (`JDL_VirtualOrganisation`)
) engine=innodb;

create table notif_jobs (
	notifid		char(32)	binary not null,
	jobid		char(32)	binary not null,

	primary key (notifid,jobid),
	index (jobid)
) engine=innodb;

create table zombie_jobs (
	jobid	varchar(32)	not null,
	prefix_id       tinyint unsigned not null,
	suffix_id       tinyint unsigned not null,

	primary key (jobid)
) engine=innodb;

create table zombie_prefixes (
	prefix_id       tinyint unsigned not null auto_increment,
	prefix		varchar(255)	binary not null,

	primary key (prefix_id)
) engine=innodb;

create table zombie_suffixes (
	suffix_id       tinyint unsigned not null auto_increment,
	suffix		varchar(255)	binary not null,

	primary key (suffix_id)
) engine=innodb;

create table job_connections (
	jobid_from	char(32)	binary not null,
	jobid_to	char(32)	binary not null,
	jobtype		int		not null,
	connection      int             not null,
	primary key (jobid_from, jobid_to)
) engine=innodb;

