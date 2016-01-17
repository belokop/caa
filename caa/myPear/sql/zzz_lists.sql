CREATE TABLE IF NOT EXISTS `zzz_lists` (
  `l_id` int(11) NOT NULL AUTO_INCREMENT,
  `l_parent` int(11) DEFAULT NULL,
  `l_name` varchar(200) COLLATE utf8_unicode_ci DEFAULT NULL,
  `l_class` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `l_member_title` varchar(132) COLLATE utf8_unicode_ci NOT NULL,
  `l_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`l_id`),
  KEY `key` (`l_parent`,`l_name`)
) AUTO_INCREMENT=1;
