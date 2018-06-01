<?php
//
// Settings for the MySQL database.
// The database must exist and be writable by the user
//

// Database engine class (NOT TO BE CHANGED!)
$GLOBALS['DBengine'] = $DBengine = 'bDB_myPear';

// Access parameter
foreach(array('host'  =>'localhost',
	      'dbname'=>'combodb',
              'user'  =>'username',
              'passwd'=>'secret',
	      ) as $item=>$v){
  $GLOBALS[$DBengine][$item] = $v;
}

//
// Settings for the SMTP mail server
// Set user/passwd to the valid username/password if your server uses authentication.
//
foreach(array('host'   =>'smtp.example.org',
              'port'   =>'25',
              'user'   =>'',
              'passwd' =>'',
	      'noreply'=>'noreply@example.org',
              ) as $item=>$v){
  $GLOBALS["bMailer_$item"] = $v;
}
