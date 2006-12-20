DROP TABLE IF EXISTS akill;
CREATE TABLE akill (
  id              INTEGER auto_increment,
  mask            VARCHAR(255) NOT NULL,
  reason          VARCHAR(255) NOT NULL,
  setter          INTEGER NOT ON DELETE SET NULL,
  time            INTEGER NOT NULL,
  duration        INTEGER NOT NULL,
  FOREIGN KEY setter REFERENCES account(id),
  UNIQUE (mask)
) ENGINE=InnoDB;

DROP TABLE IF EXISTS account_access;
CREATE TABLE account_access(
  id                   PRIMARY KEY auto_increment,
  account_id           INTEGER NOT NULL,
  level                INTEGER NOT NULL,
  FOREIGN KEY (account_id) REFERENCES account(id)
) ENGINE=InnoDB;
