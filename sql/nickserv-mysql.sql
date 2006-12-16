DROP TABLE IF EXISTS nickname CASCADE;
DROP TABLE IF EXISTS account_access CASCADE;
DROP TABLE IF EXISTS account CASCADE;
DROP TABLE IF EXISTS forbidden_nickname;

CREATE TABLE account (
  id              INTEGER PRIMARY KEY auto_increment,
  password            CHAR(40),      -- base16 encoded sha1(salt+<userpassword>).  lower case
  salt                CHAR(16),
  url                 VARCHAR(255),
  email               VARCHAR(255),
  cloak               VARCHAR(255),
  flag_enforce        BOOLEAN NOT NULL DEFAULT FALSE,
  flag_secure         BOOLEAN NOT NULL DEFAULT FALSE,
  flag_verified       BOOLEAN NOT NULL DEFAULT FALSE,
  flag_cloak_enabled  BOOLEAN NOT NULL DEFAULT FALSE,
  flag_admin          BOOLEAN NOT NULL DEFAULT FALSE,
  flag_email_verified BOOLEAN NOT NULL DEFAULT FALSE,
  language            INTEGER NOT NULL default '0',
  last_host           VARCHAR(255),
  last_realname       VARCHAR(255),
  last_quit_msg       VARCHAR(512),
  last_quit_time      INTEGER,
  reg_time            INTEGER NOT NULL -- The account itself
) ENGINE=InnoDB;

CREATE TABLE nickname (
  nick                VARCHAR(255) NOT NULL,
  user_id             INTEGER NOT NULL,
  reg_time            INTEGER NOT NULL, -- This nickname
  last_seen           INTEGER,
  UNIQUE (nick),
  FOREIGN KEY (user_id) REFERENCES account(id)
) ENGINE=InnodB;

CREATE TABLE forbidden_nickname (
  nick                VARCHAR(255) NOT NULL PRIMARY KEY
) ENGINE=InnoDB;

CREATE TABLE account_access (
  id              INTEGER PRIMARY KEY auto_increment,
  parent_id       INTEGER NOT NULL,
  entry           VARCHAR(255) NOT NULL,
  FOREIGN KEY (parent_id) REFERENCES account(id) ON DELETE CASCADE
) ENGINE=InnoDB;
