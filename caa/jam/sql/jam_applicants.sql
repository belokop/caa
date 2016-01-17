CREATE TABLE IF NOT EXISTS `jam_applicants` (
  `ap_id` int(11) NOT NULL AUTO_INCREMENT,
  `ap_tmp` int(11) NOT NULL,
  `ap_avid` int(11) NOT NULL DEFAULT '0',
  `ap_posid` int(11) NOT NULL DEFAULT '0',
  `ap_rfid` int(11) NOT NULL DEFAULT '0',
  `ap_advisor_avid` int(11) NOT NULL DEFAULT '0',
  `ap_rating` int(11) DEFAULT NULL,
  `ap_status` int(11) NOT NULL DEFAULT '0',
  `ap_status_auto` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'a:0:{}',
  `ap_comment` text COLLATE utf8_unicode_ci NOT NULL,
  `ap_dossier_url` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `ap_submitdate` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `ap_ip` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `ap_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`ap_id`),
  KEY `ap_avid` (`ap_avid`,`ap_posid`,`ap_rfid`)
) AUTO_INCREMENT=1;
