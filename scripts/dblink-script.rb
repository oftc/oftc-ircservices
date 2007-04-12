#!/usr/bin/ruby

require 'dbi'
require 'yaml'

start = Time.now

basenick = ARGV[0]
nickcount = ARGV[1].to_i
regtime = Time.now.to_i
conffile = ARGV[2]
conf = YAML::load(File.open(conffile))

dbh = DBI.connect(conf["dest_str"], conf["dest_user"], conf["dest_pass"])

puts "Connected to DB, creating #{nickcount} links for nick #{basenick}"

dbh.execute("BEGIN")

prinid = dbh.select_one("SELECT nextval(pg_get_serial_sequence('nickname', 'id'))")[0]
accid = dbh.select_one("SELECT nextval(pg_get_serial_sequence('account', 'id'))")[0]

dbh.execute("INSERT INTO account(id, primary_nick, reg_time, password, salt, email) VALUES(?, ?, ?, ?, ?, ?)",
  accid, prinid, regtime, 'password', 'salt', basenick+'@oftc.net')

puts "INSERTed #{basenick} account with #{accid}"

dbh.execute("INSERT INTO nickname(id, nick, user_id, reg_time) VALUES(?, ?, ?, ?)",
  prinid, basenick, accid, regtime)

puts "INSERTed #{basenick} with #{prinid}"

nickid = 0
handle = nil

nickcount.times do |i|
  regtime = Time.now.to_i
  nickid = dbh.select_one("SELECT nextval(pg_get_serial_sequence('nickname', 'id'))")[0]
  nick = basenick + i.to_s
  handle = dbh.prepare("INSERT INTO nickname(id, nick, user_id, reg_time) VALUES(?, ?, ?, ?)")
  handle.execute(nickid, nick, accid, regtime)
  handle.finish
  puts "INSERTed #{nick} with #{nickid}"
end

dbh.execute("COMMIT")
dbh.disconnect

end = Time.now

puts "Finished in #{end - start}"
