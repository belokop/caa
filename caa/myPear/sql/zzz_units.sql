CREATE TABLE IF NOT EXISTS `zzz_units` (
  `u_id` int(11) NOT NULL DEFAULT '0',
  `u_rank` int(11) NOT NULL,
  `u_parent` varchar(132) COLLATE utf8_unicode_ci NOT NULL,
  `u_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `u_class` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `u_member_title` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `u_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`u_id`),
  KEY `u_parent_id` (`u_parent`),
  KEY `unit_id` (`u_rank`,`u_parent`)
) AUTO_INCREMENT=1;
