--
-- Inicialization (replace pgsql by actual postgres superuser):
--
-- 1) grant privileges, someting like this in $data/pg_hba.conf:
--     local all all trust
--
-- 2) create user:
--     createuser -U pgsql rtm
--
-- 3) crate database:
--     createuser -U pgsql rtm
--
-- 4) create tables:
--     psql -U rtm rtm < test.sql
--

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
	ip TEXT NOT NULL,
	branch TEXT NOT NULL,
	serv_version TEXT NOT NULL,
	monitored BOOLEAN DEFAULT FALSE,
	last_seen DATE,
	first_seen DATE,

	PRIMARY KEY(ip)
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
