<?php
/*
 * EA updates
 */


myPear_startup_queue('ea_updates');


function ea_updates(){
  if    (file_exists($f=dirname(__FILE__)."/../config.inc")) require_once($f);
  elseif(file_exists($f=dirname(__FILE__)."/../config.inc")) require_once($f);
  if (bMaster()->register_module_update_script(EA_MODULE,EA_VERSION,__FILE__)){
    // Updates take time...
    ini_set("memory_limit", "-1");
    set_time_limit(0);

    bTiming()->cpu(__FUNCTION__);
    myPear_db()->noAutoCommit('start');
    // ea_update01();
    // ea_update02();
    // ea_update03();
    // ea_update04();
    // ea_update05();
    // ea_update06();
    //
    // 2012-07-27
    //
    // ea_update07();
    // ea_update08();
    // ea_update09();
    // ea_update10();

    //
    // 2012-08-30
    if (False){
      ea_update11();
      ea_update12();
      ea_update13();
    }

    // 2015-01-15
    if (True){
      locateAndInclude('bList');
      bList::$use_membersCache = False;
      ea_update16();
      ea_update17();
      ea_update18();
      ea_update19();
      ea_update15_off();
    }
    myPear_db()->noAutoCommit('end');
    bTiming()->cpu();
  }
}

// ea_update19();

/*
 * Make the board membership a standard employement
 */
function ea_update19(){
  locateAndInclude('bForm_Avatar_ea');

  // Remove role2 field
  $q = myPear_db()->qquery("SELECT * FROM zzz_unit_members WHERE um_option REGEXP 'role2'",True);
  while ($r = myPear_db()->next_record($q)){
    $um_option = unserialize($r['um_option']);
    unset($um_option['role2']);
    sort($um_option);
    myPear_db()->qquery("UPDATE zzz_unit_members SET ".myPear_db()->quote(serialize($um_option), 1,'um_option')." WHERE um_id=$r[um_id]",True);
  }
  
  // Replace record type
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members LEFT JOIN zzz_lists ON l_id = lm_lid ".
			   " WHERE lm_option REGEXP 'e_rate.*Board' " // . " OR lm_value REGEXP 'Board'"
			   ,True);
  while ($r = myPear_db()->next_record($q)){
    $lm_option = unserialize($r['lm_option']);
    $lm_option['rec_type'] = '20_mp';
    if (@$r['l_parent']){
      $av = new bForm_Avatar_ea($r['l_parent']);
      if ($e = $av->hook_employment()) $e->updateMember(array('rec_type' => '20_mp',
							      'lm_option'=> serialize($lm_option)),
							$r['lm_id']);
    }
  }
  // rebuld...
  myOrg()->eaMembers()->rebuild();
}

/*
 * Set up new Nordita emp. titles
 */
function ea_update17(){
  $structure = 
    array(
	  'Directors'               =>array('Nordita Director',
					    'Deputy Director'),
	  'Faculty'                 =>array('Nordita Professor',
					    'Nordita Affiliate Professor',
					    'Nordita Professor Emeritus'),
	  'Junior Faculty'          =>array('Nordita Assistant Professor',
					    'Nordic Associate Professor',
					    'Nordic Assistant Professor'),
	  'Postdocs and Researchers'=>array('Nordita Fellow',
					    'Nordita Postdoc',
					    'Nordic Fellow'),
	  'Corresponding Fellows'=>   array('Corresponding Fellow'),
	  'PhD Students' =>           array('PhD Student'),
	  'Masters Students' =>       array('Masters Student',
					    'Visiting Masters Student'),
	  'Visitors'=>   	      array('Guest Researcher',
					    'Visiting PhD Student'),
	  'Administration' =>         array('Head of Administration',
					    'Head of Computing',
					    'Web and Computing',
					    'HR Officer',
					    'Scientific Program Coordinator',
					    'Administrative Assistant',
					    'Building Caretaker'),
	  'Board' => 		      array('Board Chairman',
					    'Board Member'),
	  );
  
  // Delete old emp. types

 
  // Create new emp. types / titles
  foreach($structure as $empType=>$empTitles){
    $lm_option = array();
    foreach($empTitles as $title) $lm_option[myOrg('nordita')->orgEmpTypes()->create_textKey($title,$empType)] = $title;
    
    myOrg('nordita')->orgEmpTypes()->updateMember(array('lm_key'    =>myOrg('nordita')->orgEmpTypes()->create_textKey($empType),
							'lm_value'  =>$empType,
							'lm_option' =>serialize($lm_option)));
  }
  
}

