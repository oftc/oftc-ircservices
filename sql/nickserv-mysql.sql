CREATE TABLE `nicknames` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `nick` varchar(30) NOT NULL default '',
  `password` varchar(34) NOT NULL default '',
  `url` varchar(255) NOT NULL default '',
  `email` varchar(255) NOT NULL default '',
  `last_host` varchar(255) NOT NULL default '',
  `last_realname` varchar(255) NOT NULL default '',
  `last_quit` varchar(255) NOT NULL default '',
  `last_quit_time` int(10) NOT NULL default '0',
  `reg_time` int(10) NOT NULL default '0',
  `last_seen` int(10) NOT NULL default '0',
  `last_used` int(10) NOT NULL default '0',
  `status` int(10) unsigned NOT NULL default '0',
  `flags` int(10) unsigned NOT NULL default '0',
  `language` smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `nick` (`nick`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
