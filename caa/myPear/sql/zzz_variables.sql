CREATE TABLE IF NOT EXISTS `zzz_variables` (
  `var_module` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '''''',
  `var_name` varchar(64) COLLATE utf8_unicode_ci NOT NULL DEFAULT '''''',
  `var_value` text COLLATE utf8_unicode_ci,
  `var_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY `var_module` (`var_module`,`var_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
