# phpMyAdmin MySQL-Dump
# version 2.3.2
# http://www.phpmyadmin.net/ (download page)
#
# Host: syssql.physto.se
# Generation Time: Oct 09, 2003 at 01:37 PM
# Server version: 3.23.38
# PHP Version: 4.3.3
# Database : `hostdb_physto_se`
# --------------------------------------------------------

#
# Table structure for table `dhcpd`
#

CREATE TABLE dhcpd (
  id int(11) NOT NULL auto_increment,
  mac varchar(24) default NULL,
  name varchar(40) default NULL,
  user varchar(24) default NULL,
  ip varchar(24) default NULL,
  edt varchar(24) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

