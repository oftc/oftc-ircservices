DROP TABLE channel;
CREATE TABLE channel(
  id              SERIAL PRIMARY KEY,
  channel         VARCHAR(30) NOT NULL default '',
  founder         INTEGER NOT NULL default 0,
  FOREIGN KEY (founder) REFERENCES nickname (id),
  UNIQUE (channel)
);
