DROP TABLE IF EXISTS moranserv_track CASCADE;
CREATE TABLE moranserv_track (
  id      SERIAL PRIMARY KEY,
  setter  INTEGER REFERENCES account(id) ON DELETE SET NULL,
  time    INTEGER NOT NULL,
  track   VARCHAR(255) NOT NULL,
  reason  VARCHAR(255) NOT NULL
);
CREATE UNIQUE INDEX moranserv_track_idx ON moranserv_track ((lower(track)));
