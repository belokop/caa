<?php

ini_set("memory_limit",-1);
myPear_startup_queue('jam_update13');
myPear_startup_queue('jam_updates');

function jam_updates(){
  foreach(array('jam_applicants',
		'jam_documents',
		'jam_positions',
		'jam_recommendations',
		'jam_reviews',
		) as $t) if (!myPear_db()->tableExists($t)) return;
  
  if (bMaster()->register_module_update_script(JAM_MODULE,JAM_VERSION,__FILE__)){
    bTiming()->cpu(__FUNCTION__);
    
    set_time_limit(0);
    
    jam_update01();
    jam_update02();
    jam_update03();
    jam_update04();
    jam_update05();
    jam_update06(); 
    jam_update07();
    jam_update08();
    jam_update09();
    jam_update10();
    jam_update11();
    jam_update12();
    jam_update13();
    bTiming()->cpu();
  }
}

/*
 * Fixes...
 */
function jam_update13(){
  foreach(array('phd','ms','nation') as $i) myPear_db()->qquery("ALTER TABLE `jam_positions` CHANGE `pos_req_$i` `pos_req_$i` VARCHAR(8) NULL DEFAULT 'yes'",True);

  myPear_update_DBdefaults(array('jam_documents' =>array('int(11)' =>array('doc_apid','doc_date','doc_lmid'))));
  
  myPear_update_DBdefaults(array('jam_recommendations'=>array('int(11)' =>array('rec_apid','rec_avid'),
							      'varchar(128)'=>array('rec_letter_file')),
				 'jam_reviews'   =>array('int(11)' =>array('r_apid','r_avid')),
				 'jam_rf'        =>array('int(11)' =>array('rf_posid','rf_lmid')),
				 'jam_positions' =>array('tinytext'=>array('pos_cm_secretary'),
							 'int(11)' =>array('pos_status','pos_orgid','pos_req_phd_year'),
							 'varchar(8)'  =>array('pos_ap_noreply','pos_av_noreply'),
							 'varchar(32)' =>array('pos_nickname','pos_type','pos_sc_maybe_referee'),
							 'varchar(128)'=>array('pos_logo','pos_name','pos_email'),
							 ),
				 'jam_applicants'=>array('text'    =>array('ap_comment'),
							 'int(11)' =>array('ap_rating','ap_status','ap_advisor_avid',
									   'ap_rfid','ap_posid','ap_avid','ap_submitdate'),
							 ),
				 ),
			   'NULL');
  myPear_db()->qquery("ALTER TABLE `jam_applicants` CHANGE `ap_status` `ap_status` INT( 11 ) NULL DEFAULT '0'",True);
  myPear_db()->qquery("UPDATE jam_applicants SET ap_status = 0 WHERE ap_status IS NULL",True);
}

/*
 * Ad-hoc...
 * Remove bogus records
 */
function jam_update12(){

  // Remove bogus records
  $q = myPear_db()->qquery("SELECT ap_id,rec_id FROM jam_recommendations LEFT JOIN jam_applicants ON rec_apid = ap_id WHERE ap_id IS NULL",True);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM jam_recommendations WHERE rec_id = $r[rec_id]",True);
  }

  // Not need 'pos_namesc3'...
  if (!cnf_CLI && myPear_db()->columnExists('pos_namesc3','jam_positions')){
    myPear_db()->qquery('ALTER TABLE jam_positions DROP `pos_namesc3`',True);
    myPear_db()->reset_cache();
  }

  // Fix naming convention
  if (!myPear_db()->columnExists('pos_orgid','jam_positions')){
    myPear_db()->qquery("ALTER TABLE `jam_positions` CHANGE `pos_fid` `pos_orgid` INT(32) NOT NULL DEFAULT '0'",True);
    myPear_db()->reset_cache();
  }
}

/*
 * Status is integer
 */
function jam_update11(){
  foreach(array('jam_positions' =>'pos_status',
		'jam_applicants'=>'ap_status') as $t=>$f){
    if (myPear_db()->columnExists($f,$t)){
      $type = myPear_db()->getColumnType($t,$f);
      if (strToLower($type) != 'int'){
	myPear_db()->qquery("ALTER TABLE `$t` CHANGE `$f` `$f` INT NULL DEFAULT NULL",1);
      }
    }
  }
}

/*
 * Change the definition of "too old phd"
 */
function jam_update10(){
  foreach(array('pos_req_phd','pos_req_ms') as $f){
    if (stripos(myPear_db()->getColumnType('jam_positions',$f,True),'enum') !== False){
      myPear_db()->qquery("ALTER TABLE `jam_positions` CHANGE `$f` `$f` VARCHAR( 64 ) NULL",1); 
    }
  }

  // we have 'yes but' now, hence remove 'enum'
  if (!cnf_CLI){
    $q=myPear_db()->qquery("UPDATE `jam_positions` SET pos_req_phd_year = 0 WHERE pos_req_phd_year = 1970",1);
    $q=myPear_db()->qquery("UPDATE `jam_positions` SET pos_req_phd_year = 5 WHERE pos_req_phd_year > 2000",1);
    if (myPear_db()->num_rows($q) > 0)
      myPear_db()->qquery("ALTER TABLE `jam_positions` CHANGE `pos_req_phd_year` `pos_req_phd_year` INT( 11 ) NOT NULL DEFAULT '0'",1);
    $q=myPear_db()->qquery("UPDATE jam_positions SET pos_req_phd='".JAM_yes_but."' WHERE pos_req_phd_year > 0",1);
  } 
}

