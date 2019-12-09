<?php

//
// Settings for the MySQL database.
// The database must exist and be writable by the user
//

// Database engine class (NOT TO BE CHANGED!)
$GLOBALS['DBengine'] = $DBengine = 'bDB_myPear';

// Database access
foreach(array('host'  =>'localhost',
	      'dbname'=>'database',
              'user'  =>'username',
              'passwd'=>'password',
	      ) as $item=>$v){
  $GLOBALS[$DBengine][$item] = $v;
}

//
// Settings for the SMTP mail server
// Set user/passwd to the valid username/password if your server uses authentication.
//
foreach(array('host'  =>'smtp.example.com',
              'port'  =>'25',
              'user'  =>'',
              'passwd'=>'',
	      'noreply'=>'noreply@example.com',
	      ) as $item=>$v){
  $GLOBALS["bMailer_$item"] = $v;
}

//
// Settings for the IMAP mail server(s)
//
$GLOBALS['imapServers'] = array('example.com' => array('imap'=>array('ssl://imap.example.com','imap.example.com'),
						       'port'=>993,
						       'alias'=>array('example.com','another.example.com')),
                                'gmail.com'   => array('imap'=>array('ssl://imap.gmail.com','imap.gmail.com'),
						       'port'=>993,
						       'alias'=>array('gmail.com'))
				);
