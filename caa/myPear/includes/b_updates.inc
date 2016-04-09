<?php
/*
 * myPear core modules updates,
 * to be applied when myPear class is instantiated, otherwise wait in the queue
 */
if (myPear::$isReady)  myPear_updates();
else     myPear_startup_queue('myPear_updates');

function myPear_updates(){
  
  if (bMaster()->register_module_update_script(myPear_MODULE,myPear_VERSION,__FILE__)){
    bTiming()->cpu(__FUNCTION__);
    myPear_db()->noAutoCommit('start');
    
    // Updates for myPear_2
    // not needed since 2011-10-01, when the production version was upgraded to 3.0
    if (False){
      myPear_update01();
      myPear_update02();
      myPear_update03();
      myPear_update04();
    }

    // Updates for myPear_3.0,
    // not needed since 2012-05-01, when the production version was upgraded to 3.16
    if (False){
      locateAndInclude('bList');
      fix_bList();
      
      myPear_update05();
      myPear_update06();
      myPear_update07();
      myPear_update08();
      myPear_update09();
      myPear_update10();
      myPear_update11();
    }
    
    
    // Updates for myPear_3.16, 2012-07-25
    if (False){
      myPear_update12();
      myPear_update13();
    }

    // Updates for myPear_3.17, 2012-08-30
    if (False){
      myPear_update14();
      myPear_update15();
      myPear_update16();
      myPear_update17();
      myPear_update19();
      myPear_update20();
      myPear_update21();
      myPear_update22();
      myPear_update23();
      myPear_update24();
      myPear_update25();
      myPear_update26();
      myPear_update27();
    }

    if (False){
      // Updates for myPear_5.1, 2015-01-05
      myPear_update25();
      myPear_update28();
      myPear_update29();
      myPear_update30();
    }
    
    if (True){
      // Updates for myPear_5.72, 2015-04-15
      myPear_update31();
    }

    if (!@$_SESSION['updates'][__METHOD__]++){
      //
      // Countries always come and go...
      //
      if (myPear_db()->tableExists('zzz_countries')){
	foreach (array(array('country_code'     => 'SZ',
			     'country_continent'=> 'Africa',
			     'country_name'     => 'Swaziland'),
		       ) as $c){
	  $q = myPear_db()->qquery("SELECT *  FROM zzz_countries WHERE country_code = '$c[country_code]'");
	  if (!myPear_db()->num_rows($q)){
	    myPear_db()->qquery("INSERT INTO zzz_countries SET ".b_fmt::joinX(', ',$c,"'"),1);
	  }
	}
      }
      // Drop the local legacy sql table, not used any more
      if (myPear_db()->tableExists('legacy_maildb')) myPear_db()->qquery("DROP TABLE `legacy_maildb`");
    }
    myPear_db()->noAutoCommit('end');
    bTiming()->cpu();
  }
}

/*
 *
 */
function myPear_update31(){

  // Clean bogus records
  myPear_db()->qquery("DELETE FROM zzz_unit_members WHERE (um_avid IS NULL OR um_avid = '' OR um_avid = 0)",True);
  
  // Update sql definitions
  myPear_update_DBdefaults(array('zzz_variables'    =>array('text'        =>array('var_value')),
				 'zzz_lists'        =>array('varchar(200)'=>array('l_name')),
				 'zzz_unit_members' =>array('text'        =>array('um_option')),
				 'zzz_avatars'      =>array('int(11)'     =>array('av_ddate'),
							    'text'        =>array('av_http',),
							    'varchar(4)'  =>array('av_sex'),
							    'varchar(8)'  =>array('av_birthcountry'),
							    'varchar(32)' =>array('av_ssn','av_von','av_salutation','av_citizenship','av_residentship',
										  'av_ms_country','av_phd_country','av_zip'),
							    'varchar(64)' =>array('av_phone','av_city','av_pwd'),
							    'varchar(128)'=>array('av_photo','av_email','av_firstname','av_lastname',
										  'av_position','av_position'),
							    'varchar(230)'=>array('av_birthplace','av_pwd2','av_identity','av_institute','av_ms_institute',
										  'av_address','av_institute','av_pwd2','av_phd_institute')),
				 'zzz_organizations'=>array('varchar(230)'=>array('org_affil','org_domain','org_signature_dir'),
							    'varchar(128)'=>array('org_code','org_name'),
							    'varchar(32)' =>array('org_country','org_name_short','org_theme'))),
			   'NULL');
  myPear_update_DBdefaults(array('zzz_units'        =>array('varchar(128)'=>array('u_name','u_class','u_member_title')),
				 'zzz_list_members' =>array('varchar(128)'=>array('lm_key','lm_value','lm_status')),
				 'zzz_logs'         =>array('varchar(128)'=>array('log_ip','log_type','log_api','log_comment','log_org'))),
			   'NULL');
  
}

/*
 * Remove spam entries from the database
 */
function myPear_update30(){

  //
  // ================================================ case 1
  // The known spammers have av_firstname=av_lastname...
  //
  $where_fl[] = 'av_firstname=av_lastname';
  $where_fl[] = 'av_firstname != "unknown"';
  $where_fl[] = 'av_firstname != "x"';
  $where_fl[] = 'av_firstname != "li"';
  $where_fl[] = 'av_firstname != "S.v.M"';
  $where_bad_fl = join(' AND ',$where_fl);
  
  // ... and a strange institute name
  $where_[] = 'av_institute REGEXP "(http:|we have also seen|between great and greatest|row is sent off)"';
  $where_[] = 'av_institute = ""';
  $where_[] = 'av_institute IS NULL';
  $where_[] = 'av_institute = av_firstname';
  $where_bad_inst = '('.join(' OR ',$where_).')';
  
  // Delete those
  bForm_Avatar::delete(join(' AND ',array($where_bad_fl,$where_bad_inst)));
  
  //
  // ================================================ case 2
  // Various mistypes
  //
  static $es = array('','//','-','---','a','b','j','c','sss','none','longchamps','jingbo','nadolsky.physics.smu.edu',
                     'nobody@worldnet.att.net','optp@worldnet.att.net','info@nabu.de','biuro@ceramik.com.pl','suzanne@ccnm.ac.nz','cia.castro@bbcr.com.br',  // spam      
                     'asdjf@asg.com','not.available@somewhere.org','n@a',"none@none","mtrichas@",
                     'michael.bradleyphysics.umu.se','terje.larsen','j.m','daniele.marmiroli','marisol.alcantaraortigoza','ea212cam.ac.uk',
                     "hebrew.azuka" ,"sanjibdey4@gmail.","jsdiazpo","jafaroshriyeh","tob",
                     'kashi-sale.com','799fu.com','chanelforsalejp.org','cocochaneljapan.com',
                     );
  $where = array("av_email IS NULL ",
                 "av_email REGEXP 'https?:'");
  foreach ($es as $e) $where[] = "av_email = '$e'";
  
  // Delete those
  bForm_Avatar::delete(join(' OR ',$where));
}


