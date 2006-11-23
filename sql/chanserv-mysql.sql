DROP TABLE IF EXISTS channel;
CREATE TABLE channel (
  id              INTEGER PRIMARY KEY auto_increment,
  channel         VARCHAR(30) NOT NULL default '',
  founder         INTEGER NOT NULL default '0',
  successor       INTEGER NOT NULL default '0',
  flags           INTEGER NOT NULL default '0',
  description     VARCHAR(255) NOT NULL default '',
  url             VARCHAR(255) NOT NULL default '',
  email           VARCHAR(255) NOT NULL default '',
  entrymsg        VARCHAR(255) NOT NULL default '',
  topic           VARCHAR(255) NOT NULL default '',
  
  UNIQUE (channel)
)ENGINE=MyISAM;

DROP TABLE IF EXISTS channel_access;
CREATE TABLE channel_access (
  id              INTEGER PRIMARY KEY auto_increment,
  channel_id      INTEGER NOT NULL default '0',
  nick_id         INTEGER NOT NULL default '0',
  level           INTEGER NOT NULL default '0',

  UNIQUE (channel_id, nick_id)
)ENGINE=MyISAM;
  
