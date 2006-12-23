DROP TABLE account CASCADE;
CREATE TABLE account (
  id                  SERIAL PRIMARY KEY,
  primary_nick        INTEGER,
  password            CHAR(40),      -- base16 encoded sha1(salt+<userpassword>).  lower case
  salt                CHAR(16),
  url                 VARCHAR(255),
  email               VARCHAR(255),
  cloak               VARCHAR(255),
  flag_enforce        BOOLEAN NOT NULL DEFAULT 'False',
  flag_secure         BOOLEAN NOT NULL DEFAULT 'False',
  flag_verified       BOOLEAN NOT NULL DEFAULT 'False',
  flag_cloak_enabled  BOOLEAN NOT NULL DEFAULT 'False',
  flag_admin          BOOLEAN NOT NULL DEFAULT 'False',
  flag_email_verified BOOLEAN NOT NULL DEFAULT 'False',
  language            INTEGER NOT NULL default '0',
  last_host           VARCHAR(255),
  last_realname       VARCHAR(255),
  last_quit_msg       VARCHAR(512),
  last_quit_time      INTEGER,
  reg_time            INTEGER NOT NULL -- The account itself
);

DROP TABLE nickname CASCADE;
CREATE TABLE nickname (
  id                  SERIAL PRIMARY KEY,
  nick                VARCHAR(255) NOT NULL,
  user_id             INTEGER REFERENCES account(id) NOT NULL,
  reg_time            INTEGER NOT NULL, -- This nickname
  last_seen           INTEGER
);

ALTER TABLE account ADD FOREIGN KEY (primary_nick) REFERENCES nickname(id) ON DELETE SET NULL;

DROP TABLE forbidden_nickname;
CREATE TABLE forbidden_nickname (
  nick                VARCHAR(255) PRIMARY KEY
);

DROP TABLE account_access;
CREATE TABLE account_access (
  id                  SERIAL PRIMARY KEY,
  parent_id           INTEGER REFERENCES account(id) ON DELETE CASCADE NOT NULL,
  entry               VARCHAR(255) NOT NULL,
  UNIQUE (id, entry)
);
