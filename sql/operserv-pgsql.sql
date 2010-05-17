DROP TABLE IF EXISTS akill CASCADE;
CREATE TABLE akill (
  id              SERIAL PRIMARY KEY,
  mask            VARCHAR(255) NOT NULL,
  reason          VARCHAR(255) NOT NULL,
  setter          INTEGER REFERENCES account(id) ON DELETE SET NULL,
  time            INTEGER NOT NULL,
  duration        INTEGER NOT NULL,
  UNIQUE (mask)
);

DROP TABLE IF EXISTS sent_mail;
CREATE TABLE sent_mail (
  id              SERIAL PRIMARY KEY,
  account_id      INTEGER REFERENCES account(id) ON DELETE SET NULL,
  email           VARCHAR(255) NOT NULL,
  sent            INTEGER NOT NULL
);

DROP TABLE IF EXISTS jupes CASCADE;
CREATE TABLE jupes (
  id              SERIAL PRIMARY KEY,
  name            VARCHAR(255) NOT NULL,
  reason          VARCHAR(255) NOT NULL,
  setter          INTEGER REFERENCES account(id) ON DELETE SET NULL
);
CREATE UNIQUE INDEX jupes_name_idx ON jupes (lower(name));