//
// minor cleanup
//
function myPear_update29(){
  // Clean log file, remove duplicate message about software updates
  $q = myPear_db()->query("SELECT * FROM zzz_logs WHERE log_type = 'update' ORDER BY log_time");
  $dejaVu = array();
  while($r = myPear_db()->next_record($q)){
    $flag = '';
    if (@$dejaVu[$r['log_comment']]++){
      $flag = 'xxxxx';
      myPear_db()->qquery("DELETE FROM zzz_logs WHERE log_id = $r[log_id]",True);
    }
    myPear::DEBUG(sprintf("update %s %s %-15s\n",date('Y-m-d',$r['log_time']),$r['log_comment'],$flag));
  }

  // clean crontab avatars, those were replaced by "noreply@<domain>"
  bForm_Avatar::delete("av_email REGEXP 'cronjob'");

  // clean test records
  bForm_Avatar::delete("av_lastname='test' AND av_firstname='test'");
}

function myPear_update_zzz_lists($module){
  $b_reg = b_reg::get($module);
  //  b_debug::var_dump($b_reg['classes']);
  foreach(preg_grep('/bList/',array_keys($b_reg['classes'])) as $list){
    if (empty($b_reg['classes'][$list]['dd'])) $b_reg['classes'][$list]['dd'] = $b_reg['classes'][$list]['d'];
    foreach(array('d'=>'l_name','dd'=>'l_member_title') as $k=>$v){
      $should_be = $b_reg['classes'][$list][$k];
      $q = myPear_db()->query("SELECT $v FROM zzz_lists ".($where="WHERE l_class = '$list' AND $v != '$should_be'"));
      if (myPear_db()->num_rows($q)) myPear_db()->qquery("UPDATE zzz_lists SET $v='$should_be' $where",True);
    }
  }
}

/*
 *
 */
function myPear_update_addIndex($table,$field,$unique=False){
  if (cnf_CLI) return;
  if (!is_array($field)) $field = array($field);
  // Drop index
  foreach($field as $keyname){
    for($n=0;$n<65;$n++){
      $i = preg_replace('/\(.*\)/','',$keyname).($n==0 ? '' : '_'.$n);
      if (in_array($i,myPear_db()->showIndex($table))) myPear_db()->qquery("ALTER TABLE $table DROP INDEX $i",True);
    }
  }
  // Add index
  $keytype = ($unique ? 'UNIQUE' : 'INDEX');
  $keyname = preg_replace('/\(.*\)/','',$field[0]);
  myPear_db()->qquery("ALTER TABLE `$table` ADD $keytype $keyname (".join(',',$field).")",True);
}

function myPear_update_DBdefaults($args,$null='NOT NULL'){
  foreach($args as $table=>$fff){
    if (!myPear_db()->tableExists($table)) continue;
    foreach($fff as $type_to_be=>$ff){
      if ($set_to_null=(strToLower($null) === 'null')){
	$null_to_be = 'YES';
	$default_to_be = 'NULL';
	$default = '';
      }else{
	$null_to_be = 'NO';
	$default_to_be = (stripos($type_to_be,'char')===False ? "0" : "''");
	$default = "DEFAULT $default_to_be";
      }
      foreach($ff as $f){
	if (!myPear_db()->columnExists($f,$table)) continue;
	if (stripos($type_to_be,'int') !== False) myPear_db()->qquery("UPDATE `$table` SET $f=0 WHERE $f=''",True);
	myPear_db()->qquery("ALTER TABLE `$table` CHANGE `$f` `$f` $type_to_be $null $default",True);
	if (stripos($type_to_be,'int') !== False) myPear_db()->qquery("UPDATE `$table` SET $f=$default_to_be WHERE ($f='' OR $f IS NULL OR $f='0' OR $f=0)",True);
      }
    }
  }
}



function myPear_update28(){
  locateandInclude('bList');
  //
  // Clean-up
  myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_key='' AND lm_value=''",1);
  if (myPear_db()->columnExists('av_birthdate','zzz_avatars'))
    myPear_db()->qquery("UPDATE zzz_avatars SET av_birthdate = av_ssn WHERE av_ssn > '0' AND av_ssn < '2015' AND av_birthdate='0'",True);
  if (myPear_db()->columnExists('av_ssn','zzz_avatars'))
    myPear_db()->qquery("UPDATE zzz_avatars SET av_ssn = '' WHERE av_ssn > '0' AND av_ssn < '2015'",True);
  
  // Not used... But it was a good idea...
  foreach(array('zzz_avatars'      => 'av_phd_adviser_avid',
		'zzz_list_members' => 'lm_locale',
		'zzz_lists'        => 'l_locale',
		'zzz_units'        => 'u_locale') as $table=>$column){ 
    if (myPear_db()->columnExists($column,$table)){
      myPear_db()->qquery("ALTER TABLE `$table` DROP `$column`",True);
      myPear_db()->reset_cache();
    }
  }
  
  // Clean-up the spurious lists
  $q = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_parent NOT REGEXP '^[0-9]+$'",True);
  while($r=myPear_db()->next_record($q)) bList::deleteList($r['l_id']);
  
  // Logs clean-up
  foreach(array('node','none','',) as $m) myPear_db()->qquery("DELETE FROM zzz_logs WHERE log_api='$m'",True);
  myPear_db()->qquery("UPDATE zzz_logs SET log_type='reminder' WHERE log_type='info'",True);
  
  //
  // fixes...
  //  
  myPear_update_DBdefaults(array('zzz_lists'        =>array('varchar(200)'=>array('l_name'),
							    'int(11)'     =>array('l_parent')),
				 'zzz_unit_members' =>array('mediumtext'  =>array('um_option'),
							    'int(11)'     =>array('um_uid','um_avid')),
				 'zzz_avatars'      =>array('text'        =>array('av_ms_institute','av_phd_institute'),
							    'varchar(32)' =>array('av_citizenship','av_residentship','av_ms_country','av_phd_country','av_zip'),
							    'varchar(64)' =>array('av_city','av_pwd'),
							    'varchar(230)'=>array('av_pwd2','av_identity','av_institute')),
				 'zzz_organizations'=>array('varchar(230)'=>array('org_affil','org_domain'),
							    'varchar(128)'=>array('org_code'),
							    'tinytext'    =>array('org_signature_dir'))),'NULL');
  myPear_update_DBdefaults(array('zzz_units'        =>array('varchar(128)'=>array('u_name','u_class','u_member_title')),
				 'zzz_list_members' =>array('varchar(128)'=>array('lm_key','lm_value','lm_status')),
				 'zzz_logs'         =>array('varchar(128)'=>array('log_ip','log_type','log_api','log_comment','log_org'),
							    'int(11)'     =>array('log_avid','log_form','log_time'))));
  //
  // Optimization
  //
  myPear_update_addIndex('zzz_organizations',array('org_code(128)'),True);
  myPear_update_addIndex('zzz_avatars',array('av_email','av_email2(200)'),True);
  myPear_update_addIndex('zzz_crypt','c_value(230)',True);
  myPear_update_addIndex('zzz_logs','log_form');
}

