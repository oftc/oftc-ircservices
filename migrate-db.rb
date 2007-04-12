#!/usr/bin/ruby
#old services migration script, requires libdbd-mysql-ruby libdbd-pg-ruby
#specify a yaml file on the cli that you use as a config, see migrate-db.yaml

require 'dbi'
require 'yaml'

NI_ENFORCE      = 0x1
NI_SECURE       = 0x2
NI_PRIVATE      = 0x40
NI_CLOAK        = 0x100000

CI_PRIVATE      = 0x4
CI_TOPICLOCK    = 0x8
CI_RESTRICTED   = 0x10
CI_VERBOTEN     = 0x80
CI_VERBOSE      = 0x800

$oftcid = -1

$nicks = {}
$channels = {}

def bit_check(flags, level)
  if (flags & level > 0) then true else false end
end

def process_nicks()
  source_handle = $source.prepare("SELECT pass, salt, url, email, 
    cloak_string, last_usermask, last_realname, last_quit, last_quit_time, 
    time_registered, nick, last_seen, flags, nick_id, link_id FROM nick 
    ORDER BY link_id")
  source_handle.execute

  skipped = {}

  while row = source_handle.fetch do
    if row["link_id"].to_i != 0
      linkid = row["link_id"].to_i
      if $nicks.include?(linkid)
        insert_handle = $dest.prepare("INSERT INTO nickname(nick, user_id,
          reg_time, last_seen) VALUES(?, ?, ?, ?)")

        insert_handle.execute(row["nick"],
                              $nicks[linkid],
                              row["time_registered"],
                              row["last_seen"] == "" ? nil : row["last_seen"])

        $nicks[row["nick_id"].to_i] = $nicks[linkid]

        insert_handle.finish
      else
        puts "link id: #{linkid} not in yet handled"
	skipped[linkid] = [] unless skipped[linkid]
	skipped[linkid] << row
      end
    else
      nickid = $dest.select_one("SELECT nextval('nickname_id_seq')")
      insert_handle = $dest.prepare("INSERT INTO account(primary_nick, 
      password, salt, url, 
        email, cloak, last_host, last_realname, last_quit_msg, 
        last_quit_time, reg_time, flag_cloak_enabled, flag_secure,
        flag_enforce, flag_private) 
        VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")

      flags = row["flags"].to_i
      flag_enforce = bit_check(flags, NI_ENFORCE)
      flag_secure  = bit_check(flags, NI_SECURE)
      flag_cloak   = bit_check(flags, NI_CLOAK)
      flag_private = bit_check(flags, NI_PRIVATE)

      url = row["url"] == "" ? nil : row["url"]
      cloak = row["cloak_string"] == "" ? nil : row["cloak_string"]
      lastuser = row["last_usermask"] == "" ? nil : row["last_usermask"]
      lastreal = row["last_realname"] == "" ? nil : row["last_realname"]
      lastquit = row["last_quit"] == "" ? nil : row["last_quit"]
      lastquittime = row["last_quit_time"] == "" ? nil : row["last_quit_time"]

      insert_handle.execute(nickid[0], row["pass"], row["salt"], url, row["email"],
        cloak, lastuser, lastreal, lastquit, lastquittime, 
	row["time_registered"], flag_cloak, flag_secure, flag_enforce, 
	flag_private)

      insert_handle.finish

      accid = $dest.select_one("SELECT currval('account_id_seq')")
      insert_handle = $dest.prepare("INSERT INTO nickname(nick, user_id,
        reg_time, last_seen) VALUES(?, ?, ?, ?)")

      $oftcid = row["nick_id"] if row["nick"] == "OFTC"

      insert_handle.execute(row["nick"], accid[0], row["time_registered"],
        row["last_seen"])

      insert_handle.finish

      $nicks[row["nick_id"].to_i] = accid[0].to_i

      nickid = $dest.select_one("SELECT currval('nickname_id_seq')")
      $dest.execute("UPDATE account SET primary_nick = ? WHERE id = ?",
        nickid[0], accid[0])
    end
  end

  if skipped.length > 1
  	skipped.each_key do |x|
		puts x
	end
  end
end

def process_admins
  handle = $source.prepare("SELECT admin_id, nick_id FROM admin")
  handle.execute

  while row = handle.fetch do
    $dest.execute("UPDATE account SET flag_admin=true WHERE id = ?", $nicks[row["nick_id"]])
  end

  handle.finish
end

def process_nickaccess()
  handle = $source.prepare("SELECT nick_id, userhost FROM nickaccess")
  handle.execute

  while row = handle.fetch do
    $dest.execute("INSERT INTO account_access (parent_id, entry) VALUES(?, ?)",
      $nicks[row["nick_id"]], row["userhost"])
  end

  handle.finish
end

def process_channels()
  handle = $source.prepare("SELECT name, description, url, email, last_topic,
    entry_message, time_registered, last_used, flags, channel_id, mlock_on,
    mlock_off, mlock_limit, mlock_key, limit_offset, bantime, founder,
    successor FROM channel")
  handle.execute

  while row = handle.fetch do
    insert_handle = $dest.prepare("INSERT INTO channel(channel, description,
      url, email, topic, entrymsg, reg_time, last_used, flag_private,
      flag_restricted, flag_topic_lock, flag_verbose, flag_autolimit,
      flag_expirebans, flag_forbidden)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")

    flags = row["flags"].to_i
    flag_private = bit_check(flags, CI_PRIVATE)
    flag_restricted = bit_check(flags, CI_RESTRICTED)
    flag_topiclock = bit_check(flags, CI_TOPICLOCK)
    flag_verbose = bit_check(flags, CI_VERBOSE)
    flag_autolimit = if row["limit_offset"].to_i != 0 then true else false end
    flag_expirebans = if row["bantime"].to_i != 0 then true else false end
    flag_forbidden = bit_check(flags, CI_VERBOTEN)

    url = row["url"] == "" ? nil : row["url"]
    email = row["email"] == "" ? nil : row["email"]
    topic = row["last_topic"] == "" ? nil : row["last_topic"]
    entrymsg = row["entry_message"] == "" ? nil : row["entry_message"]

    insert_handle.execute(row["name"], row["description"], url, email,
      topic, entrymsg, row["time_registered"], row["last_used"],
      flag_private, flag_restricted, flag_topiclock, flag_verbose, 
      flag_autolimit, flag_expirebans, flag_forbidden)

    insert_handle.finish

    cid = $dest.select_one("SELECT currval('channel_id_seq')")

    chanid = cid[0].to_i
    oldchanid = row["channel_id"].to_i
    founder = row["founder"].to_i
    successor = row["successor"].to_i

    $channels[oldchanid] = chanid

    founder = $oftcid if founder == 0 or ! $nicks.include?(founder)

    $dest.execute("INSERT INTO channel_access(channel_id, account_id, level)
      VALUES (?, ?, ?)", chanid, $nicks[founder], 4)

    if successor > 0 && founder != successor && $nicks.include?(successor) &&
        $nicks[successor] != founder && $nicks[founder] != successor &&
        $nicks[successor] != $nicks[founder]
    then
      $dest.execute("INSERT INTO channel_access(channel_id, account_id, level)
        VALUES (?, ?, ?)", chanid, $nicks[successor], 4)
    end
  end

  handle.finish
end

def process_chanaccess()
  handle = $source.prepare("SELECT channel.channel_id, level, nick_id, 
    founder, successor FROM chanaccess, channel
    WHERE chanaccess.channel_id = channel.channel_id ORDER BY channel_id")
  handle.execute

  lastid = -1
  done = {}

  while row = handle.fetch do
    chanid = row["channel_id"].to_i
    accid = row["nick_id"].to_i
    founder = row["founder"].to_i
    successor = row["successor"].to_i
    level = row["level"].to_i

    if lastid != chanid
      lastid = chanid
      done.each_key { |key| done.delete(key) }
    end

    if (accid == founder || accid == successor || done.include?($nicks[accid]) ||
      $nicks[accid] == $nicks[founder] ||
      ($nicks.include?(successor) and $nicks[successor] == $nicks[accid])) then
      next
    end

    insert_handle = $dest.prepare("INSERT INTO channel_access (channel_id,
      account_id, level) VALUES (?, ?, ?)");

      nlevel = 3
      nlevel = 2 if level < 5

    insert_handle.execute($channels[chanid], $nicks[accid], nlevel)

    insert_handle.finish

    done[$nicks[accid]] = true
  end

  handle.finish
end

if ARGV.length != 1
  puts "please specify config file"
  exit(-1)
end

conffile = ARGV.shift
conf = YAML::load(File.open(conffile))

$source = DBI.connect(conf["source_str"], conf["source_user"], conf["source_pass"])
$dest   = DBI.connect(conf["dest_str"], conf["dest_user"], conf["dest_pass"])

$dest.do("BEGIN")

start = Time.now
puts "process_nicks #{Time.now}"
process_nicks()

total = Time.now - start
puts "process_admins #{Time.now} (#{total})"
process_admins()

total = Time.now - start
puts "process_nickaccess #{Time.now} (#{total})"
process_nickaccess()

total = Time.now - start
puts "process_channels #{Time.now} (#{total})"
process_channels()

total = Time.now - start
puts "process_chanaccess #{Time.now} (#{total})"
process_chanaccess()

total = Time.now - start
puts "done #{Time.now} total #{total}"

$dest.do("COMMIT")

$source.disconnect
$dest.disconnect
