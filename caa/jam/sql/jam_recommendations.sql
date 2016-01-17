CREATE TABLE IF NOT EXISTS `jam_recommendations` (
  `rec_id` int(11) NOT NULL AUTO_INCREMENT,
  `rec_apid` int(11) NOT NULL DEFAULT '0',
  `rec_avid` int(11) NOT NULL DEFAULT '0',
  `rec_letter_file` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `rec_comment` text CHARACTER SET utf8 ,
  `rec_submitdate` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `rec_askdate` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `rec_ip` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `rec_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`rec_id`),
  KEY `rec_apid` (`rec_apid`,`rec_avid`),
  KEY `rec_avid` (`rec_avid`)
) AUTO_INCREMENT=1;