/*
 * Fix for a more strict convension - no arreys in the packed options
 */
function myPear_update27(){
  foreach(array('zzz_unit_members','zzz_list_members') as $table){
    foreach(array('lm_option'=>'lm_id',
		  'um_option'=>'um_id',
		  ) as $column=>$lm_id){
      if (myPear_db()->columnExists($column,$table)){
	$q=myPear_db()->qquery("SELECT $lm_id,$column FROM $table",1);
	while($r=myPear_db()->next_record($q)){
	  if (empty($r[$column])){
	    $f = array();
	  }else{
	    $e = error_reporting(0);  $f = unserialize($r[$column]);   error_reporting($e);
	    if (!is_array($f)){
	      if (cnf_dev) b_debug::var_dump($r[$column],"- $table.$column not serialized, leave it as it is");
	      $f = array();
	    }
	  }
	  $OK = True;
	  foreach(array_keys($f) as $k){
	    $v = $f[$k];
	    if(is_array($v)){
	      // b_debug::xxx("array $column($k=".join(',',$v).')');
	      $f[$k] = join(',',$v);
	      $OK = False;
	    }
	  }
	  if (!$OK) myPear_db()->query("UPDATE $table SET $column='".serialize($f)."' WHERE $lm_id = ".$r[$lm_id]);
	}
      }
    }
  }
}

/*
 * Introduce 'org_name_short' column (Organization short name)
 */
function myPear_update26(){
  
  if (myPear_db()->tableExists('zzz_organizations')){
    if (myPear_db()->columnExists('org_nickname','zzz_organizations')){
      myPear_db()->qquery("ALTER TABLE `zzz_organizations` CHANGE `org_nickname` `org_code` VARCHAR(32) NULL",1);
      myPear_db()->reset_cache();
    }
    
    if (!myPear_db()->columnExists('org_name_short','zzz_organizations')){
      myPear_db()->qquery("ALTER TABLE `zzz_organizations` ADD `org_name_short` VARCHAR( 32 ) NOT NULL AFTER `org_name`",1);
      myPear_db()->reset_cache();
      foreach(array('an'     => 'AlbaNova',
		    'nordita'=> 'Nordita',
		    'fysikum'=> 'Fysikum',
		    'kth'    => 'KTH',
		    'okc'    => 'OKC',
		    'vh'     => 'Vetenskapenshus',
		    'gu'     => 'GU',
		    ) as $org_code=>$org_name_short){
	myPear_db()->qquery("UPDATE `zzz_organizations` SET org_name_short='$org_name_short' WHERE org_code='$org_code'",1);
	
      }
    }
  }
}

//
// Tidy up the emails
function myPear_update25(){

  $q = myPear_db()->qquery("SELECT av_email,av_email2,av_id FROM zzz_avatars WHERE av_email REGEXP 'sakura.schafer..+nameki@kcl.ac.uk'",True);
  $n = 0;
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("UPDATE zzz_avatars SET av_email='sakura.schafer-nameki".(++$n)."@kcl.ac.uk' WHERE av_id = $r[av_id]",True);
    b_debug::print_r($r);
  }

  $q = myPear_db()->qquery("SELECT av_email,av_email2,av_id FROM zzz_avatars WHERE av_email REGEXP ',|;' OR av_email2 REGEXP ',|;| ' OR av_email2 = '' OR av_email2 IS NULL",True);
  while($r = myPear_db()->next_record($q)){
    if (empty($r['av_email2'])) $r['av_email2'] = '';

    $old_mail_array = array();
    foreach(preg_split('/[,;]/',$r['av_email'])  as $e)  if (trim($e)) $old_mail_array[] = $e;
    foreach(preg_split('/[, ]/',$r['av_email2']) as $e)  if (trim($e)) $old_mail_array[] = $e;
    $old_mail_array = array_unique($old_mail_array);
    $old_mail = join(',',$old_mail_array);

    $tidy_email = bForm_Avatar::tidy_email(join(',',$old_mail_array));
    sort($old_mail_array);
    $old_email = join(',',$old_mail_array);

    if (empty($tidy_email)){
      myPear::WARNING("Can't parse '$old_email'");
    }else{
      myPear_db()->qquery("UPDATE zzz_avatars SET av_email='$tidy_email[av_email]', av_email2='$tidy_email[av_email2]' WHERE av_id=$r[av_id]",True);
      if (($tidy_email['av_email']  != $r['av_email']) 
	  //	  OR ($tidy_email['av_email2'] != $r['av_email2'])
	  ) b_debug::print_r($tidy_email,$old_email);
    }
  }
}

