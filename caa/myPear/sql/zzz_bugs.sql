CREATE TABLE IF NOT EXISTS `zzz_bugs` (
  `bug_id` int(11) NOT NULL AUTO_INCREMENT,
  `bug_title_RO` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `bug_time_RO` int(11) NOT NULL DEFAULT '0',
  `bug_ip_RO` varchar(20) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `bug_form_RO` varchar(12) COLLATE utf8_unicode_ci NOT NULL DEFAULT '0',
  `bug_text` tinytext COLLATE utf8_unicode_ci NOT NULL,
  `bug_email` varchar(128) COLLATE utf8_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`bug_id`)
) AUTO_INCREMENT=1;
