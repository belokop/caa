CREATE TABLE `zzz_tt_locales` (
  `l_id` int(11) NOT NULL AUTO_INCREMENT,
  `l_locale` char(8),
  `l_name` char(32),
  `l_flag` char(8) NOT NULL,
  `l_currency` char(8) NOT NULL,
  PRIMARY KEY (`l_id`),
  KEY  (`l_locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
