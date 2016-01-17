CREATE TABLE IF NOT EXISTS `zzz_logs` (
  `log_id` int(11) NOT NULL AUTO_INCREMENT,
  `log_ip` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `log_avid` int(11) NOT NULL DEFAULT '0',
  `log_form` int(11) NOT NULL DEFAULT '0',
  `log_time` int(11) NOT NULL DEFAULT '0',
  `log_type` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `log_api` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `log_org` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `log_comment` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`log_id`),
  KEY `log_time` (`log_time`),
  KEY `log_type` (`log_type`),
  KEY `log_form` (`log_form`)
) AUTO_INCREMENT=1;
