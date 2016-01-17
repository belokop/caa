CREATE TABLE IF NOT EXISTS `jam_rf` (
  `rf_id` int(11) NOT NULL AUTO_INCREMENT,
  `rf_posid` int(11) NOT NULL DEFAULT '0',
  `rf_lmid` int(11) NOT NULL,
  `rf_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`rf_id`)
) AUTO_INCREMENT=1;
