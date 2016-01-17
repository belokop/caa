CREATE TABLE IF NOT EXISTS `zzz_semaphore` (
  `s_name` varchar(240) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `s_value` varchar(240) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `s_expire` double NOT NULL,
  `s_ts` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`s_name`),
  KEY `s_value` (`s_value`),
  KEY `s_expire` (`s_expire`)
) AUTO_INCREMENT=1;
