-- DUMP Current services with the following command
-- mysqldump -u <username> -p -cnt --extended-insert=FALSE --compatible=postgresql <servicesdb> [ALL TABLES BUT SESSIONS] > somefile
-- CREATE DB SCHEMA oldservices first see pgsql-oldservices.sql
-- INSERT mysql dump add to the top of the mysql dump file SET search_path TO oldservices; and do the following
-- psql -f somefile <database name> 2> error.log
-- if there are no errors in the file 
-- psql -f thisfile <database name>

BEGIN;
-- CLEAR PREVIOUS
DELETE FROM public.nickname_access;
DELETE FROM public.channel;
DELETE FROM public.nickname;
-- INSERT NICKNAMES
INSERT INTO 
  public.nickname(
    nick, password, salt, url, email, cloak, last_host, last_realname, last_quit, last_quit_time, reg_time, last_seen, status, flags, language
  )
SELECT
  nick, pass, salt, url, email, cloak_string, last_usermask, last_realname, last_quit, last_quit_time, time_registered, last_seen, status, flags, language
FROM oldservices.nick;
-- INSERT NICNAME ACCESS LIST
INSERT INTO
  public.nickname_access(parent_id, entry)
SELECT public.nickname.id, oldservices.nickaccess.userhost
FROM
  oldservices.nickaccess
JOIN
  oldservices.nick ON oldservices.nick.nick_id = oldservices.nickaccess.nick_id
JOIN
  public.nickname ON oldservices.nick.nick = public.nickname.nick;
-- FIX ANY CHANNELS WITH MISSING SUCCESSORS
UPDATE
  oldservices.channel
SET
  successor = 0
WHERE
  channel_id IN (
    (SELECT
       channel_id
     FROM
       oldservices.channel
     WHERE
       successor != 0)
    EXCEPT
    (SELECT
       channel_id
     FROM
       oldservices.channel
     JOIN
       oldservices.nick ON oldservices.channel.successor = nick.nick_id
    )
   );
-- At least 6
-- INSERT CHANNELS
INSERT INTO
  public.channel(
    channel, founder, successor, description, url, email, entrymsg, topic, flags
  )
SELECT
  origchan.name,
  pfnd.id,
  successor,
  origchan.description,
  origchan.url,
  origchan.email,
  origchan.entry_message,
  origchan.last_topic,
  origchan.flags
FROM
  oldservices.channel AS origchan
JOIN
  oldservices.nick ON origchan.founder = oldservices.nick.nick_id
JOIN
  public.nickname as pfnd ON oldservices.nick.nick = pfnd.nick
-- 3333 (because 10 channels are forbidden?)
-- update successor ids
UPDATE public.channel
SET successor =
  (select id 
   from nickname
   join oldservices.nick on nick.nick = nickname.nick
   where nick_id = successor)
WHERE successor > 0;
-- at least 143
COMMIT;
