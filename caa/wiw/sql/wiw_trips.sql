CREATE TABLE IF NOT EXISTS `wiw_trips` (
  `t_id` int(11) NOT NULL,
  `t_idf` int(11) DEFAULT NULL,
  `t_idn` int(11) DEFAULT NULL,
  `t_departure` int(11) DEFAULT NULL,
  `t_arrival` int(11) DEFAULT NULL,
  `t_avid` int(11) DEFAULT NULL,
  `t_orgid` int(11) DEFAULT NULL,
  `t_type` int(11) DEFAULT NULL,
  `t_destination` varchar(128) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `t_contact` text,
  `t_comment` text,
  `t_acl` varchar(32) DEFAULT NULL,
  `t_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`t_id`),
  KEY `t_departure` (`t_departure`),
  KEY `t_arrival` (`t_arrival`),
  KEY `t_avid` (`t_avid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
