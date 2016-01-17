<?php

myPear_startup_queue('prp_updates');

function prp_updates(){
  if (bMaster()->register_module_update_script(PRP_MODULE,PRP_VERSION,__FILE__)){
    prp_update_01();
    prp_update_02();
    prp_update_03();
  }
}

/*
 * MySQL is getting strict...
 */
function prp_update_03(){
  // Null
  myPear_update_DBdefaults(array('prp_list'=>array('tinytext' =>array('prp_authors','prp_title'),
						   'int(11)'  =>array('prp_orgid','prp_avid','prp_report','prp_day0','prp_tm'),
						   )),
			   'NULL');
}

/*
 *
 */
function prp_update_02(){

  myPear_db()->qquery("UPDATE `zzz_avatars` SET `av_lastname` = 'Unknown' WHERE `av_lastname` REGEXP 'Anonymous'",True);

  $q = myPear_db()->qquery("SELECT * FROM `prp_list`  WHERE `prp_archive` REGEXP 'Own_Papers/Koso.Lund.Bran12.pdf'",True);
  while ($r=myPear_db()->next_record($q)){
    $n = str_replace('Own_Papers','Own_Papers/D/2012',$r['prp_archive']);
    myPear_db()->qquery("UPDATE `prp_list` SET `prp_archive` = '$n' WHERE `prp_id` = $r[prp_id]",True);
  }
  myPear_db()->qquery("UPDATE `prp_list` SET `prp_local` = '200368.pdf' WHERE `prp_local` = '200368.PDF'",True);
  myPear_db()->qquery("UPDATE `prp_list` SET `prp_status` = '".PRP_CANCELED."' WHERE `prp_status` = 'free'",True);
  myPear_db()->qquery("UPDATE `prp_list` SET `prp_status` = '".PRP_RESERVED."' WHERE `prp_status` = 'registered'",True);
}

/*
 *
 */
function prp_update_01(){
  myPear_db()->reset_cache();
  if (!myPear_db()->tableExists('prp_list'))
    myPear_db()->qquery("CREATE TABLE `prp_list` (
			 `prp_id` INT NOT NULL AUTO_INCREMENT PRIMARY KEY ,
			 `prp_authors` TEXT NOT NULL ,
			 `prp_title` TEXT NOT NULL ,
			 `prp_field` VARCHAR( 8 ) NULL DEFAULT NULL ,
			 `prp_orgid` INT NULL ,
			 `prp_avid` INT NULL ,
                         `prp_report` INT NULL ,
			 `prp_day0` INT NOT NULL ,
			 `prp_local` VARCHAR( 200 ) NULL DEFAULT NULL ,
			 `prp_status` VARCHAR( 16 ) NULL DEFAULT NULL ,
			 `prp_archive` VARCHAR( 200 ) NULL DEFAULT NULL ,
			 `prp_doi` VARCHAR( 200 ) NULL DEFAULT NULL ,
			 `prp_publisher` VARCHAR( 200 ) NULL DEFAULT NULL ,
			 `prp_tm` INT NOT NULL ,
                         `prp_timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
			 INDEX ( `prp_avid` , `prp_day0` , `prp_tm`)
			 );",True);
  myPear_db()->reset_cache();
}
