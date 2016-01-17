CREATE TABLE IF NOT EXISTS `abs_leases` (
  `lease_id` int(11) NOT NULL AUTO_INCREMENT,
  `lease_vid` int(11) DEFAULT NULL,
  `lease_aid` int(11) DEFAULT NULL,
  `lease_start` int(11) DEFAULT NULL,
  `lease_end` int(11) DEFAULT NULL,
  `lease_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`lease_id`),
  KEY `l_vid` (`lease_vid`),
  KEY `l_aid` (`lease_aid`),
  KEY `l_start_2` (`lease_start`,`lease_end`)
) AUTO_INCREMENT=1;