function ea_update18(){
  b_debug::xxx(starting);
  
  $org_id = 40090;
  $org_name = myOrg($org_id)->name();
  
  $drops = array(
		 'director emeritus',
		 'coordinator',
		 );
  $as_is = array('nordic associate professor',
		 'nordic assistant professor',
		 'nordic fellow',
		 'carlsberg foundation fellow',
		 'board chairman',
		 'web and computing',
		 'head of administration',
		 'head of computing',
		 'deputy director',
		 'building caretaker',
		 'scientific program coordinator',
		 'hr officer',
		 'visiting masters student',
		 'tage erlander professor',
		 'scientist',	
		 'it-gruppen',
		 'marie curie fellow',	
		 'marie curie fellowship',
		 'angelo della riccia foundation fellow',
		 'distinguished visitor',
 );
  $replaces = array('visitor'=>'guest researcher',
		    'professor,director'=>'nordita professor',
		    );
  $regexp_replaces = array('/.*visiting.*(phd).*/' =>'visiting phd student',
			   '/.*(associate.*fellow|guest|visitor|visiting).*/'=> 'guest researcher',
			   '/.*corresponding fellow.*/'=>'corresponding fellow',
			   '/.*(affiliate|associate).*phd.*/'=> 'guest researcher',
			   '/.*(nordita)?.*phd student.*univer.*/'    => 'phd student',
			   '/.*(administrat|(student|administrative).*assistant).*/' => 'administrative assistant',
			   '/.*board member.*/'                        => 'board member',
			   '/.*(adjunct|assistant) professor.*/' => 'nordita assistant professor',
			   '/.*post.*doc.*/'    => 'nordita postdoc',
			   '/.*nordic fellow.*/' => 'nordic fellow',
			   '/.*associate professor.*/' =>  'nordic associate professor',
			   );
  
  $prefix_to_add = array('director',
			 'professor',
			 'affiliate professor',
			 'assistant professor',
			 'professor emeritus',
			 'postdoc',
			 'fellow',
			 // 'phd student',
			  );		 
  foreach($prefix_to_add as $n) $with_prefix[] = strToLower("$org_name $n");
  
  b_cnf::set('noCompactMessages',1);
  locateAndInclude('bList_eaEmpRecords');
  if (!function_exists('nameIt')){
    function nameIt(&$r,&$name_printed){
      if (!$name_printed) { 
	$name_printed=1; 
	myPear::DEBUG('======================================================================'.$r['av_firstname'].' '.$r['av_lastname']); 
      }
    }
  }
  if (!function_exists('keepIt')){
    function keepIt($title){
      static $n_titles = array();
      if ($title === 'print'){
	ksort($n_titles);
	$t = new b_table_zebra(array('c'=>'Count','t'=>'Title'));
	foreach($n_titles as $title=>$count) $t->prt(array('t'=>$title,'c'=>$count));
	$t->close();
      }else{
	@$n_titles[$title]++; 
      }
    }
  }
  if (!function_exists('replaceIt')){
    function replaceIt($rr,$r,&$name_printed,$from,$to){
      if (strToLower($from) != strToLower($to)){
	keepIt($to);
	nameIt($rr,$name_printed);
	$to = b_fmt::ucwords($to);
	myPear::DEBUG('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'.b_time::period($r['lm_key'],$r['lm_status'])." $from =====> $to");
	//myPear_db()->qquery("UPDATE zzz_list_members SET lm_value = '$to' WHERE lm_id = ".$r['lm_id'],True);
      }
      return $to;
    }
  }

  // setup the new emp.types
  $updates = array();
  foreach(myOrg($org_id)->eaMembers()->getMembers() as $lm_id=>$rr){
    $av_id = $rr['av_id'];
    $name_printed = False;
    $bList_eaEmpRecords = new bList_eaEmpRecords($av_id);
    //    foreach($bList_eaEmpRecords->getMembers(array('e_org'=>$org_id)) as $lm_id=>$r){
    foreach($bList_eaEmpRecords->getMembers() as $lm_id=>$r){
      $to = $title = strToLower(trim(str_replace('  ',' ',$r['lm_value'])));
      if (empty($title) || in_array($title,$drops)){
 	b_debug::print_r($r,'??? drop me');
	myPear_db()->qquery("DELETE FROM  zzz_list_members WHERE lm_id = ".$r['lm_id'],True);
	continue;
      }
      
      if (empty($r['e_org']) || ($r['e_org'] != $org_id)){
	nameIt($rr,$name_printed);
 	b_debug::xxx("... not myOrg, keep the title '$title'");
      }else{
	// Nordita...
	foreach($regexp_replaces as $f=>$t){
	  $title_fixed = preg_replace($f.'i',$t,$title);
	  if ($title_fixed === Null) b_debug::internalError("preg_replace($f, $t, $title)");
	  if ($title_fixed != $title) break;
	}
	
	if ($title_fixed != $title) {
	  $to = replaceIt($rr,$r,$name_printed,$title,$title_fixed);
	}elseif (in_array($title,$prefix_to_add)){ // -----------------Catch the missing "nordita" prefix
	  $to = replaceIt($rr,$r,$name_printed,$title,"Nordita $title");
	}elseif(in_array($title,$as_is) || 
		in_array($title,array_values($replaces)) ||
		in_array($title,array_values($regexp_replaces))){ // --------------Skip the unchanged titles
	  keepIt($title);
	}elseif(in_array($title,array_keys($replaces))){ // -------------------Replace the outdated title
	  $to = replaceIt($rr,$r,$name_printed,$title,$replaces[$title]);
	}elseif (!in_array($title,$with_prefix)){        // -------------------Catch an unknown title
	  nameIt($rr,$name_printed);
	  myPear::DEBUG('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'.b_time::period($r['lm_key'],$r['lm_status'])." ??? $title");
	  keepIt("??? $title");
	}
      }
	
      // Update the database records
      nameIt($rr,$name_printed);
      //    First the title
      $args = array();
      if ($title != $to) $args['lm_value'] = $to;
      
      //    Second - option
      $lm_option_new = $lm_option = unserialize($r['lm_option']);
      
      if (@$r['e_org'] == $org_id){
	if (($old_rate=@$r['e_rate']) != ($new_rate=myOrg($org_id)->orgEmpTypes()->title2empType($to,$rr['av_id'].' '.$rr['av_firstname'].' '.$rr['av_lastname']))){
	  b_debug::xxx("'$old_rate' =====> '$new_rate' ($to)");
	  $lm_option_new['e_rate'] = $args['e_rate'] = $new_rate;
	}
      }

	// Clean the records
      if (!empty($r['e_org'])) foreach(array('e_inst','e_cc') as $f) if (isset($lm_option_new[$f])) unset($lm_option_new[$f]);
      
      if (!bList_eaEmpRecords::is_current($r,Null,$verbose=True)){ 
	foreach(array('e_gid', 'e_off','e_rate') as $f) unset($lm_option_new[$f]);
      }
      if ($lm_option_new !== $lm_option){
	ksort($lm_option_new);
	b_debug::xxx(sprintf(' [%s] ====> [%s]',b_fmt::joinX(',',$lm_option),b_fmt::joinX(',',$lm_option_new)));
	myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($lm_option_new), 1,'lm_option')." WHERE lm_id=$r[lm_id]",True);
      }
      
      $updates[] = (empty($args) 
		    ? array('av_id'=>$av_id)
		    : array('av_id'=>$av_id,
			    'lm_id'=>$r['lm_id'],
			    'args' =>$args,
			    'who'  =>$rr['av_firstname'].' '.$rr['av_lastname'],
			    ));
    }
  }

  b_debug::xxx("-============================== updating bList_eaEmpRecords");
  b_debug::xxx("--============================= updating bList_eaEmpRecords");
  b_debug::xxx("---============================ updating bList_eaEmpRecords");
  foreach($updates as $u){
    if (!empty($u['args'])){
      $bList_eaEmpRecords = new bList_eaEmpRecords($u['av_id']);
      $bList_eaEmpRecords->updateMember($u['args'],$u['lm_id']);
    }
    new bForm_Avatar_ea($u['av_id']);
  }
  //  keepIt('print');
  b_cnf::set('noCompactMessages',0);
  b_debug::xxx(exiting);
}

