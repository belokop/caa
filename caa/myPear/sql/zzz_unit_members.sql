CREATE TABLE IF NOT EXISTS `zzz_unit_members` (
  `um_id` int(11) NOT NULL AUTO_INCREMENT,
  `um_avid` int(11) DEFAULT NULL,
  `um_status` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  `um_uid` int(11) DEFAULT NULL,
  `um_option` mediumtext COLLATE utf8_unicode_ci,
  `um_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`um_id`),
  KEY `um_avid` (`um_avid`),
  KEY `um_uid` (`um_uid`)
) AUTO_INCREMENT=1;
