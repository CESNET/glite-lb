CREATE TABLE "jobs" (
	jobid VARCHAR PRIMARY KEY,
	lb VARCHAR,
	ce VARCHAR,
	queue VARCHAR,
	rb VARCHAR,
	ui VARCHAR,
	state VARCHAR,
	state_entered TIMESTAMP,
	rtm_timestamp TIMESTAMP,
	active BOOLEAN,
	state_changed BOOLEAN,
	registered TIMESTAMP,
	vo VARCHAR
);

CREATE TABLE "lb20" (
	lb VARCHAR,
	port INTEGER,

	PRIMARY KEY(lb, port)
);

CREATE TABLE "notifs" (
	lb VARCHAR,
	port INTEGER,
	notifid VARCHAR,
	notiftype VARCHAR,
	valid TIMESTAMP,
	refresh TIMESTAMP,
	last_update TIMESTAMP,
	errors INTEGER,

	PRIMARY KEY(lb, port, notiftype)
);