//
//
function myPear_update24(){

  if (!myPear_db()->columnExists('org_country','zzz_organizations')){
    myPear_db()->qquery("ALTER TABLE `zzz_organizations` ADD `org_country` VARCHAR(64) NOT NULL DEFAULT 'SE' AFTER `org_name`",1); 
    myPear_db()->reset_cache();
  }

  if (!myPear_db()->columnExists('av_von','zzz_avatars')){
    myPear_db()->qquery("ALTER TABLE `zzz_avatars` ADD `av_von` VARCHAR(32) NOT NULL DEFAULT '' AFTER `av_firstname`",1); 
    myPear_db()->reset_cache();
  }
  myPear_db()->qquery("ALTER TABLE `zzz_avatars` CHANGE `av_identity` `av_identity` VARCHAR(240) NULL DEFAULT NULL"); 

  // extract the "nobiliary particle"
  static $nobiliary = array('von der', 'von zur', 'von zu', 'von',
			    'zu',
			    'van der', 
			    );
  if (myPear_db()->columnExists('av_von','zzz_avatars')){
    foreach($nobiliary as $av_von){
      $q = myPear_db()->qquery("SELECT * FROM zzz_avatars ".
				 " WHERE av_von = '' AND ".
				 " av_lastname REGEXP '^$av_von( |-)' ",1);
      while($r = myPear_db()->next_record($q)){
	$av_lastname = trim(preg_replace("/^$av_von-?/i","",$r['av_lastname']));
	myPear_db()->qquery("UPDATE zzz_avatars SET av_von = '$av_von', av_lastname = '$av_lastname' WHERE av_id = $r[av_id]",1);
      }
    }
  }
}

//
// Moving to the better bLists
//
function myPear_update23(){
  if(myPear_db()->columnExists('l_name','zzz_lists')){

    foreach(bList::get_all(Null,'WIW_nordita') as $l_id){
      bList::deleteList($l_id);
    }

    foreach(array(
		  'bList_eaEmpRecords'      => 'Employment record',
		  'bList_orgTrades'         => 'Employment types',
		  'bList_jam_requiredDocs'  => 'required documents',
		  'bList_jam_subPositions'  => 'research fields',
		  'bList_vm_agenda'         => 'agenda events',
		  'bList_vm_budgetSource'   => 'budget sources',
		  'bList_vm_cotenants'      => 'Shared accommodation',
		  'bList_vm_projects'       => 'projects',
		  'bList_vm_scholarshipRates' => 'Scholarship rates',
		  'bList_vm_socialEvents'     => 'Social events',
		  'bList_vm_socialEventsSetup'=> 'proposed social events',
		  'bList_wiw_tripTypes'     => 'trip types',
		  'bList_yas_projects'      => 'YAS projects',
		  ) as $l_class=>$l_name){
      $q = myPear_db()->query("SELECT l_id FROM zzz_lists WHERE l_name = '$l_name'");
      $n = myPear_db()->num_rows($q);
      myPear_db()->qquery("UPDATE zzz_lists SET l_class = '$l_class' WHERE l_name = '$l_name'",1);
	
      //      if (!$n && !cnf_CLI)    myPear::WARNING("no match for bList l_name='$l_name'");
    }
  }
  $q = myPear_db()->qquery("SELECT * FROM zzz_lists WHERE l_class IS NULL OR l_class = ''",1);
  $n = myPear_db()->num_rows($q);
  if ($n) myPear::ERROR("$n lists without class");
  while($r = myPear_db()->next_record($q)){
    print_r($r);
  }
}

/*
 * Clean-up the alumni definition
 */
function myPear_update22(){
  myPear_db()->qquery("UPDATE zzz_unit_members SET um_status='".EA_role_employee.EA_role_alumnus."' WHERE um_status='".EA_role_alumnus."'",1);
}

/* 
 * Simplify bList / bUnit
 */
function myPear_update21(){

  // Clean up
  $q=myPear_db()->qquery("SELECT * FROM zzz_units WHERE u_rank = 'RO'",1);
  while($r=myPear_db()->next_record($q)) b_debug::print_r($r,'record');
  
  myPear_db()->qquery("DELETE FROM zzz_units WHERE u_rank = 'RO'",1);
  myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_key='' AND lm_value=''",1);      
  myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_key='' AND lm_value=''",1);      
  $q = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_member_title='department'",1);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM zzz_lists        WHERE   l_id=$r[l_id]",1);
    myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_lid=$r[l_id]",1);
  }
  myPear_db()->qquery("UPDATE zzz_organizations SET org_name='SU/Fysikum' WHERE org_name='Stockholm U/Fysikum'",1);


  // NO. Write module name to the database for the bLists
  foreach(array('zzz_units'=> 'u',
		'zzz_lists'=> 'l',
		) as $t=>$prefix){

    if (myPear_db()->columnExists("${prefix}_module",$t)){
      myPear_db()->query("ALTER TABLE `$t`  DROP `${prefix}_module`");
      myPear_db()->reset_cache();
    }
  }
  
  // Upgrade Organozation
  if (!myPear_db()->columnExists('org_affil','zzz_organizations')){
    myPear_db()->qquery("ALTER TABLE `zzz_organizations` ADD `org_affil` VARCHAR(240) NOT NULL AFTER `org_theme`",1); 
    myPear_db()->reset_cache();
  }

    // Keep u_name
  $type = myPear_db()->getColumnType('zzz_units','u_rank');
  if (strToLower($type) != 'int'){
    myPear_db()->qquery("ALTER TABLE `zzz_units` CHANGE `u_rank` `u_rank` INT NOT NULL"); 
  }

  myPear_db()->qquery("UPDATE zzz_lists SET l_class = 'bList_eaEmpRecords' WHERE l_class = 'bList_ea'",1);
  foreach(array('zzz_units'=>array(//'u_name',
				   //'u_member_title',
				   ),
		'zzz_lists'=>array('l_rank',
				   //'l_name',
				   //'l_member_title',
				   )) as $t=>$ff){
    foreach($ff as $f)
      if (myPear_db()->columnExists($f,$t)) myPear_db()->query("ALTER TABLE `$t` DROP `$f`");
  }
  myPear_db()->reset_cache();
}

// Set the default theme
function myPear_update20(){
  if (cnf_CLI) return;
  if (myPear_db()->columnExists('org_nickname','zzz_organizations')){
    $q = myPear_db()->query("SELECT org_id,org_theme,org_nickname FROM zzz_organizations");
    while ($r = myPear_db()->next_record($q)){
      $update = '';
      if (!empty($r['org_theme']) && !b_cms::themeExists($r['org_theme'])){
	$update = "''";
	$r['org_theme'] = '';
      }
      if ( empty($r['org_theme']) &&  b_cms::themeExists($r['org_nickname'])){
	$update = "'$r[org_nickname]'";
      }
      if (!empty($update)){
	myPear_db()->qquery("UPDATE zzz_organizations SET org_theme = $update WHERE org_id = $r[org_id]",1);
      }
    }
  }
}