function jam_update09(){
  if (!cnf_CLI){
    $q = myPear_db()->qquery("SELECT av_ssn,av_birthdate FROM zzz_avatars WHERE av_ssn NOT REGEXP '[0-9][0-9][0-9][0-9][0-9][0-9]-.[0-9][0-9][0-9]' GROUP BY av_ssn",1);
    while($r=myPear_db()->next_record($q)){
      $av_birthdate = $r['av_birthdate'];
      if (preg_match('/^(19|20)([0-9][0-9][0-9][0-9][0-9][0-9])([0-9][0-9][0-9][0-9])$/',($n=trim($r['av_ssn'])),$m)){
	$ssn = x("'",$m[1].$m[2].'-'.$m[3]);
      }elseif (preg_match('/^([0-9][0-9])-([0-9][0-9])-(19[0-9][0-9])$/',($n=trim($r['av_ssn'])),$m)){
	$ssn = x("'",$m[3].'-'.$m[2].'-'.$m[1].'-P123');
      }elseif (preg_match('/^[0-9][0-9][0-9][0-9][0-9][0-9]$/',($n=trim($r['av_ssn'])))){
	$ssn = "'$n-P123'";
      }else{
	$ssn = 'NULL';
	if ((int)$r['av_ssn'] > 1900 && (int)$r['av_ssn'] < 2000) $r['av_birthdate'] = $r['av_ssn'];
      }
      if (!(int)$av_birthdate) $av_birthdate = 'NULL';
      MSG::DEBUG(sprintf("ssn: '%s' <== '%s', av_birthdate: '%s'",$ssn,$r['av_ssn'],$r['av_birthdate']));
      myPear_db()->qquery("UPDATE zzz_avatars SET  av_ssn=$ssn,av_birthdate=$av_birthdate WHERE av_ssn = '$r[av_ssn]'",1);
    }
  }
}

function jam_update08(){
  myPear_update_addIndex('jam_recommendations',array('rec_apid','rec_avid'));
  myPear_db()->query("ALTER TABLE `jam_recommendations` CHANGE `rec_comment` `rec_comment` TEXT NULL DEFAULT NULL ",$stopIfFails=False);
}

function jam_update07(){
  
  if (myPear_db()->tableExists('jam_positions')){
    if (!cnf_CLI && myPear_db()->columnExists('pos_n_referees_max','jam_positions')){
      myPear_db()->qquery('ALTER TABLE jam_positions DROP `pos_n_referees_max`',True);
      myPear_db()->reset_cache();
    }
    
    if (!myPear_db()->columnExists('pos_req_ms','jam_positions')){
      myPear_db()->qquery("ALTER TABLE `jam_positions` ADD `pos_req_ms` ENUM('yes','no','bonus') NOT NULL DEFAULT 'yes' AFTER `pos_req_phd_year`",1); 
      myPear_db()->reset_cache();
      myPear_db()->qquery("UPDATE `jam_positions` SET pos_req_ms='bonus' WHERE pos_req_ms = '' OR pos_req_ms IS NULL",1);
    }
  }

  if (myPear_db()->tableExists('jam_applicants') && myPear_db()->tableExists('jam_recommendations')){
    
    myPear_db()->qquery('ALTER TABLE `jam_reviews` CHANGE `r_rating` `r_rating` TINYINT NULL DEFAULT NULL',True); 
    
    if (!myPear_db()->columnExists('ap_ip','jam_applicants')){
      myPear_db()->qquery("ALTER TABLE `jam_applicants` ADD `ap_ip` VARCHAR( 32 ) NULL DEFAULT NULL AFTER `ap_submitdate`",1); 
      myPear_db()->reset_cache();
    }
    
    if (!myPear_db()->columnExists('ap_dossier_url','jam_applicants')){
      myPear_db()->qquery("ALTER TABLE `jam_applicants` ADD `ap_dossier_url` VARCHAR(244) NULL DEFAULT NULL AFTER `ap_comment` ",1);
      myPear_db()->reset_cache();
    }
    
    if (!myPear_db()->columnExists('rec_ip','jam_recommendations') && myPear_db()->columnExists('rec_askdate','jam_recommendations')){
      myPear_db()->qquery("ALTER TABLE `jam_recommendations` ADD `rec_ip` VARCHAR( 32 ) NULL DEFAULT NULL AFTER `rec_askdate`",1); 
      myPear_db()->reset_cache();
    }
  }
}

/*
 * Major update,
 * - start bList_jam_requiredDocs
 * - start jam_Document, replace comma separated fild by bList, 
 */
