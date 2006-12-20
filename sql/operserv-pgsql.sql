DROP TABLE akill CASCADE;
CREATE TABLE akill (
  id              SERIAL PRIMARY KEY,
  mask            VARCHAR(255) NOT NULL,
  reason          VARCHAR(255) NOT NULL,
  setter          INTEGER REFERENCES account(id),
  time            INTEGER NOT NULL,
  duration        INTEGER NOT NULL,
  UNIQUE (mask)
);

DROP TABLE account_access CASCADE;
CREATE TABLE account_access(
  id              SERIAL PRIMARY KEY,
  account_id      INTEGER NOT NULL REFERENCES account(id),
  level           INTEGER NOT NULL
);
