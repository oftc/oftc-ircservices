-- Some extra functions and views useful for looking into the database.
-- These are not used by services itself.

BEGIN;

CREATE OR REPLACE FUNCTION mktime(integer) RETURNS timestamp without time zone
    AS $_$ SELECT to_timestamp($1)::timestamp without time zone $_$
    LANGUAGE sql IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION nick(integer) RETURNS text
    AS $_$ SELECT nick FROM nickname JOIN account ON (account.primary_nick = nickname.id) WHERE account.id = $1 $_$
    LANGUAGE sql STABLE STRICT;

CREATE AGGREGATE array_accum(anyelement) (
    SFUNC = array_append,
    STYPE = anyarray,
    INITCOND = '{}'
);

CREATE OR REPLACE VIEW akick_view AS
    SELECT channel.channel,
	channel_akick.chmode,
	COALESCE(nick(channel_akick.target), channel_akick.mask) AS mask,
	mktime(channel_akick."time") AS "time",
	channel_akick.duration,
	channel_akick.reason
    FROM channel
	JOIN channel_akick ON (channel.id = channel_akick.channel_id);

CREATE OR REPLACE VIEW akill_view AS
    SELECT nick(akill.setter) AS setter,
	mktime(akill."time") AS "time",
	justify_hours((akill.duration || ' sec'::text)::interval) AS duration,
	akill.mask,
	akill.reason
    FROM akill;

CREATE OR REPLACE VIEW cs_view AS
    SELECT c.channel,
	"substring"((c.description)::text, 1, 50) AS description,
	"substring"((c.topic)::text, 1, 50) AS topic,
	mktime(c.reg_time) AS reg_time,
	mktime(c.last_used) AS last_used,
	array_accum(nick(ca.account_id)) AS masters
    FROM channel c
	LEFT JOIN channel_access ca ON ((c.id = ca.channel_id) AND (ca.level = 4))
    GROUP BY 1, 2, 3, 4, 5;

CREATE OR REPLACE VIEW ns_view AS
    SELECT nickname.nick,
	mktime(nickname.last_seen) AS last_seen,
	mktime(account.reg_time) AS reg_time,
	mktime(account.last_quit_time) AS last_quit_time,
	account.last_host,
	account.last_realname,
	account.last_quit_msg,
	account.email,
	account.cloak,
	account.url
    FROM account JOIN nickname ON (account.primary_nick = nickname.id);

COMMIT;
