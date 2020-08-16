DROP TABLE IF EXISTS ganneffserv CASCADE;
CREATE TABLE ganneffserv (
  id      SERIAL PRIMARY KEY,
  setter  INTEGER REFERENCES account(id) ON DELETE SET NULL,
  time    INTEGER NOT NULL,
  channel VARCHAR(255) NOT NULL,
  reason  VARCHAR(255) NOT NULL,
  kills   INTEGER NOT NULL DEFAULT 0,
  monitor_only BOOLEAN NOT NULL DEFAULT 'False'
);
CREATE UNIQUE INDEX ganneffserv_channel_idx ON ganneffserv (irc_lower(channel));
