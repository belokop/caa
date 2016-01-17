CREATE TABLE IF NOT EXISTS `lic_user` (
  `licu_id` int(128) NOT NULL AUTO_INCREMENT,
  `u_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `u_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`licu_id`)
) AUTO_INCREMENT=1;