/*
 * Move "emp.type" from lm_option -> lm_value, reserve lm_option for the emp. titles array
 */
function ea_update16(){
  
  // Delete first the old Nordita emp. types
  $recs = myOrg('nordita')->orgEmpTypes()->getMembers();
  foreach(array_keys($recs) as $lm_id) myOrg('nordita')->orgEmpTypes()->deleteMember($lm_id);

  // Change the format for the rest
  $qq = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_class = 'bList_orgEmpTypes'",True);
  while ($rr = myPear_db()->next_record($qq)){
    $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_value IS NULL AND lm_lid = ".$rr['l_id'],True);
    while ($r = myPear_db()->next_record($q)){
      $lm_option = unserialize($r['lm_option']);
      $rec_type = $lm_option['rec_type'];
      if (empty($rec_type)){
	b_debug::print_r($r,"??? both lm_value & lm_option are empty");
	myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_id=$r[lm_id]",True);
      }else{
	sort($lm_option);
	myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($lm_option), 1,'lm_option').", lm_value='$rec_type' WHERE lm_id=$r[lm_id]",True);
      }
    }
  }
}

/*
 * Re-erect offices allocation...
 */
function ea_update15_off(){
  if (!cnf_CLI){

    vm_init();

    foreach(array('450110-3129',
		  '421029-2491',
		  '420222-2341',
		  '431124-2184',
		  '401214-2177',
		  ) as $wrong_ssn){
      $n = explode('-',$wrong_ssn,2);
      myPear_db()->qquery("UPDATE zzz_avatars SET av_ssn = '".$n[0]."-P123' WHERE av_ssn = '$wrong_ssn'",True);
    }

    // Offices for visitors as of 1-Apr-2016
    $off_visitors = array(
			  '122:004' => array('o_capacity'=>5, 'o_status'=>'visitors'), 
			  '122:017' => array('o_capacity'=>2, 'o_status'=>'visitors'), 
			  '122:027' => array('o_capacity'=>4, 'o_status'=>'visitors'), 
			  '122:028' => array('o_capacity'=>4, 'o_status'=>'visitors'), 
			  '122:029' => array('o_capacity'=>5, 'o_status'=>'visitors'), 
			  '122:030' => array('o_capacity'=>2, 'o_status'=>'visitors'), 
			  );
    $off_bogus = array('023:010',
		       '023:012',
		       '133:027');
  

    // myPear_db()->qquery("RENAME TABLE abs_offices TO ea_offices",cnf_dev);
    myPear_db()->qquery("ALTER TABLE `abs_offices` CHANGE `o_capacity` `o_capacity` INT(11) NOT NULL DEFAULT '1'",cnf_dev);
    myPear_db()->qquery("ALTER TABLE `abs_offices` DROP INDEX o_name, ADD UNIQUE o_name (o_name)",cnf_dev);
    
    myPear_db()->qquery("DELETE FROM abs_offices WHERE o_capacity > 999",cnf_dev);
    myPear_db()->qquery("UPDATE abs_offices SET o_capacity=1 WHERE o_capacity = 0 OR o_capacity IS NULL",cnf_dev);
    myPear_db()->qquery("ALTER TABLE `abs_offices` CHANGE `o_capacity` `o_capacity` INT(11) NOT NULL DEFAULT '1'",cnf_dev);

    myPear_db()->qquery("UPDATE abs_offices SET o_status = 'staff'",cnf_dev);
    foreach($off_visitors as $o_name=>$data){
      $o = myPear::getInstance_new('bForm_vm_Office',array('o_name'=>$o_name),'nocache',array('abs_offices','o_id'));
      $o->isVITAL = array();
      $o->updateDB($data);
    }

    foreach($off_bogus as $o_name) myPear_db()->qquery("DELETE FROM abs_offices WHERE o_name='$o_name'",cnf_dev);

    myPear_db()->qquery("DELETE FROM abs_offices WHERE o_capacity > 999",cnf_dev);
    myPear_db()->qquery("UPDATE abs_offices SET o_capacity=1 WHERE o_capacity = 0 OR o_capacity IS NULL",cnf_dev);
    
    // "newcomer" looks better
    myPear_db()->qquery("UPDATE zzz_logs SET log_type='newcomer' WHERE log_type='ea' OR log_type='add_member'",True);

    // Clean foreign organizations which structure is not known 
    locateAndInclude('bUnit');
    locateAndInclude('bForm_Organization');
    $av = bForm_Avatar::hook(array('av_identity'=>'hvzm'));
    foreach(array('umich','yale','kth','gu') as $org){
      if (!bForm_Organization::org_code_2_org_id($org,False)) continue;
      if (is_object($av)){
	$u = new bUnit(myOrg($org)->ID,RANK__ea_org_manager);
	$u->add_unitMember($av->ID);
      }
      foreach(myOrg($org)->orgGroups()->getMembers() as $um_id=>$r){
	myOrg($org)->orgGroups()->deleteMember($um_id);
      }
    }
  }
}


