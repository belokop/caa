<?php

myPear_startup_queue('wiw_updates');

function wiw_updates(){
  if (bMaster()->register_module_update_script(WIW_MODULE,WIW_VERSION,__FILE__)){
    wiw_update01();
    wiw_update02();
  }
}

function wiw_update02(){
  // Null
  myPear_update_DBdefaults(array('wiw_trips'=>array('int(11)' =>array('t_idf','t_idn','t_departure','t_arrival',
								      't_avid','t_orgid','t_type'),
						    'varchar(32)' =>array('t_acl'),						    
						    'text'=>array('t_comment','t_contact'),
						    )),
			   'NULL');
}

function wiw_update01(){
  myPear_db()->qquery("DELETE FROM `zzz_lists` WHERE l_id = 7473",1);
  foreach(array('t_idf','t_idn') as $f){
    if (!myPear_db()->columnExists($f,'wiw_trips')){
      myPear_db()->qquery("ALTER TABLE `wiw_trips` ADD `$f` INT NOT NULL AFTER `t_id`",1); 
      myPear_db()->reset_cache();
    }
  }
  if (!myPear_db()->columnExists('t_acl','wiw_trips')){
    myPear_db()->qquery("ALTER TABLE `wiw_trips` ADD `t_acl` VARCHAR( 130 ) NOT NULL DEFAULT '".wiw_acl_public."' AFTER `t_type`",1); 
    myPear_db()->reset_cache();
  }
}
