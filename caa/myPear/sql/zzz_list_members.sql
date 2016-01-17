CREATE TABLE IF NOT EXISTS `zzz_list_members` (
  `lm_id` int(11) NOT NULL AUTO_INCREMENT,
  `lm_lid` int(11) NOT NULL,
  `lm_key` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `lm_value` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `lm_status` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `lm_option` mediumtext COLLATE utf8_unicode_ci,
  `lm_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`lm_id`),
  UNIQUE KEY `lm_id` (`lm_id`,`lm_lid`),
  KEY `key2` (`lm_key`,`lm_value`),
  KEY `key3` (`lm_lid`,`lm_key`,`lm_value`),
  KEY `key1` (`lm_key`)
) AUTO_INCREMENT=1;