/*
 * Ad-hoc updates
 */
function myPear_update19(){
  myPear_db()->qquery("UPDATE zzz_lists SET l_class = 'bList_eaEmpRecords' WHERE l_class = 'bList_ea'",1);
  if (!myPear_db()->columnExists('av_position','zzz_avatars')){
    myPear_db()->query("ALTER TABLE `zzz_avatars` ADD `av_position` VARCHAR( 128 ) NULL AFTER `av_institute`"); 
    }
  if (!myPear_db()->columnExists('av_phd_adviser_avid','zzz_avatars')){
    myPear_db()->query("ALTER TABLE `zzz_avatars` ADD `av_phd_adviser_avid` INT NOT NULL AFTER `av_phd_country`"); 
  }
  if (!myPear_db()->columnExists('org_theme','zzz_organizations')){
    myPear_db()->query("ALTER TABLE `zzz_organizations` ADD `org_theme` VARCHAR( 128 ) NULL AFTER `org_domain`"); 
  }
  if (!myPear_db()->columnExists('l_rank','zzz_lists')){
    myPear_db()->query("ALTER TABLE `zzz_lists` ADD `l_rank` INT NULL AFTER `l_parent`"); 
  }
}

/*
 * Important cleanup...
 */
function myPear_update17(){
  if (!cnf_CLI) foreach(array('av_phd_year','av_ms_year','av_birthdate') as $f){
      $value = ($f=='av_birthdate' ? 'NULL' : CONST_year_not_considered);
      myPear_db()->qquery("UPDATE zzz_avatars SET $f =$value WHERE $f REGEXP 'not.*considered'",True);
      myPear_db()->qquery("UPDATE zzz_avatars SET $f = NULL  WHERE $f = 0",True);
      myPear_db()->query("ALTER TABLE `zzz_avatars` CHANGE `$f` `$f` INT NULL DEFAULT NULL"); 

      /*
      $where = "DAY(FROM_UNIXTIME($f)) = 1 AND MONTH(FROM_UNIXTIME($f)) = 7";
      $y     = "YEAR(FROM_UNIXTIME($f))"; 
      $q = myPear_db()->qquery("SELECT $f,$y as y FROM zzz_avatars WHERE $where GROUP BY y ORDER BY y",1); 
      while ($r = myPear_db()->next_record($q)){
	$y_converted = b_time::y2d($r['y']);
	myPear_db()->qquery("UPDATE zzz_avatars SET $f = $y_converted WHERE $where AND $y = $r[y]",1);
      }
      */
    }
}


/*
 * Strange bUnit...
 */
function myPear_update16(){
  if (True){
    myPear_db()->qquery("DELETE FROM zzz_units WHERE u_rank = 'RO'",1);
  }else{
    $q = myPear_db()->qquery("SELECT * FROM zzz_units LEFT JOIN zzz_unit_members ON u_id=um_uid WHERE u_rank = 'RO'",1);
    while($r = myPear_db()->next_record($q)){
      b_debug::print_r($r,"u_rank=RO");
      $qq = myPear_db()->qquery("SELECT * FROM zzz_units WHERE u_id = '$r[u_parent]'",1);
      while($rr = myPear_db()->next_record($qq)){
	b_debug::print_r($rr,'parent');
      }
    }
  }
}

/*
 * Important cleanup
 */
function myPear_update14(){
  if (myPear_db()->tableExists('zzz_flavors')) {
    if (myPear_db()->tableExists('zzz_organizations')) myPear_db()->qquery("DROP TABLE `zzz_organizations`");
    myPear_db()->qquery("RENAME TABLE `zzz_flavors` TO `zzz_organizations`",1);
    myPear_db()->reset_cache();
  }

  foreach(array('zzz_organizations'=>array('org_id'       => ' int(11) NOT NULL AUTO_INCREMENT',
					   'org_name'     => ' varchar(128) NOT NULL DEFAULT \'\'',
					   'org_nickname'      => ' varchar(32) NOT NULL DEFAULT \'\'',
					   'org_signature_dir' => ' tinytext NOT NULL',
					   'org_domain'        => ' varchar(24) NOT NULL DEFAULT \'\'',
					   'org_timestamp'     => ' timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP',),
		'zzz_logs'         =>array('log_org'           => ' VARCHAR( 32 ) NOT NULL'),
		)  as $table => $changes){
    foreach($changes as $n=>$atr){
      $f = str_replace('org_','f_',$n);
      $f = str_replace('_org','_flavor',$f);
      if (myPear_db()->columnExists($f,$table)){
	myPear_db()->qquery("ALTER TABLE `$table` CHANGE `$f` `$n` $atr",1);
	myPear_db()->reset_cache();
      }
    }
  }

  myPear_db()->qquery("UPDATE zzz_logs SET log_org='nordita' WHERE log_org REGEXP 'nordita'",True);
}

/*
 *
 */
function myPear_update15(){
  static $wrong_name = 'unknown|xxx';
  $q = myPear_db()->qquery("SELECT * FROM zzz_avatars WHERE av_email REGEXP '$wrong_name' OR av_email2 REGEXP '$wrong_name'",cnf_dev);
  while($r = myPear_db()->next_record($q)){
    if (preg_match("/$wrong_name/i",$r['av_firstname'])) unset($r['av_firstname']);
    if (preg_match("/$wrong_name/i",$r['av_lastname']))  unset($r['av_lastname']);
    $generated_email = bForm_Avatar::generate_email(@$r['av_firstname'],@$r['av_lastname']);
    $existing_email = join(',',array_keys(bForm_Avatar::tidy_email($r['av_email'].','.@$r['av_email2'])));
    $updates = bForm_Avatar::tidy_email("$existing_email,$generated_email");
    $av = new bForm_Avatar($r['av_id']);
    $av->isVITAL = array();
    $av->updateDB($updates);
  }
}

/*
 * 2012-06-28
 */
function myPear_update13(){
  locateAndInclude('bForm_Organization');
  bForm_Organization::delete(54643);
}

/*
 * 2012-06-28
 */
