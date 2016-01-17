CREATE TABLE IF NOT EXISTS `prp_list` (
  `prp_id` int(11) NOT NULL AUTO_INCREMENT,
  `prp_authors` tinytext CHARACTER SET utf8 ,
  `prp_title` tinytext CHARACTER SET utf8 ,
  `prp_field` varchar(8) CHARACTER SET utf8  DEFAULT NULL,
  `prp_orgid` int(11) DEFAULT NULL,
  `prp_avid` int(11) DEFAULT NULL,
  `prp_report` int(11) DEFAULT NULL,
  `prp_day0` int(11) DEFAULT NULL,
  `prp_local` varchar(200) CHARACTER SET utf8  DEFAULT NULL,
  `prp_status` varchar(16) CHARACTER SET utf8  DEFAULT NULL,
  `prp_archive` varchar(200) CHARACTER SET utf8  DEFAULT NULL,
  `prp_doi` varchar(200) CHARACTER SET utf8  DEFAULT NULL,
  `prp_publisher` varchar(200) CHARACTER SET utf8  DEFAULT NULL,
  `prp_tm` int(11) DEFAULT NULL,
  `prp_timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`prp_id`),
  KEY `prp_avid` (`prp_avid`,`prp_day0`,`prp_tm`)
) AUTO_INCREMENT=1;
