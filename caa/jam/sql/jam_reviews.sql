CREATE TABLE IF NOT EXISTS `jam_reviews` (
  `r_id` int(11) NOT NULL DEFAULT '0',
  `r_apid` int(11) NOT NULL DEFAULT '0',
  `r_avid` int(11) NOT NULL DEFAULT '0',
  `r_rating` tinyint(4) DEFAULT NULL,
  `r_comment` text COLLATE utf8_unicode_ci,
  `r_file` varchar(132) COLLATE utf8_unicode_ci DEFAULT NULL,
  `r_time` int(11) DEFAULT NULL,
  `r_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`r_id`)
) AUTO_INCREMENT=1;
