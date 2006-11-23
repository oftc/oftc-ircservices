DROP TABLE IF EXISTS akill;
CREATE TABLE akill (
  id              INTEGER NOT NULL auto_increment,
  mask            VARCHAR(255) NOT NULL DEFAULT '',
  reason          VARCHAR(255) NOT NULL DEFAULT '',
  setter          INTEGER NOT NULL DEFAULT 0,
  time            INTEGER NOT NULL DEFAULT 0,
  duration        INTEGER NOT NULL DEFAULT 0,
  UNIQUE (mask)
)ENGINE=MyISAM;
