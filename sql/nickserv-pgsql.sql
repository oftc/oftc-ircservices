CREATE TABLE nickname (
  id              SERIAL PRIMARY KEY,
  nick            VARCHAR(30) NOT NULL default '',
  password        VARCHAR(34) NOT NULL default '',
  url             VARCHAR(255) NOT NULL default '',
  email           VARCHAR(255) NOT NULL default '',
  cloak           VARCHAR(255) NOT NULL default '',
  last_host       VARCHAR(255) NOT NULL default '',
  last_realname   VARCHAR(255) NOT NULL default '',
  last_quit       VARCHAR(255) NOT NULL default '',
  last_quit_time  INTEGER NOT NULL default '0',
  reg_time        INTEGER NOT NULL default '0',
  last_seen       INTEGER NOT NULL default '0',
  last_used       INTEGER NOT NULL default '0',
  status          INTEGER NOT NULL default '0',
  flags           INTEGER NOT NULL default '0',
  language        INTEGER NOT NULL default '0',
  UNIQUE (nick)
);
