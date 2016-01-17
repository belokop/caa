CREATE TABLE IF NOT EXISTS `zzz_avatars` (
  `av_id` int(11) NOT NULL AUTO_INCREMENT,
  `av_id2` text COLLATE utf8_unicode_ci,
  `av_photo` varchar(127) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_sex` varchar(4) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `av_ssn` varchar(120) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_birthdate` int(11) DEFAULT NULL,
  `av_ddate` int(32) DEFAULT NULL,
  `av_birthcountry` varchar(8) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_birthplace` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_email` varchar(132) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_email2` text COLLATE utf8_unicode_ci,
  `av_firstname` varchar(132) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `av_von` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `av_lastname` varchar(132) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `av_salutation` varchar(132) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_phone` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_institute` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_position` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_address` varchar(132) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_zip` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_city` varchar(64) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_residentship` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_citizenship` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_http` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_ms_institute` text COLLATE utf8_unicode_ci,
  `av_ms_year` int(11) DEFAULT NULL,
  `av_ms_country` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_phd_institute` text COLLATE utf8_unicode_ci,
  `av_phd_year` int(11) DEFAULT NULL,
  `av_phd_country` varchar(32) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_pwd` varchar(64) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_pwd2` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_identity` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `av_lastlogin` int(11) DEFAULT NULL,
  `av_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`av_id`),
  UNIQUE KEY `av_email` (`av_email`,`av_email2`(200)),
  KEY `av_pwd` (`av_pwd`,`av_pwd2`),
  KEY `av_identity` (`av_identity`),
  KEY `av_residentship` (`av_residentship`,`av_citizenship`),
  KEY `av_birthplace` (`av_birthplace`),
  KEY `av_birthcountry` (`av_birthcountry`),
  KEY `av_ms_country` (`av_ms_country`),
  KEY `av_phd_country` (`av_phd_country`),
  KEY `av_ssn` (`av_ssn`)
) AUTO_INCREMENT=1;
