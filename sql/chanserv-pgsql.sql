DROP TABLE channel CASCADE;
CREATE TABLE channel(
  id                    SERIAL PRIMARY KEY,
  channel               VARCHAR(255) NOT NULL,
  founder               INTEGER NOT NULL REFERENCES account(id),
  successor             INTEGER REFERENCES account(id),
  flag_forbidden        BOOLEAN NOT NULL DEFAULT 'False', -- channel is forbidden.  it may not be used
  flag_private          BOOLEAN NOT NULL DEFAULT 'False', -- do not show up in list of channels
  flag_restricted_ops   BOOLEAN NOT NULL DEFAULT 'False', -- only people on the access list can hold channel operator status
  flag_topic_lock       BOOLEAN NOT NULL DEFAULT 'False', -- topics can only be changed via chanserv
  flag_secure           BOOLEAN NOT NULL DEFAULT 'False', -- only people who have identified with their password (not by access list) can use their privileged channel access
  flag_verbose          BOOLEAN NOT NULL DEFAULT 'False', -- notice all chanserv actions to the channel
  description           VARCHAR(255),
  url                   VARCHAR(255),
  email                 VARCHAR(255),
  entrymsg              VARCHAR(255),
  topic                 VARCHAR(512),
  UNIQUE (channel)
);

DROP TABLE channel_access;
CREATE TABLE channel_access(
  id                   SERIAL PRIMARY KEY,
  channel_id           INTEGER NOT NULL REFERENCES channel(id),
  nick_id              INTEGER NOT NULL REFERENCES account(id),
  level                INTEGER NOT NULL,
  UNIQUE (channel_id, nick_id)
);

DROP TABLE channel_akick;
CREATE TABLE channel_akick(
  id                  SERIAL PRIMARY KEY,
  channel_id          INTEGER NOT NULL REFERENCES channel(id),
  nick_id             INTEGER NOT NULL REFERENCES account(id), -- If a nickname akick
  setter              INTEGER NOT NULL REFERENCES account(id),
  mask                VARCHAR(255), -- If a mask akick
  reason              VARCHAR(512),
  time                INTEGER NOT NULL,
  duration            INTEGER NOT NULL
);
