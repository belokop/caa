CREATE TABLE IF NOT EXISTS `abs_events` (
  `e_id` int(11) NOT NULL AUTO_INCREMENT,
  `e_orgid` int(11) DEFAULT NULL,
  `e_code` int(11) DEFAULT NULL,
  `e_name` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `e_start` int(11) DEFAULT NULL,
  `e_end` int(11) DEFAULT NULL,
  `e_reg_start` int(11) DEFAULT NULL,
  `e_reg_end` int(11) DEFAULT NULL,
  `e_policy` int(11) DEFAULT NULL,
  `e_v_policy` tinytext COLLATE utf8_unicode_ci,
  `e_datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`e_id`),
  KEY `e_code` (`e_code`)
) AUTO_INCREMENT=1;
