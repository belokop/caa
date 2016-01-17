CREATE TABLE IF NOT EXISTS `abs_huts` (
  `hut_id` int(11) NOT NULL,
  `hut_name` varchar(230) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_code` varchar(32) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_status` varchar(32) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_capacity` int(11) NOT NULL DEFAULT '999',
  `hut_url` varchar(230) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_entrance_code` varchar(32) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_street` varchar(230) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_city` varchar(230) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_postcode` varchar(32) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `hut_country` varchar(4) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL DEFAULT 'SE',
  `hut_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`hut_id`),
  KEY `hut_name` (`hut_name`),
  KEY `hut_code` (`hut_code`)
) AUTO_INCREMENT=1;
