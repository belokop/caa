CREATE TABLE IF NOT EXISTS `zzz_mail_queue` (
  `id` bigint(20) NOT NULL DEFAULT '0',
  `create_time` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `time_to_send` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `sent_time` datetime DEFAULT NULL,
  `id_user` bigint(20) NOT NULL DEFAULT '0',
  `ip` varchar(20) CHARACTER SET utf8  NOT NULL DEFAULT 'unknown',
  `sender` varchar(50) CHARACTER SET utf8  NOT NULL DEFAULT '',
  `recipient` text CHARACTER SET utf8  NOT NULL,
  `headers` text CHARACTER SET utf8  NOT NULL,
  `body` longtext CHARACTER SET utf8  NOT NULL,
  `try_sent` tinyint(4) NOT NULL DEFAULT '0',
  `delete_after_send` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`),
  KEY `id` (`id`),
  KEY `time_to_send` (`time_to_send`),
  KEY `id_user` (`id_user`)
) AUTO_INCREMENT=1;
