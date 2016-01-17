<?php
//
// Settings for the MySQL database.
// The database must exist and be writable by the user
//
foreach(array('host'  =>'localhost:3306',
	      'dbname'=>'combodb',
              'user'  =>'username',
              'passwd'=>'secret',
	      ) as $item=>$v){
  $GLOBALS["bDB_myPear_$item"] = $v;
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
