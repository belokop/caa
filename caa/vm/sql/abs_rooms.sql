CREATE TABLE IF NOT EXISTS `abs_rooms` (
  `a_id` int(11) NOT NULL AUTO_INCREMENT,
  `a_hutid` int(11) DEFAULT NULL,
  `a_start` int(11) DEFAULT NULL,
  `a_end` int(11) DEFAULT NULL,
  `a_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `a_code` varchar(8) COLLATE utf8_unicode_ci DEFAULT NULL,
  `a_phone` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `a_area` int(11) NOT NULL DEFAULT '0',
  `a_price` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `a_price_cleaning` int(11) NOT NULL DEFAULT '0',
  `a_capacity` int(11) NOT NULL DEFAULT '0',
  `a_status` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `a_comment` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `a_datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`a_id`),
  KEY `a_code` (`a_code`)
) AUTO_INCREMENT=1;
