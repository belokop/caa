CREATE TABLE IF NOT EXISTS `zzz_organizations` (
  `org_id` int(11) NOT NULL AUTO_INCREMENT,
  `org_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `org_name_short` varchar(32) COLLATE utf8_unicode_ci NOT NULL,
  `org_country` varchar(64) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'SE',
  `org_code` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  `org_signature_dir` tinytext COLLATE utf8_unicode_ci,
  `org_domain` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `org_theme` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  `org_affil` varchar(230) COLLATE utf8_unicode_ci DEFAULT NULL,
  `org_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`org_id`),
  UNIQUE KEY `f_nickname` (`org_code`),
  UNIQUE KEY `org_code` (`org_code`)
) AUTO_INCREMENT=1;
