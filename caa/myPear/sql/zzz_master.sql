CREATE TABLE IF NOT EXISTS `zzz_master` (
  `master_id` int(11) NOT NULL AUTO_INCREMENT,
  `master_object` varchar(24) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `master_value` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `master_option` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `master_ts` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`master_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

