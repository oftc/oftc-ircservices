DROP TABLE IF EXISTS nickname;
CREATE TABLE nickname (
  id              INTEGER PRIMARY KEY auto_increment,
  nick            VARCHAR(255) NOT NULL default '',
  password        VARCHAR(255) NOT NULL default '',
  salt            VARCHAR(50)  NOT NULL default '',
  url             VARCHAR(255),
  email           VARCHAR(255),
  cloak           VARCHAR(255),
  last_host       VARCHAR(255) NOT NULL default '',
  last_realname   VARCHAR(255) NOT NULL default '',
  last_quit       VARCHAR(512) NOT NULL default '',
  last_quit_time  INTEGER NOT NULL default '0',
  reg_time        INTEGER NOT NULL default '0',
  last_seen       INTEGER NOT NULL default '0',
  last_used       INTEGER NOT NULL default '0',
  flags           INTEGER NOT NULL default '0',
  language        INTEGER NOT NULL default '0',
  link            INTEGER NOT NULL default '0',
  KEY link (link),
  UNIQUE (nick)
)ENGINE=MyISAM;

DROP TABLE IF EXISTS nickname_access;
CREATE TABLE nickname_access (
  id              INTEGER PRIMARY KEY auto_increment,
  parent_id     INTEGER NOT NULL default '0',
  entry           VARCHAR(255) NOT NULL default '',
  UNIQUE KEY id (id),
  UNIQUE KEY parent_id (parent_id, entry)
) TYPE=MyISAM;