function jam_update06(){
  bTiming()->cpu(__FUNCTION__);

  if (myPear_db()->columnExists('pos_status','jam_positions')){
    locateAndInclude('bList_jam_requiredDocs');
    locateAndInclude('bList_jam_subPositions');
    fix_subPositions();
    fix_requiredDocs();
  }
  
  foreach(array('requiredDocs',
		'subPositions',
		) as $l){
    myPear_db()->qquery("UPDATE `zzz_lists` SET l_class='bList_jam_$l' WHERE l_class = 'bList_$l'",1);
  }
  
  bTiming()->cpu();
}

/*
 * Minor update
 */
function jam_update05(){
  if (myPear_db()->columnExists('u_name','zzz_units')){
    myPear_db()->query("UPDATE zzz_units     SET u_name      = 'Faculty'             WHERE u_name      REGEXP 'Faculty'");
    myPear_db()->query("UPDATE zzz_units     SET u_name      = 'Research Committee'  WHERE u_name      REGEXP 'SC section2|Research Committee'");
  }
  myPear_db()->query("UPDATE jam_positions SET pos_namesc1 = 'Faculty'             WHERE pos_namesc1 REGEXP 'Faculty'");
  myPear_db()->query("UPDATE jam_positions SET pos_namesc2 = 'Research Committee'  WHERE pos_namesc2 REGEXP 'SC section2|Research Committee'");
  
  myPear_db()->query("UPDATE jam_positions SET pos_nickname = 'phdAB-2012'  WHERE pos_nickname = 'ap2012'");
  myPear_db()->query("UPDATE jam_positions SET pos_nickname = 'phdEA-2011'  WHERE pos_nickname = 'eddy-2011'");
  myPear_db()->query("UPDATE jam_positions SET pos_nickname = 'phdBUB-2009' WHERE pos_nickname = 'bubphd-2009'");
  myPear_db()->query("UPDATE jam_positions SET pos_nickname = 'phdBUB-2010' WHERE pos_nickname = 'bubJP-2010'");
  myPear_db()->query("UPDATE jam_positions SET pos_nickname =  'vphd-2010'  WHERE pos_nickname = 'vfellows-2010'");
  myPear_db()->query("UPDATE jam_positions SET pos_nickname =  'vphd-2011'  WHERE pos_nickname = 'vfellows-2011'");
  myPear_db()->query("UPDATE jam_positions SET pos_nickname =  'vphd-2012'  WHERE pos_nickname = 'cfellows-2012'");
}

/*
 * Minor update
 */
function jam_update04(){}

/*
 * Minor update
 */
function jam_update03(){
  foreach(array('jam_reviews'=>array('r_comment'),
		'zzz_avatars'=>array('av_lastname')) as $table=>$fields){
    foreach($fields as $field){
      foreach(array('D.*Hoker'=>"D'Hoker",
		    ) as $fr=>$to){
	$toe = myPear_db()->quote($to,True,$field);
	myPear_db()->query('UPDATE '.$table.' SET '.$toe.' WHERE '.$field.' REGEXP \'[[:<:]]'.$fr.'[[:>:]]\'');
      }
    }
  }
}

/*
 * Major update, make pos_type a checkbox
 */
function jam_update01(){
  if (myPear_db()->tableExists('jam_positions')){
    myPear_db()->query("UPDATE jam_positions SET pos_type = 'yes' WHERE pos_type = 'jam'");
    myPear_db()->query("UPDATE jam_positions SET pos_type = 'yes' WHERE pos_type = 'am'");
    myPear_db()->query("UPDATE jam_positions SET pos_type = 'no'  WHERE pos_type = 'other'");
  }
}

/*
 * Minor update
 */
function jam_update02(){
  foreach(array('jam_positions','jam_old_positions') as $table){
    if (myPear_db()->tableExists($table)){
      foreach(array(
		    'masterclass-2009' =>'masterclass-2009',
		    'prof2010'      =>'prof-2010',
		    'AD ass.prof.'  =>'assprofAB-2009',
		    'Ass.Prof-2009' =>'assprof-2009',
		    'postdocAP-2011'=>'postdocAB-2011',
		    'ADpostdoc-2009'=>'postdocAB-2009',
		    'fellows-2008'  =>'fellows-2008',
		    'Fellows-2009'  =>'fellows-2009',
		    'Fellows-2010'  =>'fellows-2010',
		    'fellows-2011'  =>'fellows-2011',
		    'AD PhD'        =>'phdAB-2009',
		    'vfellows2010'  =>'vfellows-2010',
		    'vfellows2011'  =>'vfellows-2011',
		    'vphd2011'      =>'vphd-2011',
		    'eddy2011'      =>'eddy-2011',
		    'bubPHD'        =>'bubphd-2009',
		    'php-QS'        =>'bubJP-2010',
		    'postdocCC2011' =>'bubCC-2011',
		    ) as $f=>$t){
	myPear_db()->qquery("UPDATE $table SET pos_nickname = '$t' WHERE pos_nickname = '$f'",1);
      }
    }
  }
}
