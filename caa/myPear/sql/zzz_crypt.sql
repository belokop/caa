CREATE TABLE IF NOT EXISTS `zzz_crypt` (
  `c_code` int(11) NOT NULL,
  `c_value` varchar(230) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `c_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`c_code`),
  UNIQUE KEY `c_value` (`c_value`)
) AUTO_INCREMENT=1;
