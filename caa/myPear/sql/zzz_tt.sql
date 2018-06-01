CREATE TABLE `zzz_tt` (
  `tt_id` int(11) NOT NULL AUTO_INCREMENT,
  `tt_module` char(8) COLLATE utf8_unicode_ci NOT NULL,
  `tt_key` char(32) COLLATE utf8_unicode_ci NOT NULL,
  `tt_locale` char(8) COLLATE utf8_unicode_ci DEFAULT NULL,
  `tt_translated` text COLLATE utf8_unicode_ci,
  `tt_default` text COLLATE utf8_unicode_ci,
  PRIMARY KEY (`tt_id`),
  KEY `tt_key` (`tt_key`),
  KEY `tt_locale` (`tt_locale`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1777 ;