function myPear_locate_duplicate_bLists(){
  static $dejaVu = array();
  static $table = array('Employment record'=>'bList_eaEmpRecords',
			'Social events'    =>'bList_vm_socialEvents',
			'Scholarship rates'=>'bList_vm_scholarshipRates',
			LIST_budgetSource  =>'bList_vm_budgetSource',
			LIST_projects      =>'bList_vm_projects',
			);

  $qqq = myPear_db()->qquery("SELECT l_class,l_name,l_id FROM zzz_lists ORDER BY l_class,l_name",True);
  while($rrr = myPear_db()->next_record($qqq)){
    
    if (myPear::is_empty($l_class = $rrr['l_class'])){
      if (!myPear::is_empty($l_class = @$table[$rrr['l_name']])){
	myPear::DEBUG("recover l_class=\"$l_class\"");
	myPear_db()->qquery("UPDATE zzz_lists SET l_class='$l_class' WHERE l_id=$rrr[l_id]",True);
      }
    }
    if (empty($l_class))   myPear::WARNING("missing l_class l_id=$rrr[l_id] l_name=\"$rrr[l_name]\"");
    if (empty($l_class) || @$dejaVu[$l_class]++) continue;
      myPear::DEBUG("==================================================================== $l_class");

    $qq = myPear_db()->qquery("SELECT COUNT(*),av_firstname,av_lastname,l_class,l_name,l_parent,l_id FROM zzz_lists ".
			       " LEFT JOIN zzz_avatars ON l_parent = av_id ".
			       " WHERE l_class = '$l_class' ".
			       " GROUP BY l_parent ".
			       " HAVING COUNT(*)>1 ORDER BY COUNT(*) DESC",True);
    while($rr = myPear_db()->next_record($qq)){
      $q = myPear_db()->qquery("SELECT * FROM zzz_lists ".
				 " WHERE l_class = '$l_class' AND l_parent = '$rr[l_parent]' ");
      $id = bForm_Avatar::_fmtName('Lf',$rr);
      if (empty($id)) $id = "$rr[l_name]_$rr[l_parent]";
      myPear::DEBUG("&nbsp; ------------------------------ $id");
      print x('h2',$id);
      while($r = myPear_db()->next_record($q)){
	print x('h3',$r['l_id']);
	if (!class_exists($l_class,0)) continue;
	switch($l_class){
	case 'bList_eaEmpRecords':
	  $bList = new $l_class($r['l_parent'],$r['l_id']);
	  $bList->MODE = 'RO';
	  $bList->show();
	  break;
	case 'bList_vm_projects':
	  $bList = new $l_class($r['l_id']);
	  $bList->MODE = 'RO';
	  $bList->show();
	  break;
	default:
	  b_debug::_array($rr,"$l_class l_id=$r[l_id]",1);
	}
      }
    }
  }
}


/*
 * 2012-05-25
 */
function myPear_update12(){
  if (!myPear_db()->columnExists('av_ddate','zzz_avatars')){
    $q = myPear_db()->qquery("ALTER TABLE `zzz_avatars` ADD `av_ddate` INT( 32 ) NULL AFTER `av_birthdate`",True);
    myPear_db()->reset_cache();
  } 
}


/*
 * Get a normal residential address instead of ambigious one
 */
function myPear_update11(){
  myPear_db()->qquery("UPDATE zzz_avatars SET av_address = '' WHERE (av_address != '' AND av_address IS NOT NULL)",1);
  if (!myPear_db()->columnExists('av_zip','zzz_avatars')){
    myPear_db()->qquery("ALTER TABLE `zzz_avatars` ADD `av_zip` VARCHAR( 132 ) NOT NULL DEFAULT '' AFTER `av_address`"); 
    myPear_db()->reset_cache();
  }
  if (!myPear_db()->columnExists('l_class','zzz_lists')){
    myPear_db()->qquery("ALTER TABLE `zzz_lists` ADD `l_class` VARCHAR( 64 ) NOT NULL AFTER `l_parent`",1);
    myPear_db()->qquery("ALTER TABLE `zzz_units` ADD `u_class` VARCHAR( 64 ) NOT NULL AFTER `u_parent`",1); 
    myPear_db()->reset_cache();
  } 
}

