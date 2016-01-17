CREATE TABLE IF NOT EXISTS `jam_documents` (
  `doc_id` int(11) NOT NULL AUTO_INCREMENT,
  `doc_apid` int(11) NOT NULL DEFAULT '0',
  `doc_date` int(11) NOT NULL DEFAULT '0',
  `doc_lmid` int(11) NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`doc_id`),
  KEY `doc_apid` (`doc_apid`,`doc_lmid`)
) AUTO_INCREMENT=1;
