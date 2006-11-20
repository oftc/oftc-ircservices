DROP TABLE akill CASCADE;
CREATE TABLE akill (
  id              SERIAL PRIMARY KEY,
  mask            VARCHAR(255) NOT NULL DEFAULT '',
  reason          VARCHAR(255) NOT NULL DEFAULT '',
  setter          INTEGER REFERENCES nickname(id),
  time            INTEGER NOT NULL DEFAULT 0,
  duration        INTEGER NOT NULL DEFAULT 0,
  UNIQUE (mask)
);