// Minor fixes
function ea_update12(){
  $q = myPear_db()->qquery("SELECT lm_id,lm_option FROM zzz_list_members WHERE lm_option REGEXP 'e_gid.*b_checkBox'",1); 
  while ($r = myPear_db()->next_record($q)){
    $lm_option = unserialize($r['lm_option']);
    unset($lm_option['e_gid']);
    myPear_db()->qquery("UPDATE zzz_list_members SET lm_option='".serialize($lm_option)."' WHERE lm_id=$r[lm_id]",1);
  }
}

/*
 *
 */
function ea_update13(){
  if (!cnf_CLI){
    foreach(array('acc2ea.php','ssn2ea.php','cv2ea.php') as $script){
      $f = dirname(__FILE__)."/../$script";
      if (file_exists($f)){
	myPear::WARNING_T("Executing $f");
	$cmd = "export PATH=/usr/local/php5/bin:/usr/local/bin/:/usr/pkg/php/pro/bin:\$PATH; cd ".dirname($f)."; php $f 2>&1";
	myPear::WARNING_T(x('pre',`$cmd`));
      }
    }
  }
}


/*
 * Import the Nordita structure to the database
 */
function ea_update11(){

  if (myOrg_nameS != 'Nordita') return;
  locateAndInclude('bForm_Avatar');
  if ($av=bForm_Avatar::hook(array('av_identity'=>'hvzm'))){
    $u = new bUnit(myOrg('nordita')->ID,RANK__ea_org_manager);
    $u->add_unitMember($av->ID);
  }

  $groups = array(EA_group_adm => 'Administration',
		  'AP'         => 'Astrophysics',
		  'CM'         => 'Condensed Matter',
		  'SA'         => 'High Energy Physics',
		  //'IT'=> 'IT support',
		  //'MA'=> 'Mathematical Physics',
		  );
  $emp_rates=array('faculty'   => 'Faculty',
		   'fellows'   => 'Fellows/Postdocs',
		   'postgrads' => 'PhD students',
		   'visitors'  => 'Visitors',
		   EA_group_adm=> 'Administration',
		   );
  
  foreach($emp_rates as $key=>$value){
    myOrg('nordita')->orgEmpTypes()->updateMember(array('lm_key'  =>$key,
							'lm_value'=>$value));
  }
  
  foreach($groups as $key=>$value){
    myOrg('nordita')->orgGroups()->updateMember(array('lm_key'=>$key,
						      'g_name'=>$value));
  }
}

