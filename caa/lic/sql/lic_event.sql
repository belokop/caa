CREATE TABLE IF NOT EXISTS `lic_event` (
  `lice_id` int(128) NOT NULL AUTO_INCREMENT,
  `e_userid` int(11) NOT NULL DEFAULT '0',
  `e_hostid` int(11) NOT NULL DEFAULT '0',
  `e_srvid` int(11) NOT NULL DEFAULT '0',
  `e_sid` int(11) NOT NULL DEFAULT '0',
  `e_time` int(11) NOT NULL DEFAULT '0',
  `e_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`lice_id`)
) AUTO_INCREMENT=1;
