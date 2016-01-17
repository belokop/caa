<?php
/* 
 *  pear install -a MDB2_Driver_mysql 
 *  pear install -a Mail_Queue
 *  pear install -a Mail_mimeDecode
 */

error_reporting(E_ALL & ~E_DEPRECATED & ~E_STRICT );
require_once "Mail/Queue.php";

// options for storing the messages
// type is the container used, currently there are 'creole', 'db', 'mdb' and 'mdb2' available
$db_options['type']       = 'mdb2';
// the others are the options for the used container
// here are some for db
$db_options['dsn']        = 'mysql://user:password@host/database';
$db_options['mail_table'] = 'zzz_mail_queue';

// here are the options for sending the messages themselves
// these are the options needed for the Mail-Class, especially used for Mail::factory()
$mail_options['driver']    = 'smtp';
$mail_options['host']      = 'smtp.kth.se';
$mail_options['port']      = 587;
$mail_options['localhost'] = 'localhost'; //optional Mail_smtp parameter
$mail_options['auth']      = False;
$mail_options['username']  = '';
$mail_options['password']  = '';

print_r($db_options);