/*
 * Pack um_option
 */
function ea_update10(){
  $q = myPear_db()->qquery("SELECT * FROM zzz_unit_members WHERE um_uid=40092",1);
  while($r = myPear_db()->next_record($q)){
    if (empty($r['um_option'])) continue;
    if (is_array($um_option = unserialize($r['um_option']))) continue;
    $um_option = serialize(array('role2'=>$r['um_option']));
    myPear_db()->qquery("UPDATE zzz_unit_members SET um_option = '$um_option' WHERE um_id=$r[um_id]",1);
  }
}

/*
 * Delete the spurious records...          2012-06-28
 */ 
function ea_update09(){
  foreach(array(5704,
		40776,
		41040,
		43057,
		43270,
		) as $l_id) bList::deleteList($l_id);
}


/*
 * 2012-05-01
 * Reshuffle the EA unit, introduce status 'Visitors'
 */
function ea_update08(){

  $debug = cnf_CLI;
  
  // Remove "current position" as an object, make it virtual 
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP '10_cp'",$debug);
  while($r = myPear_db()->next_record($q)){
    $option = unserialize($r['lm_option']);
    if (empty($r['lm_key'])) print_r($r);
    if ( empty($option['e_org'])) unset($option['e_org']);
    if (!empty($option['e_org'])) unset($option['e_cc']);
    if (!empty($option['e_org'])) unset($option['e_inst']);
    if (!empty($option['e_cc'])) $option['e_cc'] = b_cc::getCC($option['e_cc']);

    $option['rec_type'] = '20_mp';
    $lm_status = CONST_eternity;
    myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($option), 1,'lm_option').", lm_status='$lm_status' WHERE lm_id = $r[lm_id]",$debug);
  }

  // Put board membership to the "other activity"
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP 'Nordita|".myOrg('nordita')->ID."' AND lm_value REGEXP 'Board|deputy director'",$debug);
  while($r = myPear_db()->next_record($q)){
    $option = array('e_org' => myOrg('nordita')->ID,
		    'rec_type'=> '22_op');
    myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($option), 1,'lm_option')." WHERE lm_id = $r[lm_id]",$debug);
  }

  // Put visits into the new object "visiting appointment"
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP 'e_org' && lm_value REGEXP 'visiting|visitor|Tage Erlander' ",$debug);
  while($r = myPear_db()->next_record($q)){
    $option = unserialize($r['lm_option']);
    if ( empty($option['e_org'])) unset($option['e_org']);
    if (!empty($option['e_org'])) unset($option['e_cc']);
    if (!empty($option['e_org'])) unset($option['e_inst']);
    if (!empty($option['e_cc'])) $option['e_cc'] = b_cc::getCC($option['e_cc']);
    $option['rec_type'] = '40_va';
    $lm_value = str_ireplace('phd','PhD',b_fmt::ucWords($r['lm_value']));
    myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($option), 1,'lm_option').
			  ", lm_value='$lm_value' WHERE lm_id = $r[lm_id]",$debug);
  }
}

