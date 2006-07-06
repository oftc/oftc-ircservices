CREATE TABLE channel (
  id              INTEGER PRIMARY KEY auto_increment,
  channel         VARCHAR(30) NOT NULL default '',
  founder         INTEGER NOT NULL default '0',
  
  FOREIGN KEY (founder) REFERENCES nickname (id),
  UNIQUE (channel)
)ENGINE=InnoDB;
