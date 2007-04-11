DROP TABLE akill CASCADE;
CREATE TABLE akill (
  id              SERIAL PRIMARY KEY,
  mask            VARCHAR(255) NOT NULL,
  reason          VARCHAR(255) NOT NULL,
  setter          INTEGER NOT NULL REFERENCES account(id),
  time            INTEGER NOT NULL,
  duration        INTEGER NOT NULL,
  UNIQUE (mask)
);

DROP TABLE sent_mail 
CREATE TABLE sent_mail (
  id              SERIAL PRIMARY KEY,
  account_id      INTEGER NOT NULL REFERENCES account(id),
  email           VARCHAR(255) NOT NULL,
  sent            INTEGER NOT NULL
);
