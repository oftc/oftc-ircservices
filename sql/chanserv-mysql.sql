DROP TABLE IF EXISTS channel_access;
DROP TABLE IF EXISTS channel_akick;
DROP TABLE IF EXISTS channel;
CREATE TABLE channel (
  id                      INTEGER PRIMARY KEY auto_increment,
  channel                 VARCHAR(255) NOT NULL,
  flag_forbidden          BOOLEAN NOT NULL DEFAULT FALSE, -- channel is forbidden.  it may not be used
  flag_private            BOOLEAN NOT NULL DEFAULT FALSE, -- do not show up in list of channels
  flag_restricted         BOOLEAN NOT NULL DEFAULT FALSE, -- only people on the access list can hold channel operator status
  flag_topic_lock         BOOLEAN NOT NULL DEFAULT FALSE, -- topics can only be changed via chanserv
  flag_verbose            BOOLEAN NOT NULL DEFAULT FALSE, -- notice all chanserv actions to the channel
  flag_autolimit          BOOLEAN NOT NULL DEFAULT FALSE, -- sets limit just above the current usercount
  flag_expirebans         BOOLEAN NOT NULL DEFAULT FALSE, -- expire bans that are old
  description             VARCHAR(255) NOT NULL,
  url                     VARCHAR(255) NOT NULL,
  email                   VARCHAR(255) NOT NULL,
  entrymsg                VARCHAR(255) NOT NULL,
  topic                   VARCHAR(255) NOT NULL,
  mlock                   VARCHAR(255) NOT NULL,
  UNIQUE (channel)
)ENGINE=InnoDB;

CREATE TABLE channel_access (
  id                      INTEGER PRIMARY KEY auto_increment,
  channel_id              INTEGER NOT NULL,
  account_id              INTEGER NOT NULL,
  level                   INTEGER NOT NULL,
  FOREIGN KEY (channel_id)REFERENCES channel(id) ON DELETE CASCADE,
  FOREIGN KEY (account_id)REFERENCES account(id),
  UNIQUE (channel_id, account_id)
)ENGINE=InnoDB;

CREATE TABLE channel_akick(
  id                      SERIAL PRIMARY KEY,
  channel_id              INTEGER NOT NULL,
  target                  INTEGER, -- If a nickname akick
  setter                  INTEGER,
  mask                    VARCHAR(255), -- If a mask akick
  reason                  VARCHAR(512),
  time                    INTEGER NOT NULL,
  duration                INTEGER NOT NULL,
  FOREIGN KEY (channel_id)REFERENCES channel(id),
  FOREIGN KEY (target)    REFERENCES account(id),
  FOREIGN KEY (setter)    REFERENCES account(id) ON DELETE SET NULL
);
