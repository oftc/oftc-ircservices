DROP TABLE nickname CASCADE;
CREATE TABLE nickname (
  id              SERIAL PRIMARY KEY,
  nick            VARCHAR(255) NOT NULL default '',
  password        VARCHAR(255) NOT NULL default '',
  salt            VARCHAR(50)  NOT NULL default '',
  url             VARCHAR(255) NOT NULL default '',
  email           VARCHAR(255) NOT NULL default '',
  cloak           VARCHAR(255) NOT NULL default '',
  last_host       VARCHAR(255) NOT NULL default '',
  last_realname   VARCHAR(255) NOT NULL default '',
  last_quit       VARCHAR(512) NOT NULL default '',
  last_quit_time  INTEGER NOT NULL default '0',
  reg_time        INTEGER NOT NULL default '0',
  last_seen       INTEGER NOT NULL default '0',
  last_used       INTEGER NOT NULL default '0',
  status          INTEGER NOT NULL default '0',
  flags           INTEGER NOT NULL default '0',
  language        INTEGER NOT NULL default '0',
  UNIQUE (nick)
);

DROP TABLE nickname_access;
CREATE TABLE nickname_access (
  id              SERIAL PRIMARY KEY,
  parent_id       INTEGER REFERENCES nickname(id),
  entry           VARCHAR(255) NOT NULL default ''
);

DROP TABLE nickname_links;
CREATE TABLE nickname_links (
 nick_id          INTEGER REFERENCES nickname(id),
 link_id          INTEGER REFERENCES nickname(id)
);