// Fix the birthday & SSN
function myPear_update10(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    if (!myPear_db()->columnExists('av_birthdate','zzz_avatars')){
      $q = myPear_db()->qquery("ALTER TABLE `zzz_avatars` CHANGE `av_ssn` `av_birthdate` VARCHAR( 32 ) NOT NULL",cnf_CLI);
      myPear_db()->reset_cache();
    } 
    myPear_update_createIfMissing('av_ssn', 'zzz_avatars','av_sex');
    
    foreach(array('av_birthdate','av_ms_year','av_phd_year') as $field){
      $n_re = $re = array();
      $re1  = '^[0-9][0-9][0-9][0-9][0-9][0-9]-';
      $re[] = $re1;
      
      $re2 = '^[0-9][0-9][0-9][0-9][0-9][0-9]$';
      $re[] = $re2;
      
      $re3 = '^[0-9][0-9][0-9]?[0-9]?[-/\.][0-9][0-9][-/\.][0-9][0-9][0-9]?[0-9]?$';
      $re[] = $re3;
      
      // Normal personal number
      if ($field == 'av_birthdate'){
	$q = myPear_db()->qquery("SELECT * FROM zzz_avatars WHERE $field REGEXP '$re1'");
	while($r = myPear_db()->next_record($q)){  
	  $ssn = $r[$field];
	  preg_match('/^([0-9][0-9])([0-9][0-9])([0-9][0-9])-(.*)/',$ssn,$m);
	  $day = b_time::txt2unix(sprintf('%02d-%02d-%02d',$m[1],$m[2],$m[3]));
	  if     ((int)$m[1] == 0 && (int)$m[2] ==0) {$ssn = ''; $day = '';}
	  elseif ($m[4]=='0000')                      $ssn = '';
	  elseif ($m[4]=='P1234')                     $ssn = '';
	  elseif (is_numeric($m[4]))                  $ssn = sprintf('%02d%02d%02d-%s',$m[1],$m[2],$m[3],$m[4]);
	  myPear::WARNING_T("$r[av_birthdate] - ".($day==''?'':date('Y-m-d',$day))." $ssn");
	  myPear_db()->qquery("UPDATE zzz_avatars SET av_ssn = '$ssn', av_birthdate = '$day' WHERE av_id=$r[av_id]");
	}
      }
      
      // date YYMMDD
      $q = myPear_db()->qquery("SELECT * FROM zzz_avatars WHERE $field REGEXP '$re2'");
      while($r = myPear_db()->next_record($q)){  
	preg_match('/^([0-9][0-9])([0-9][0-9])([0-9][0-9])$/',$r[$field],$m);
	$day = b_time::txt2unix(sprintf('%02d-%02d-%02d',$m[1],$m[2],$m[3]));
	if ((int)$m[2] > 12)  $day = '';
	if ((int)$m[3] > 31)  $day = '';
	myPear::WARNING_T("$field: ".$r[$field]." -> ".($day==''?'?':date('Y-m-d',$day)));
	myPear_db()->qquery("UPDATE zzz_avatars SET $field = '$day' WHERE av_id=$r[av_id]");
      }
      
      // date YY/MM/DD
      $q = myPear_db()->qquery("SELECT * FROM zzz_avatars WHERE $field REGEXP '$re3'");
      while($r = myPear_db()->next_record($q)){  
	preg_match('/^([0-9]?[0-9]?[0-9][0-9]).([0-9][0-9]).([0-9][0-9][0-9]?[0-9]?)$/',$r[$field],$m);
	if ((int)$m[3] > 31)  {$x=$m[3]; $m[3]=$m[1]; $m[1] = $x; }
	$day = b_time::txt2unix(sprintf('%02d-%02d-%02d',$m[1],$m[2],$m[3]));
	myPear::WARNING_T("$field: ".$r[$field]." - ".($day==''?'?':date('Y-m-d',$day)));
	myPear_db()->qquery("UPDATE zzz_avatars SET $field = '$day' WHERE av_id=$r[av_id]");
      }
      
      // Known problems
      foreach(array('July 15, 1982'=>'82-07-15',
		    '18 February 1945'=>'45-02-18',
		    '05 */ 15 / 1929'=>'29-05-15',
		    '15 /03 /1973' => '73-03-15',
		    '^[a-zA-Z\(\)\-\/ ]*$'  => '',
		    ) as $f=>$t){
	$re[] = $f;
	$q = myPear_db()->qquery("SELECT * FROM zzz_avatars WHERE ($field != '' ) AND $field REGEXP '$f'");
	while($r = myPear_db()->next_record($q)){  
	  $day = (empty($t) ? '' : b_time::txt2unix($t));
	  myPear::WARNING_T("$field: ".$r[$field]." - ".($day==''?'?':date('Y-m-d',$day)));
	  myPear_db()->qquery("UPDATE zzz_avatars SET $field = '$day' WHERE av_id=$r[av_id]");
	}
      }

      // What else ???
      $n_re[] = "($field != '' )";
      foreach($re as $f)       $n_re[] = " ( $field NOT REGEXP '$f' ) ";
      $q = myPear_db()->qquery("SELECT av_id,$field FROM zzz_avatars WHERE ".join(' AND ',$n_re)." GROUP BY $field");
      while($r = myPear_db()->next_record($q)){  
	if (!(is_numeric($r[$field]))){
	  if (preg_match('/\b(\d\d\d\d)\b/', $r[$field],$m) && $m[1]>1900 && $m[1]<=CONST_eternity_year){
	    $new_value = $m[1];
	    myPear::WARNING_T("$field = '".$r[$field]."' -> $new_value");
	    myPear_db()->qquery("UPDATE zzz_avatars SET $field = '$new_value' WHERE av_id=$r[av_id]");
	  }else
	    myPear::WARNING_T("$field ? '".$r[$field]."'");
	}else{
	  if (!(date('Y',$r[$field]) > 1900 && date('Y',$r[$field]) <= CONST_eternity_year)) myPear::WARNING_T("$field ? '".$r[$field]."' ".date('Y-m-d',$r[$field]));
	}
      }
    }
  }
}

function myPear_update09(){
  $key = 'lm_id';
  myPear_db()->query("ALTER TABLE zzz_list_members DROP INDEX `$key`",False);
  myPear_db()->query("ALTER TABLE zzz_list_members ADD UNIQUE ( `lm_id` , `lm_lid` )",False);
  myPear_update_createIfMissing('av_ms_country',  'zzz_avatars','av_ms_year');
  myPear_update_createIfMissing('av_phd_country', 'zzz_avatars','av_phd_year');
}

function myPear_update_createIfMissing($column,$table,$after){
  if (!myPear_db()->columnExists($column,$table)){
    myPear_db()->qquery("ALTER TABLE `$table` ADD `$column` VARCHAR( 120 ) NULL AFTER `$after`",True);
    myPear_db()->reset_cache();
  }
  myPear_update_addIndex($table,$column);
}

// Supress education fields in the EA record, use the standard avatar values
function myPear_update08(){
  if (!@$_SESSION['updates'][__METHOD__]++){
  }
}


function myPear_update07(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    myPear_db()->qquery("ALTER TABLE `zzz_avatars` CHANGE `av_ms_year`  `av_ms_year`  VARCHAR( 32 ) NULL DEFAULT NULL",cnf_CLI);
    myPear_db()->qquery("ALTER TABLE `zzz_avatars` CHANGE `av_phd_year` `av_phd_year` VARCHAR( 32 ) NULL DEFAULT NULL",cnf_CLI);
  }
}


function myPear_update06(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    myPear_db()->debug = cnf_CLI;
    myPear_db()->query("ALTER TABLE  zzz_flavors DROP INDEX   f_nickname",$stopIfFails=False); 
    myPear_db()->query("ALTER TABLE `zzz_flavors` ADD UNIQUE (`f_nickname`)",$stopIfFails=False); 
    myPear_db()->debug = 0;

    myPear_db()->query("ALTER TABLE  zzz_avatars DROP INDEX   av_email",$stopIfFails=False); 
    myPear_db()->qquery("ALTER TABLE `zzz_avatars` CHANGE `av_id2`    `av_id2`    TEXT NULL DEFAULT NULL",cnf_CLI); 
    myPear_db()->qquery("ALTER TABLE `zzz_avatars` CHANGE `av_email2` `av_email2` TEXT NULL DEFAULT NULL",cnf_CLI); 
    myPear_db()->query("ALTER TABLE `zzz_avatars` ADD UNIQUE `av_email` (`av_email` , `av_email2`(255))",False);
    myPear_db()->reset_cache();
  }
}

