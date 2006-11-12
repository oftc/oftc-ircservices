DROP TABLE channel;
CREATE TABLE channel(
  id              SERIAL PRIMARY KEY,
  channel         VARCHAR(30) NOT NULL default '',
  founder         INTEGER NOT NULL default 0,
  successor       INTEGER NOT NULL default 0,
  flags           INTEGER NOT NULL default 0,
  description     VARCHAR(255) NOT NULL default '',
  url             VARCHAR(255) NOT NULL default '',
  email           VARCHAR(255) NOT NULL default '',
  entrymsg        VARCHAR(255) NOT NULL default '',
  topic           VARCHAR(255) NOT NULL default '',

  FOREIGN KEY (founder) REFERENCES nickname (id),
  UNIQUE (channel)
);
