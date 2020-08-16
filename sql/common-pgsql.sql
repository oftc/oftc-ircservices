-- prerequisites for higher-level modules

CREATE OR REPLACE FUNCTION irc_lower(item text)
RETURNS text CALLED ON NULL INPUT LANGUAGE SQL PARALLEL SAFE
AS $$SELECT translate(lower(item), '[]\^', '{}|~')$$;

/* migration script for the above function:
CREATE UNIQUE INDEX channel_channel_idx2 ON channel (irc_lower(channel));
CREATE UNIQUE INDEX forbidden_channel_channel_idx2 ON forbidden_channel (irc_lower(channel));
CREATE UNIQUE INDEX ganneffserv_channel_idx2 ON ganneffserv (irc_lower(channel));
CREATE UNIQUE INDEX group_lower_name_idx ON "group" (irc_lower(name)); -- new index
CREATE UNIQUE INDEX nickname_nick_idx2 ON nickname (irc_lower(nick));
CREATE UNIQUE INDEX forbidden_nickname_nick_idx2 ON forbidden_nickname (irc_lower(nick));

DROP INDEX channel_channel_idx;
ALTER INDEX channel_channel_idx2 RENAME TO channel_channel_idx;
DROP INDEX forbidden_channel_channel_idx;
ALTER INDEX forbidden_channel_channel_idx2 RENAME TO forbidden_channel_channel_idx;
DROP INDEX ganneffserv_channel_idx;
ALTER INDEX ganneffserv_channel_idx2 RENAME TO ganneffserv_channel_idx;
DROP INDEX nickname_nick_idx;
ALTER INDEX nickname_nick_idx2 RENAME TO nickname_nick_idx;
DROP INDEX forbidden_nickname_nick_idx;
ALTER INDEX forbidden_nickname_nick_idx2 RENAME TO forbidden_nickname_nick_idx;
*/
