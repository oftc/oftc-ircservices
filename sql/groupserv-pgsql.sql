DROP TABLE "group" CASCADE;
CREATE TABLE "group" (
  id                  SERIAL PRIMARY KEY,
  description         VARCHAR(255) NOT NULL,
  url                 VARCHAR(255),
  email               VARCHAR(255) NOT NULL,
  flag_private        BOOLEAN NOT NULL DEFAULT 'False',
  reg_time            INTEGER NOT NULL 
);