/*
 * 2011-06-11
 */
function myPear_update05(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    if (myPear_db()->columnExists('f_headerclass','zzz_flavors')){
      myPear_db()->qquery('ALTER TABLE `zzz_flavors` DROP `f_headerclass`',cnf_CLI);
      myPear_db()->reset_cache();
    }
    if (!myPear_db()->columnExists('um_option',  'zzz_unit_members')){
      myPear_db()->qquery('ALTER TABLE `zzz_unit_members` ADD `um_option` MEDIUMTEXT NOT NULL AFTER `um_uid` ',cnf_CLI); 
      myPear_db()->reset_cache();
    }
  }
}

/*
 * 2011-06-01
 */
function myPear_update04(){
  if (!@$_SESSION['updateszzz'][__METHOD__]++){
    myPear_db()->query("UPDATE zzz_logs SET log_type = 'alumnus' WHERE log_type = 'alumnus_eh'");
    myPear_db()->reset_cache();
    if (!myPear_db()->columnExists('av_birthplace',  'zzz_avatars')){
      myPear_db()->query('ALTER TABLE `zzz_avatars` ADD `av_birthplace` VARCHAR( 240 ) NULL DEFAULT NULL AFTER `av_birthdate`, ADD INDEX ( av_birthplace )'); 
      myPear_db()->query('ALTER TABLE `zzz_avatars` ADD `av_birthcountry` VARCHAR( 8 ) NULL DEFAULT NULL AFTER `av_birthdate`, ADD INDEX ( av_birthcountry )'); 
      myPear_db()->reset_cache();
    }
  }
}

function myPear_update03(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    myPear_db()->query("ALTER TABLE `zzz_list_members` CHANGE `lm_option` `lm_option` MEDIUMTEXT");
  }
}

function myPear_update02(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    if (!myPear_db()->columnExists('lm_option',  'zzz_list_members')){
      myPear_db()->query('ALTER TABLE `zzz_list_members` ADD `lm_option` VARCHAR(32) NOT NULL AFTER `lm_status`'); 
      myPear_db()->reset_cache();
    }
  }
}

function myPear_update01(){
  if (!@$_SESSION['updates'][__METHOD__]++){
    if (!myPear_db()->columnExists('tpl_owner',  'zzz_templates')){
      myPear_db()->query('ALTER TABLE `zzz_templates` ADD `tpl_owner` VARCHAR(132) NOT NULL AFTER `tpl_id`');
      myPear_db()->reset_cache();
    }
  }
}


/*
 * Migrate database-4 to new conventions in database-5
 */
class b_renumber{

  public  static $debug  = 0;
  private static $counter = 0;

  /*
   * renumber ALL the forms with TABLEID in TABLE
   */
  public static function all($TABLE, $TABLEID, $trigger=Null,$tablesToConsider=array()){
    if (!myPear_db()->tableExists($TABLE))      return;

    bTiming()->cpu("renumber_$TABLE");
    self::$counter = 0;
    print '<h1>'.b_debug::__().'</h1>';

    // build the list of <x>_<y>id fields
    $db = myPear_db();
    if (is_null($trigger)) {
      $doit = True;
    }else{
      $q = $db->query("SELECT * FROM $TABLE WHERE $TABLEID = '$trigger'");
      $doit = ($db->num_rows($q) > 0);
    }
    if ($doit){
      $iddt = Null;
      self::getTables($TABLE,$TABLEID,$tablesToConsider);
      
      $q = $db->query("SELECT * FROM $TABLE");
      $db->debug = self::$debug;
      while ($r = $db->next_record($q)){
	$old_id = $r[$TABLEID];
	$db->debug = 0; 
	$new_id = bMaster()->reserve_masterCounter();
	$db->debug = self::$debug;
	self::id($r,$TABLE,$TABLEID,$old_id,$new_id,$iddt);
      }
      $db->debug = 0;
    }
    bTiming()->cpu();
  }
  
  /*
   * Build list of tables where the <xxx>_id is used
   */
  static $affectedTables = array();
  public static function getTables($TABLE,$TABLEID,$tablesToConsider){
    $idid = '_' . str_replace('_','',$TABLEID);
    self::$affectedTables[$TABLEID.$TABLE] = array();
    myPear_db()->reset_cache();
    if (empty($tablesToConsider))  $tablesToConsider = myPear_db()->getTables();
    foreach  ($tablesToConsider as $tabl){
      foreach (myPear_db()->getColumns($tabl) as $c){ 
	if (preg_match("/($idid|log_form|_parent)$/",$c)) // columns ending by, say,  "_avid"
	  self::$affectedTables[$TABLEID.$TABLE][$tabl][] = $c;
      }
    }
  }
  
  public static function id($r,$TABLE,$TABLEID,$old_id,$new_id,$iddt=Null){	
    $db = myPear_db();
    $set = array("$TABLEID=$new_id");
    if ($iddt&&($d=$r[$iddt]))$set[] = "$iddt='$d'";
    if ($TABLEID == 'av_id')  $set[] = self::merge('av_id2',        $old_id,$r['av_id2'],' ');
    if ($TABLEID == 'ap_id')  $set[] = self::merge('ap_id_spurious',$old_id,$r['ap_id_spurious'],',');

    $q = $db->query($m="UPDATE $TABLE SET ".join(',',$set)." WHERE $TABLEID=$old_id");
    if ($n = $db->num_rows($q)){
      print "<br/><b>".(++self::$counter)." - ".($n>1?" ($n rows) " : "")."$m</b>  <br/>\n";
    }
    foreach (self::$affectedTables[$TABLEID.$TABLE] as $tabl=>$cc){
      foreach($cc as $c)  $db->qquery($m="UPDATE $tabl SET $c='$new_id' WHERE $c='$old_id'",1);
    }      
  }
  
  public static function merge($name,$value,$values,$sep=','){
    if (is_string($value))  $value = array($value);
    if (is_string($values)) $values= array($values);
    $result = $value;
    foreach($values as $vv){
      foreach(explode($sep,$vv) as $v) if (trim($v)) $result[] = trim($v);
    }
    return myPear_db()->quote(join($sep,array_unique($result)),1,$name);
  }
}


