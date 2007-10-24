DROP TABLE account CASCADE;
CREATE TABLE account (
  id                  SERIAL PRIMARY KEY,
  primary_nick        INTEGER NOT NULL,
  password            CHAR(40) NOT NULL,      -- base16 encoded sha1(salt+<userpassword>).  lower case
  salt                CHAR(16) NOT NULL,
  url                 VARCHAR(255),
  email               VARCHAR(255) NOT NULL,
  cloak               VARCHAR(255),
  flag_enforce        BOOLEAN NOT NULL DEFAULT 'False',
  flag_secure         BOOLEAN NOT NULL DEFAULT 'False',
  flag_verified       BOOLEAN NOT NULL DEFAULT 'False',
  flag_cloak_enabled  BOOLEAN NOT NULL DEFAULT 'False',
  flag_admin          BOOLEAN NOT NULL DEFAULT 'False',
  flag_email_verified BOOLEAN NOT NULL DEFAULT 'False',
  flag_private        BOOLEAN NOT NULL DEFAULT 'False',
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
  account_id          INTEGER REFERENCES account(id) NOT NULL,
  reg_time            INTEGER NOT NULL, -- This nickname
  last_seen           INTEGER
);
CREATE UNIQUE INDEX nickname_nick_idx ON nickname ((lower(nick)));
-- this speeds up GET_NICK_LINKS("SELECT nick FROM nickname WHERE account_id=?d") for instance.
-- it's otherwise not often needed
CREATE INDEX nickname_account_id_idx ON nickname (account_id);

-- This needs to be here because of the table creation order and the circular
-- reference
ALTER TABLE account ADD FOREIGN KEY (primary_nick) REFERENCES nickname(id) DEFERRABLE INITIALLY DEFERRED;

DROP TABLE forbidden_nickname;
CREATE TABLE forbidden_nickname (
  nick                VARCHAR(255) PRIMARY KEY
);
-- this is not so much for performance as for unique constraint reasons:
CREATE UNIQUE INDEX forbidden_nickname_nick_idx ON forbidden_nickname ((lower(nick)));

DROP TABLE account_access;
CREATE TABLE account_access (
  id                  SERIAL PRIMARY KEY,
  account_id          INTEGER REFERENCES account(id) ON DELETE CASCADE NOT NULL,
  entry               VARCHAR(255) NOT NULL,
  UNIQUE (id, entry)
);
CREATE INDEX account_access_account_id_idx ON account_access (account_id);

DROP TABLE account_fingerprint;
CREATE TABLE account_fingerprint (
  id                  SERIAL PRIMARY KEY,
  account_id          INTEGER REFERENCES account(id) ON DELETE CASCADE NOT NULL,
  fingerprint         VARCHAR(40) NOT NULL,
  UNIQUE(account_id, fingerprint)
);
CREATE INDEX account_fingerprint_account_id_idx ON account_fingerprint (account_id);
