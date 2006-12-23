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
