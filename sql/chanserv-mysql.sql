DROP TABLE IF EXISTS channel;
CREATE TABLE channel (
  id              INTEGER PRIMARY KEY auto_increment,
  channel         VARCHAR(30) NOT NULL default '',
  founder         INTEGER NOT NULL default '0',
  successor       INTEGER NOT NULL default '0',
  
  UNIQUE (channel)
)ENGINE=MyISAM;
