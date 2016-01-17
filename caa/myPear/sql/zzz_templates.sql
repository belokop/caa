CREATE TABLE IF NOT EXISTS `zzz_templates` (
  `tpl_id` int(11) NOT NULL AUTO_INCREMENT,
  `tpl_owner` varchar(132) COLLATE utf8_unicode_ci NOT NULL,
  `tpl_title` varchar(230) COLLATE utf8_unicode_ci NOT NULL,
  `tpl_body` longtext COLLATE utf8_unicode_ci NOT NULL,
  `tpl_render` varchar(230) COLLATE utf8_unicode_ci NOT NULL,
  `tpl_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`tpl_id`),
  KEY `tpl_title` (`tpl_title`)
) AUTO_INCREMENT=1;