/*
 * Minor fix                           2011-04-25
 * Replace org_code by org_id in the bList_eaEmpRecords  
 */
function ea_update07(){
  signal_update(__METHOD__);
  foreach(array('albanova','nordita','fysikum','vh','an') as $org_code){
    $q = myPear_db()->qquery('SELECT * FROM zzz_list_members WHERE lm_option REGEXP "[[:<:]]e_org[[:>:]].*[[:<:]]'.$org_code.'[[:>:]]"',1);
    while ($r = myPear_db()->next_record($q)){
      $e = unserialize($r['lm_option']);
      $e['e_org'] = myOrg($e['e_org'])->ID;
      unset($r['lm_option']);
      foreach ($e as $k=>$v) $r[$k] = $v;
      ksort($r);
      ksort($e);
      myPear_db()->qquery("UPDATE zzz_list_members SET lm_option='".serialize($e)."' WHERE lm_id='$r[lm_id]'",1);
    }
  }
}

/*
 * Major fix
 */
function ea_update06(){
  signal_update(__METHOD__);

  if (!defined('TEXT_empl_record')) define('TEXT_empl_record'  , 'Employment record');

  // remove the obsolete EA records and use the bForm_Avatar field instead
  $q = myPear_db()->qquery("SELECT * FROM zzz_lists ".
			     " LEFT JOIN zzz_list_members ON l_id = lm_lid ".
			     " LEFT JOIN zzz_avatars ON av_id = l_parent ".
			     " WHERE l_name = '".TEXT_empl_record."' AND lm_option REGEXP 'rec_type.*(34_phd|38_ms)'",cnf_CLI);
  while($r = myPear_db()->next_record($q)){
    $av   = $w = array();
    $edu  = unserialize($r['lm_option']);
    $code = ($edu['rec_type']=='34_phd' ? 'phd' : 'ms');
    $av["av_${code}_year"]  = $r['lm_key'];
    if ($i = @$edu['e_cc'])   $av["av_${code}_country"]   = $i;
    if ($i = @$edu['e_inst']) $av["av_${code}_institute"] = $i;
    if ($av){
      foreach($av as $k=>$v)  $w[] = myPear_db()->quote($v,1,$k);
      myPear_db()->qquery("UPDATE zzz_avatars SET ".join(',',$w)." WHERE av_id = $r[av_id]",cnf_CLI);
      myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_id = $r[lm_id]",cnf_CLI);
    }
  }
  
  // Make '0_org' a virtual type record, 0_org should be (and is) used only for the dialog as a shortcut for "e_org + 20_mp"
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP 'rec_type.*0_org'",cnf_CLI);
  while($r = myPear_db()->next_record($q)){
    $option = unserialize($r['lm_option']);
    $option['rec_type'] = '20_mp';
    myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($option), 1,'lm_option')." WHERE lm_id = $r[lm_id]",cnf_CLI);
  }
}

