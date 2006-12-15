DROP TABLE IF EXISTS channel_access;
DROP TABLE IF EXISTS channel;
CREATE TABLE channel (
  id                      INTEGER PRIMARY KEY auto_increment,
  channel                 VARCHAR(255) NOT NULL,
  founder                 INTEGER NOT NULL,
  successor               INTEGER NOT NULL,
  flag_forbidden          BOOLEAN NOT NULL DEFAULT FALSE, -- channel is forbidden.  it may not be used
  flag_private            BOOLEAN NOT NULL DEFAULT FALSE, -- do not show up in list of channels
  flag_restricted_ops     BOOLEAN NOT NULL DEFAULT FALSE, -- only people on the access list can hold channel operator status
  flag_topic_lock         BOOLEAN NOT NULL DEFAULT FALSE, -- topics can only be changed via chanserv
  flag_secure             BOOLEAN NOT NULL DEFAULT FALSE, -- only people who have identified with their password (not by access list) can use their privileged channel access
  flag_verbose            BOOLEAN NOT NULL DEFAULT FALSE, -- notice all chanserv actions to the channel
  description             VARCHAR(255) NOT NULL,
  url                     VARCHAR(255) NOT NULL,
  email                   VARCHAR(255) NOT NULL,
  entrymsg                VARCHAR(255) NOT NULL,
  topic                   VARCHAR(255) NOT NULL,
  FOREIGN KEY (founder)   REFERENCES account(id),
  FOREIGN KEY (successor) REFERENCES account(id),
  UNIQUE (channel)
)ENGINE=InnoDB;

CREATE TABLE channel_access (
  id              INTEGER PRIMARY KEY auto_increment,
  channel_id      INTEGER NOT NULL,
  nick_id         INTEGER NOT NULL,
  level           INTEGER NOT NULL,
  FOREIGN KEY (channel_id) REFERENCES channel(id),
  FOREIGN KEY (nick_id) REFERENCES account(id),
  UNIQUE (channel_id, nick_id)
)ENGINE=InnoDB;
  
