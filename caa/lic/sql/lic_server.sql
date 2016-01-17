CREATE TABLE IF NOT EXISTS `lic_server` (
  `licsrv_id` int(128) NOT NULL AUTO_INCREMENT,
  `srv_name` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `srv_port` int(11) NOT NULL DEFAULT '0',
  `srv_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`licsrv_id`)
) AUTO_INCREMENT=1;