/*
 * Minor fix
 */
function ea_update05(){
  if (!@$_SESSION[__METHOD__]++){
    signal_update(__METHOD__);
    myPear_db()->qquery("UPDATE zzz_logs SET log_type = 'ea' WHERE log_type = 'alumnus'",cnf_CLI);
    foreach(array('ADB'=>EA_MODULE,'adb'=>EA_MODULE) as $f=>$t){
      myPear_db()->qquery("UPDATE zzz_logs SET log_api = '$t' WHERE log_api = '$f'",cnf_CLI);
    }

    /*
    $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP  '[[:<:]]e_org[[:>:]]' ",cnf_CLI);
    while($r = myPear_db()->next_record($q)){
      $option = unserialize($r['lm_option']);
      unset($option['e_cc']);
      unset($option['e_inst']);
      myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($option), 1,'lm_option')." WHERE lm_id = $r[lm_id]",cnf_CLI);
    }
    */
  }
}

function signal_update($f){
  //  print "============================================================================================== $f\n";
}

/*
 * Replace emp_flavor -> e_org   2012-01-10
 */
function ea_update04(){
  if (!@$_SESSION[__METHOD__]++){
    signal_update(__METHOD__);
    $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP 'emp_|country|institute|flavor'",cnf_CLI);
    while($r = myPear_db()->next_record($q)){
      $option = unserialize($r['lm_option']);
      foreach($option as $k=>$v){
	$k_new = str_replace('emp_','e_',str_replace('0_flavor','e_org',$k));
	foreach(array('flavor'   =>'org',
		      'country'  =>'cc',
		      'institute'=>'inst') as $f=>$t) $k_new = str_replace($f,$t,$k_new);
	unset($option[$k]);
	if ($v == '0_flavor') $v = '20_mp';
	$option[$k_new] = $v;
      }
      myPear_db()->qquery("UPDATE zzz_list_members SET ".myPear_db()->quote(serialize($option), 1,'lm_option')." WHERE lm_id = $r[lm_id]");
    }
  }
}


