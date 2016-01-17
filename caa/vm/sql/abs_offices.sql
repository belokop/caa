CREATE TABLE IF NOT EXISTS `abs_offices` (
  `o_id` int(11) NOT NULL AUTO_INCREMENT,
  `o_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `o_phone` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `o_capacity` int(11) NOT NULL DEFAULT '0',
  `o_status` varchar(8) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'staff',
  `o_datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`o_id`),
  KEY `o_name` (`o_name`),
  KEY `o_status` (`o_status`)
) AUTO_INCREMENT=1;
