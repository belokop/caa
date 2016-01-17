CREATE TABLE IF NOT EXISTS `lic_host` (
  `lich_id` int(128) NOT NULL AUTO_INCREMENT,
  `h_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `h_ip` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `h_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`lich_id`)
) AUTO_INCREMENT=1;
