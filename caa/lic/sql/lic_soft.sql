CREATE TABLE IF NOT EXISTS `lic_soft` (
  `lics_id` int(128) NOT NULL AUTO_INCREMENT,
  `s_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `s_vrsn` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `s_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`lics_id`)
) AUTO_INCREMENT=1;
