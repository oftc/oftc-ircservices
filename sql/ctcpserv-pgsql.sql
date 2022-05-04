DROP TABLE IF EXISTS ctcpserv_bad_versions CASCADE;
CREATE TABLE ctcpserv_bad_versions (
  id      SERIAL PRIMARY KEY,
  setter  INTEGER REFERENCES account(id) ON DELETE SET NULL,
  time    INTEGER NOT NULL,
  pattern VARCHAR(1024) NOT NULL,
  reason  VARCHAR(512) NOT NULL,
  kills   INTEGER NOT NULL DEFAULT 0,
  monitor_only BOOLEAN NOT NULL DEFAULT 'False',
  active  BOOLEAN NOT NULL DEFAULT 'True'
);
CREATE UNIQUE INDEX ctcpserv_bad_versions_pattern_idx ON ctcpserv_bad_versions (pattern);