/*                                            2011-09-25
 * Add lm_status 'a'->alumni, 'e'->employee
 * based on the employment record
 */
function ea_update03(){
  return;
  if (!@$_SESSION[__METHOD__]++){
    $q = myPear_db()->qquery("ALTER TABLE `zzz_unit_members` CHANGE `um_status` `um_status` VARCHAR( 128 ) NULL",cnf_CLI); 
    $q = myPear_db()->qquery("SELECT um_id FROM zzz_unit_members ".
   			      " LEFT JOIN zzz_lists        ON um_avid = l_parent ". // employment history                                                                                                     
			      " LEFT JOIN zzz_list_members ON l_id  = lm_lid ".
			      " WHERE um_uid=40092 AND (um_status != 'e') AND (lm_option REGEXP '(emp_flavor|e_org).*(nordita|40090)' AND (lm_status='present' OR lm_status>".date('"Y-m-d"',time())."))",cnf_CLI);
 
    while($r = myPear_db()->next_record($q)) myPear_db()->qquery("UPDATE zzz_unit_members SET um_status='e' WHERE um_id = $r[um_id]",False);
    myPear_db()->qquery("UPDATE zzz_unit_members SET um_status='a' WHERE um_uid = 40092 AND um_status != 'e'",False);
  }
}

// better code...
function ea_update02(){
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP '0_flavor'",cnf_CLI);
  while($r = myPear_db()->next_record($q)){
    $l = unserialize($r['lm_option']);
    $l['rec_type']   = '20_mp';
    myPear_db()->qquery("UPDATE zzz_list_members SET ".
			 myPear_db()->quote(serialize($l), 1,'lm_option').
			 " WHERE lm_id=$r[lm_id]",cnf_CLI);
  }
}

/*
 * More suitable packing 2011-05-27
 */
function ea_update01(){
  return;
    static $compat = array('1_ce' =>'10_cp',
			   '2_eh' =>'20_mp',
			   '3_phd'=>'34_phd',
			   '4_ms' =>'38_ms');

    // job notation is extended
    foreach($compat as $f=>$t){
      $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP '$f'",cnf_CLI);
      while($r = myPear_db()->next_record($q)){
	$l = unserialize($r['lm_option']);
	$l['rec_type'] = $t;
	myPear_db()->qquery("UPDATE zzz_list_members SET ".
			      myPear_db()->quote(serialize($l), 1,'lm_option').
			      " WHERE lm_id=$r[lm_id]",cnf_CLI);
      }
    }
    
    // emp_end is now in 'lm_status'
    $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_option REGEXP 'emp_end'",cnf_CLI);
    while($r = myPear_db()->next_record($q)){
      $l = unserialize($r['lm_option']);
      $emp_end = $l['emp_end'];
      unset($l['emp_end']);
      myPear_db()->qquery("UPDATE zzz_list_members SET ".
			   join(',',array(myPear_db()->quote(serialize($l), 1,'lm_option'),
					  myPear_db()->quote($emp_end,      1,'lm_status'))). 
			   " WHERE lm_id=$r[lm_id]",cnf_CLI);
    }
    
    // fine tuning
    myPear_db()->qquery("UPDATE zzz_list_members SET lm_status='present' WHERE lm_status='now'",cnf_CLI);
}
