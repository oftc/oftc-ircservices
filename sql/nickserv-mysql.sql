DROP TABLE IF EXISTS nickname;
CREATE TABLE nickname (
  id              INTEGER PRIMARY KEY auto_increment,
  nick            VARCHAR(255) NOT NULL default '',
  password        VARCHAR(255) NOT NULL default '',
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
)ENGINE=MyISAM;

DROP TABLE IF EXISTS nickname_access;
CREATE TABLE nickname_access (
  id              INTEGER PRIMARY KEY auto_increment,
  nickname_id     INTEGER NOT NULL default '0',
  entry           VARCHAR(255) NOT NULL default '',
  UNIQUE KEY id (id),
  UNIQUE KEY nickname_id (nickname_id, entry)
) TYPE=MyISAM;

DROP TABLE IF EXISTS nickname_links;
CREATE TABLE nickname_links (
  nick_id         INTEGER,
  link_id         INTEGER,
  KEY nick_id,
  KEY link_id
) TYPE=MyISAM;
