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

DROP TABLE channel_access;
CREATE TABLE channel_access(
  id             SERIAL PRIMARY KEY,
  channel_id     INTEGER NOT NULL default '0',
  nick_id        INTEGER NOT NULL default '0',
  level          INTEGER NOT NULL default '0',

  FOREIGN KEY (channel_id) REFERENCES channel (id),
  FOREIGN KEY (nick_id) REFERENCES nickname (id),

  UNIQUE (channel_id, nick_id)
};
