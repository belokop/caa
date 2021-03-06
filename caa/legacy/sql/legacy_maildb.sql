CREATE TABLE IF NOT EXISTS `legacy_maildb` (
  `m_id` int(11) NOT NULL AUTO_INCREMENT,
  `user` varchar(35) NOT NULL DEFAULT '',
  `m_photo` varchar(132) NOT NULL DEFAULT '',
  `groupid` varchar(12) NOT NULL DEFAULT '',
  `name` varchar(35) NOT NULL DEFAULT '',
  `altname` varchar(109) DEFAULT NULL,
  `pers_number` varchar(51) DEFAULT NULL,
  `expiration` date NOT NULL DEFAULT '0000-00-00',
  `phone_home` varchar(27) NOT NULL DEFAULT '',
  `mailname` varchar(35) NOT NULL DEFAULT '',
  `forward` varchar(64) NOT NULL DEFAULT '',
  `cmt` varchar(64) NOT NULL DEFAULT '',
  `status` enum('','Active','Passive','Expired') NOT NULL DEFAULT '',
  `postnummer` varchar(23) DEFAULT NULL,
  `ortnamn` varchar(32) NOT NULL DEFAULT '',
  `address` varchar(79) DEFAULT NULL,
  `office` varchar(52) DEFAULT NULL,
  `von` varchar(34) DEFAULT NULL,
  `namefam` varchar(45) DEFAULT NULL,
  `position` varchar(34) NOT NULL DEFAULT '',
  `employeeType` varchar(32) NOT NULL DEFAULT '',
  `employeeTitle` varchar(64) NOT NULL DEFAULT '',
  `country` varchar(12) NOT NULL DEFAULT '',
  `maildrop` varchar(48) NOT NULL DEFAULT '',
  `last_login` text NOT NULL,
  `phone` varchar(17) NOT NULL DEFAULT '',
  `changeslog` text NOT NULL,
  `changesdate` varchar(53) DEFAULT NULL,
  `sukatdate` varchar(41) DEFAULT NULL,
  `altacct` varchar(81) NOT NULL DEFAULT '',
  `acc_su` varchar(20) NOT NULL DEFAULT '',
  `acc_kth` varchar(20) NOT NULL DEFAULT '',
  `aliases` varchar(240) NOT NULL DEFAULT '',
  `edt` date NOT NULL DEFAULT '0000-00-00',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `rating` smallint(6) NOT NULL DEFAULT '0',
  `phone_mobile` varchar(20) NOT NULL DEFAULT '',
  `created_by` varchar(32) NOT NULL DEFAULT '',
  `institution` varchar(24) NOT NULL DEFAULT 'FKM',
  `norGraduationCity` varchar(48) NOT NULL DEFAULT '',
  `norHomeInstitute` varchar(128) NOT NULL DEFAULT '',
  `norLocalContactPerson` varchar(64) NOT NULL DEFAULT '',
  `norPeriodOfStay` varchar(64) NOT NULL DEFAULT '',
  `su_code` varchar(24) DEFAULT NULL,
  PRIMARY KEY (`m_id`),
  UNIQUE KEY `user_2` (`user`),
  KEY `pers_number` (`pers_number`),
  KEY `phone` (`phone`),
  KEY `name` (`name`),
  KEY `status` (`status`),
  KEY `von` (`von`),
  KEY `institution` (`institution`),
  KEY `namefam` (`namefam`),
  KEY `user` (`user`)
);
