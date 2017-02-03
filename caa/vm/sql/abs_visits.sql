CREATE TABLE IF NOT EXISTS `abs_visits` (
  `v_id` int(11) NOT NULL AUTO_INCREMENT,
  `v_eid` int(11) DEFAULT NULL,
  `v_oid` int(11) DEFAULT NULL,
  `v_type` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `v_group` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `v_avid` int(11) DEFAULT NULL,
  `v_projectid` int(11) NOT NULL DEFAULT '0',
  `v_code` int(11) NOT NULL DEFAULT '0',
  `v_start` int(11) DEFAULT NULL,
  `v_end` int(11) DEFAULT NULL,
  `v_status` varchar(230) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `v_policy` int(11) NOT NULL DEFAULT '0',
  `v_accompaning` smallint(6) NOT NULL DEFAULT '0',
  `v_host_avid` int(11) DEFAULT NULL,
  `v_acc_wish` text COLLATE utf8_unicode_ci,
  `v_gsf` text COLLATE utf8_unicode_ci,
  `v_owner_avid` int(11) DEFAULT NULL,
  `v_admin_avid` int(11) DEFAULT NULL,
  `v_created` int(11) DEFAULT NULL,
  `v_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`v_id`),
  KEY `v_code` (`v_code`),
  KEY `v_eid` (`v_eid`,`v_oid`,`v_avid`,`v_host_avid`)
) AUTO_INCREMENT=1;
