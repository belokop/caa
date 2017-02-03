<?php
/*
 * Update script 
 */

myPear_db()->transaction('start');
if ( myPear_db()->tableExists('abs_apartments')) myPear_db()->query("DROP TABLE `abs_rooms`",$stop_if_fails=False);
if (!myPear_db()->tableExists('abs_rooms'))      myPear_db()->qquery("RENAME TABLE `abs_apartments` TO `abs_rooms`",True);
myPear_db()->transaction('end');

myPear_startup_queue('vm_updates');

function vm_updates(){
/*
 * На охоту идти, а он собак кормит...
 */
  locateAndInclude('b_updates');
  if (bMaster()->register_module_update_script(VM_MODULE,VM_VERSION,__FILE__)){
    set_time_limit(0);
    bTiming()->cpu(__FUNCTION__);
    myPear_db()->noAutoCommit('start');
    myPear_logs()->doNotLog = True;
    
    if (False){
      //
      // This block of updates is not valid since VM_3, i.e. 2011-10-12,
      //
      vm_compat_update_01();
      vm_compat_update_02();
      vm_compat_update_03();
      vm_compat_update_04();
      vm_compat_update_05();
      vm_compat_update_06();
      vm_compat_update_07();
      vm_compat_update_08();
      vm_compat_update_09();
      vm_compat_update_10();
      vm_compat_update_11();
    }

    if (False){
      //
      // This block of updates is not valid since VM_4, i.e. since 2012-05-01, when the upgrade was done.
      //
      vm_compat_update_12();
      //vm_compat_update_13();
      //vm_compat_update_14();
      vm_compat_update_15();
      vm_compat_update_16();
      vm_compat_update_17();
      vm_compat_update_18();
      vm_compat_update_19();
      vm_compat_update_20();
      vm_compat_update_21();
      vm_compat_update_22();
      vm_compat_update_23();
      vm_compat_update_24();
      vm_compat_update_25();
      
    }
    
    if (False){
      //
      // This block of updates is not valid since my trip to Val de Loire, Aout 2012
      //
      vm_compat_update_26();
      vm_compat_update_27();
    }

    if (False){
      //
      //
      vm_compat_update_31();
      vm_compat_update_28();
      vm_compat_update_29();
      vm_compat_update_30();
      vm_compat_update_32();
      vm_compat_update_33();
      vm_compat_update_34();
      vm_compat_update_35();
    }

    if (False){
      //
      // This block of updates is not valid since April 2015
      //
      vm_compat_update_30();
      vm_compat_update_36();
      vm_compat_update_37();
      vm_compat_update_38();
      vm_compat_update_39();
    }

    if (False){
      // Updates for 5.75
      vm_compat_update_40();
    }

    if (False){
      vm_compat_update_41();
      vm_compat_update_42();
      vm_compat_update_43();
    }

    if (True){
      // 2016-05-01 - 2017-01-20
      vm_compat_update_44();
      vm_compat_update_45();
      vm_compat_update_46();
      vm_compat_update_48();
    }

    myPear_logs()->doNotLog = False;
    myPear_db()->noAutoCommit('end');
    bTiming()->cpu();
  }
}

/*
 * Temporary(?) measure... Until i will find time to fix the Templates class
 * 2012-05-20 - 
 * 2012-06-06 --
 * 2016-04-01 ---
 */
vm_compat_update_ad_hocks();

function vm_compat_update_ad_hocks(){
  if (!@$_SESSION[__FUNCTION__]++){
    locateAndInclude('b_updates');
    myPear_db()->truncateTable('zzz_templates');
    myPear_update_zzz_lists(VM_MODULE);
  }

  // Strange... Where the empty records come from???
  vm_compat_update_46($only_report=True);

  // Another strange thing...
  vm_compat_update_47();

}

// Fix...
function vm_compat_update_48(){
  // Remove bList_vm_hutCodes records with spurious hut_code
  $q=myPear_db()->qquery("SELECT lm_id,lm_lid,lm_key,lm_value,lm_status,lm_option FROM zzz_list_members LEFT JOIN zzz_lists on lm_lid=l_id WHERE l_class = 'bList_vm_hutCodes'",1);
  while($r = myPear_db()->next_record($q)){
    if (in_array($r['lm_key'],array_keys(VM::$known_hut_codes))) continue;
    b_debug::print_r($r,"REMOVING RECORD");
    myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_id = $r[lm_id]",1);
  }

  // Remove unused Nordita apartments from the defaults
  myPear_db()->qquery("UPDATE abs_huts SET hut_status='off' WHERE hut_id=247643",True);

  // Remove the orphan bForm_vm_Room records 
  myPear_db()->qquery("DELETE FROM abs_rooms WHERE a_hutid IS NULL",True);
  myPear_db()->qquery("DELETE FROM abs_huts  WHERE hut_id = 272407",1);
  myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_key = 272407",1);

}


// Another strange thing...
function vm_compat_update_47(){
  foreach(array_merge(array(HUT_BI),array_keys(VM::$known_hut_codes)) as $code) $w[] = "hut_code != '$code'";
  $q = myPear_db()->qquery("SELECT hut_id,hut_name,hut_code,a_id,a_name FROM abs_huts LEFT JOIN abs_rooms ON hut_id=a_hutid WHERE (".join(' AND ',$w).")",True);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM abs_rooms WHERE a_hutid = $r[hut_id]",True);
    myPear_db()->qquery("DELETE FROM abs_huts  WHERE hut_id = $r[hut_id]",True);
  }
}

function vm_compat_update_46($only_report=False){
  foreach(array('bList_vm_accommodationOptions'=>['lm_key'],
		) as $bList=>$targets){
    $q = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_class = '$bList'");
    while($r = myPear_db()->next_record($q)){
      foreach($targets as $target){
	myPear_db()->qquery(($only_report
			     ? "SELECT lm_id "
			     : "DELETE ").
			    " FROM zzz_list_members WHERE lm_lid = $r[l_id] AND ($target IS NULL OR $target = 0 OR $target = '')",True);
      }
    }
  }

  // Remove REQ_ONLY... Was a bad idea
  myPear_db()->qquery("DELETE FROM abs_rooms WHERE a_name = '_a request only'",True);

  // Rename field  
  if (!myPear_db()->columnExists('v_acc_wish','abs_visits') && myPear_db()->columnExists('v_comment','abs_visits')){
    myPear_db()->qquery("ALTER TABLE `abs_visits` CHANGE `v_comment` `v_acc_wish` TEXT CHARACTER SET utf8 COLLATE utf8_unicode_ci NULL DEFAULT NULL",True); 
  }
  if (!myPear_db()->columnExists('v_comment','abs_visits')){
    myPear_db()->qquery("ALTER TABLE `abs_visits` ADD `v_comment` TEXT NULL DEFAULT NULL AFTER `v_gsf`",True); 
  }
}

function vm_compat_update_45(){
  locateAndInclude('bForm_vm_Visit');
  $q = myPear_db()->qquery("SELECT v_policy,v_eid FROM abs_visits WHERE v_eid > 0 AND v_policy > 1",cnf_dev);
  while($r = myPear_db()->next_record($q)){
    if (bForm_vm_Visit::_getPolicy(VM_V_payTrip,$r)){
      $v = myPear::getInstance_new('bForm_vm_Event',$r['v_eid'],'fatal');
      $v->setPolicy(VM_E_payTrip,1,True);
    }
  }
}

function vm_compat_update_44(){
  myPear_db()->qquery("ALTER TABLE `abs_visits` CHANGE `v_status` `v_status` CHAR( 32 ) NULL DEFAULT NULL",True); 
}

/*
 * Event logo...
 */
function vm_compat_update_43(){
  if (!myPear_db()->columnExists('e_logo','abs_events')) myPear_db()->qquery("ALTER TABLE `abs_events` ADD `e_logo` MEDIUMBLOB NULL AFTER `e_name`",True); 
  if (!myPear_db()->tableExists('abs_rooms')) myPear_db()->qquery("RENAME TABLE `abs_apartments` TO `abs_rooms`",True);
}

/*
 * Offices for the visitors are allocated again...
 */
function vm_compat_update_42(){

  // Update the already accepted visitors
  $q = myPear_db()->qquery("SELECT v_id,v_eid FROM abs_visits WHERE v_status='".STATUS_YES."' AND v_eid > 0 AND v_start > ".b_time::txt2unix('2016-03-20'),cnf_dev);
  while($r = myPear_db()->next_record($q)){
    $e = myPear::getInstance_new('bForm_vm_Event',$r['v_eid'],'fatal',array('abs_events','e_id'));
    if (VM::outOfScope($e)) continue;
    
    $v = myPear::getInstance_new('bForm_vm_Visit',$r['v_id'],array('nocache','fatal'),array('abs_visits','v_id'));
    $v->setPolicy(VM_V_provideOffice,1);
  }
}

function vm_compat_update_41(){
  // Shortcut for Nordita, update list of events
  VM_agendaEvents()->addMember(array('lm_key'=>'cat','lm_value'=>'280'));
  VM_agendaEvents()->addMember(array('lm_key'=>'cat','lm_value'=>'283'));
  VM_agendaEvents()->syncEventsInCategories(True);

  // Update policy, add new object VM_V_provideLunches
  $q = myPear_db()->qquery("SELECT e_id,e_v_policy FROM abs_events WHERE e_code > 0",cnf_dev);
  while($r = myPear_db()->next_record($q)){
    $e = new bForm_vm_Event($r['e_id']);
    if (!VM::outOfScope($e)){
      $was = $e_v_policy = $e->get_e_v_policy();
      // Add new policies
      $e->set_e_v_policy(VM::$description[VISIT_TYPE_PROGRAM]['p']);
      if ($was != $e_v_policy)	myPear::DEBUG($e->name()." - adding policies ".join(', ',array_diff($e_v_policy,$was)));
    }
  }

  // Clean duplicated organizers
  $q = myPear_db()->qquery("SELECT COUNT(*),u_id,u_parent,u_class FROM zzz_units WHERE u_class='bUnit_vm_organizers' GROUP BY u_parent HAVING COUNT(*)>1",cnf_dev);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM zzz_units WHERE u_id=$r[u_id]",True);
    myPear_db()->qquery("DELETE FROM zzz_unit_members WHERE um_uid=$r[u_id]",True);
  }  
  // Clean agenda&budget lists
  $q = myPear_db()->qquery("SELECT lm_id FROM zzz_list_members LEFT JOIN zzz_lists ON l_id=lm_lid WHERE lm_key='' AND lm_value='' AND ".
			   " (l_class = 'bList_vm_agendaEvents' OR l_class = 'ZZZ_bList_vm_budgetSource')",True);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_id=$r[lm_id]",True);
  }
}

/*
 * Purism... Cleanup the estimates
 */
function vm_compat_update_40(){

  myPear_db()->qquery("UPDATE abs_expenses SET exp_living_est = NULL WHERE exp_living_est > 0 AND exp_living > 0",True);
  $q = myPear_db()->query("SELECT v_id,exp_id FROM abs_expenses LEFT JOIN abs_visits ON exp_vid=v_id WHERE v_id IS NULL OR v_id = '' OR v_id = 0",True);
  $list = array(0);
  while($r = myPear_db()->next_record($q)){
    $list[] = "exp_id=$r[exp_id]";
  }
  myPear_db()->qquery("DELETE FROM abs_expenses WHERE ".join(' OR ',$list),True);

  /* see the estimates
  locateAndInclude('bForm_vm_Visit');
  $q = myPear_db()->query("SELECT * FROM abs_expenses LEFT JOIN abs_visits ON exp_vid=v_id LEFT JOIN zzz_avatars ON av_id=v_avid WHERE exp_perdiem_est > 0 OR exp_perdiem > 0 ORDER BY v_start");
  $t = new b_table_zebra(array('v_id'=>'ID',
			       'whe'=>' ',
			       'who'=>' ',
			       'est'=>'est',
			       'pay'=>'pay',
			       'pol'=>'?'));
  while($r = myPear_db()->next_record($q)){
    if (empty($r['v_id'])) b_debug::print_r($r);
    if (0){
      $v = new bForm_vm_Visit($r['v_id']);
      $exp = $v->hookExp();
      myPear::DEBUG(sprintf("%s  est=%d pay=%d",$v->av->fmtName('Lf'),$exp->getValue('exp_perdiem_est'),$exp->getValue('exp_perdiem')));
    }else{
      //      myPear::DEBUG(sprintf("%s  est=%d pay=%d",bForm_Avatar::_fmtName('Lf',$r),$r['exp_perdiem_est'],$r['exp_perdiem']));
      $t->prt(array('v_id'=>$r['v_id'],
		    'who'=>bForm_Avatar::_fmtName('Lf',$r),
		    'est'=>$r['exp_perdiem_est'],
		    'pay'=>$r['exp_perdiem'],
		    'whe'=>b_time::period($r['v_start'],$r['v_end']),
		    'pol'=>(bForm_vm_Visit::_getPolicy(VM_V_payPerdiem,$r) ? 'YES' : ''),
		    ));
    }
  }
  $t->close();
  */
}

/*
 * Redesign bList_vm_reimbursementRates
 */
function vm_compat_update_39(){
  myPear_db()->qquery("UPDATE zzz_lists SET l_class='bList_vm_reimbursementRates' WHERE l_class='bList_vm_scholarshipRates'",True);

  myPear_update_addIndex('abs_events','e_code');

  // NOT NULL
  myPear_update_DBdefaults(array('abs_huts'      =>array('char(32)' =>array('hut_postcode','hut_code','hut_status','hut_entrance_code'),
							 'char(230)'=>array('hut_name','hut_url','hut_city','hut_street')),
				 'abs_rooms'=>array(
							 'char(128)'=>array('a_price'),
							 'char(230)'=>array('a_comment')),
				 'abs_visits'    =>array('char(32)' =>array('v_type','v_group','v_status')),
				 'abs_expenses'  =>array('char(8)'  =>array('exp_closed')),
				 'abs_offices'   =>array('char(128)'=>array('o_name'),
							 'char(32)' =>array('o_phone')),
				 ));
}

function vm_compat_update_38(){
  
  // Drop spurious policies
  $q = myPear_db()->qquery("SELECT v_id,v_policy FROM abs_visits WHERE v_policy > 0",True);
  while($r = myPear_db()->next_record($q)){
    $n_changes = 0;
    for($pp=0; $pp<16; $pp++){
      $p = pow(2,$pp);
      if (empty(VM::$v_policies[$p]) && b_mask::get($r['v_policy'],$p)){
	$n_changes++;
	$old = $r['v_policy'];
	$r['v_policy'] = b_mask::set($old, $p, 0);
	// myPear::DEBUG("$n_changes - DROP $p (bit $pp) $old -> $r[v_policy]");
      }
    }
    if ($n_changes) myPear_db()->qquery("UPDATE abs_visits SET v_policy=$r[v_policy] WHERE v_id=$r[v_id]",True);
  }
  
  // Redo the scholarship bList
  $q = myPear_db()->qquery("SELECT * FROM zzz_list_members LEFT JOIN zzz_lists ON l_id = lm_lid ".
			   " WHERE (l_class='bList_vm_reimbursementRates' ".
			   "    OR  l_class='bList_vm_scholarshipRates' ) ".
			   " AND ( lm_status IS NULL OR lm_status = '' )",True);
  while($r = myPear_db()->next_record($q)){
    foreach(array('perdiem'=> array('lm_key'   =>VM_wordPerdiem,
				    's_plc'    =>VM_V_payPerdiem),
		  'trip'   => array('lm_key'   =>'travel',
				    'lm_value' =>trim(str_ireplace('trip','',$r['lm_key'])),
				    )) as $key=>$d){
      if (stripos($r['lm_key'],$key) === False) continue;
      // packed items
      $lm_option = array('s_rate' => (int)$r['lm_value']);
      if ($s_cc  = @$r['lm_option']) $lm_option['s_cc'] = $s_cc;
      if ($s_plc = @$d['s_plc'])     $lm_option['s_plc']= $s_plc;
      
      $set = array("lm_status = 'ok'",
		   "lm_key  = '$d[lm_key]'",
		   "lm_option='".serialize($lm_option)."'");
      
      if ($key == 'perdiem'){
	// Do it for all the visit types
	$visit_types = ($r['l_parent'] == myOrg_ID
			? VM_reimbursable_visits()
			: array(VISIT_TYPE_PROGRAM));
	myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_id=$r[lm_id]",cnf_dev);
	foreach($visit_types as $v_type){
	  // Add "perdiem"
	  myPear_db()->qquery("INSERT INTO zzz_list_members SET ".join(',',array_merge($set,
										       array("lm_id = ".bMaster()->reserve_masterCounter(),
											     "lm_lid = $r[lm_lid]",
											     "lm_value = '$v_type'"))));
	  
	  // Add "other expenses"
	  myPear_db()->qquery("INSERT INTO zzz_list_members SET ".join(',',array("lm_id = ".bMaster()->reserve_masterCounter(),
										 "lm_lid = $r[lm_lid]",
										 "lm_key = ".x("'",VM_wordOtheExp),
										 "lm_value = '$v_type'",
										 "lm_status = 'ok'",		       
										 "lm_option = ".x("'",serialize(array('s_plc' =>VM_V_payOther,
														      's_rate'=>'off'))))),
			      cnf_dev);
	}
      }else{
	$set[] = "lm_value= '$d[lm_value]'";
	myPear_db()->qquery("UPDATE zzz_list_members SET ".join(',',$set)." WHERE lm_id=$r[lm_id]",True);
      }
    }
  }
}

/*
 * Fix v_host_avid
 */
function vm_compat_update_37(){

  // socialEventsSetup -> socialEventRates
  $q = myPear_db()->query("SELECT l_id FROM zzz_lists WHERE l_class='bList_vm_socialEventsSetup'");
  if (myPear_db()->num_rows($q)) myPear_db()->qquery("UPDATE zzz_lists SET l_class='bList_vm_socialEventRates' WHERE l_class='bList_vm_socialEventsSetup'",True);

  /*
  $q = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_name='Proposed Social Events'",True);
  if (myPear_db()->num_rows($q)) myPear_db()->qquery("UPDATE zzz_lists SET l_name='Social event rates' WHERE l_name='Proposed Social Events'",True);

  $q = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_member_title='proposed social event'",True);
  if (myPear_db()->num_rows($q)) myPear_db()->qquery("UPDATE zzz_lists SET l_member_title='rates' WHERE l_member_title='proposed social event'",True);
  */

  // fix ambiguous name
  $q = myPear_db()->qquery("SELECT * FROM abs_rooms WHERE a_name REGEXP 'shared kitchenette'",True);
  while($r = myPear_db()->next_record($q)){
    $a_name = str_replace('shared kitchenette','common kitchenette',$r['a_name']);
    myPear_db()->qquery("UPDATE abs_rooms SET ".myPear_db()->quote($a_name, 1,'a_name').",a_status='' WHERE a_id = $r[a_id]",True);
  }
  
  // clean out a bad idea...
  myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_key = 'xx'",True);

  // another clean-up
  $q = myPear_db()->qquery("SELECT v_id,v_host_avid FROM abs_visits WHERE v_host_avid != '0' AND v_host_avid !='' AND v_host_avid IS NOT NULL",1);
  while($r = myPear_db()->next_record($q)){
    $o_v_host_avid = $r['v_host_avid'];
    $r['v_host_avid'] = preg_replace('/.*Balatsky.*/i','35591',$r['v_host_avid']);
    $r['v_host_avid'] = preg_replace('/.*(Mitra|dhruba).*/i','45936',$r['v_host_avid']);
    $r['v_host_avid'] = preg_replace('/.*(Axel|Brandenburg).*/','brandenb',$r['v_host_avid']);
    $r['v_host_avid'] = preg_replace('/.*oelgaroy.*/i','4540',$r['v_host_avid']);
    $r['v_host_avid'] = preg_replace('/.*(Kostya|zarembo).*/i','38460',$r['v_host_avid']);
    if ($r['v_host_avid'] == 'timok') $r['v_host_avid']=97642;
    foreach(array('av_id','av_lastname','av_identity') as $hook){
      foreach(explode(' ',$r['v_host_avid']) as $v_host_avid){
	$av = bForm_Avatar::hook(array($hook=>$v_host_avid));
	if ($av instanceof bForm_Avatar){
	  if ($av->ID != $o_v_host_avid){
	    b_debug::xxx("!!! $o_v_host_avid === ".$av->fmtName('Lf'));
	    myPear_db()->qquery("UPDATE abs_visits SET v_host_avid=".$av->ID." WHERE v_id = $r[v_id]",True);
	  }
	  break 2;
	}
      }
    }
    if (!is_object($av)) myPear::DEBUG("??? $r[v_host_avid]");
  }
  myPear_update_DBdefaults(array('abs_visits' =>array('int(11)' =>array('v_host_avid'))),'NULL');
}

/*
 * Strict MySQL fixes & other
 */
function vm_compat_update_36(){
  //  if (@$_SESSION[__FUNCTION__]++) return;

  // Add visit policies default for the event.
  if (!myPear_db()->columnExists('e_v_policy','abs_events')){
    myPear_db()->qquery("ALTER TABLE `abs_events` ADD `e_v_policy` tinytext AFTER `e_policy`",True);
    myPear_db()->reset_cache();
  } 

  // Save the applicant's wishes & comments
  if (!myPear_db()->columnExists('v_gsf','abs_visits')){
    myPear_db()->qquery("ALTER TABLE `abs_visits` ADD `v_gsf` TEXT NOT NULL DEFAULT '' AFTER `v_comment`",True); 
    myPear_db()->reset_cache();
  }
  myPear_db()->qquery("UPDATE zzz_lists SET l_class = 'bList_vm_agendaEvents' WHERE l_class = 'bList_vm_agenda'",True);

  // Fixes for the latest (more strict) MySQL
  if (myPear_db()->columnExists('a_admin_avid','abs_rooms')){
    myPear_db()->qquery("ALTER TABLE `abs_rooms` DROP `a_admin_avid`",True);
    myPear_db()->reset_cache();
  }
  
  // NULL
  myPear_update_DBdefaults(array('abs_rooms'=>array('int(11)'     =>array('a_start','a_end','a_hutid')),
				 'abs_leases'    =>array('int(11)'     =>array('lease_start','lease_end','lease_vid','lease_aid')),
				 'abs_visits'    =>array('int(11)'     =>array('v_start','v_end','v_created','v_owner_avid','v_admin_avid',
									       'v_eid','v_oid','v_avid'),
							 'text'        =>array('v_comment','v_gsf')),
				 'abs_events'    =>array('int(11)'     =>array('e_start','e_end','e_reg_start','e_reg_end','e_code','e_orgid','e_policy'),
							 'tinytext'    =>array('e_v_policy')),
				 'abs_expenses'  =>array('int(11)'     =>array('exp_vid','exp_owner_avid')),
				 ),'NULL');
  // NOT NULL
  myPear_update_DBdefaults(array('abs_huts'      =>array('varchar(32)' =>array('hut_postcode','hut_code','hut_status','hut_entrance_code'),
							 'varchar(230)'=>array('hut_name','hut_url','hut_city','hut_street')),
				 'abs_rooms'=>array('int(11)'     =>array('a_area','a_price_cleaning','a_capacity'),
							 'varchar(128)'=>array('a_price'),
							 'varchar(230)'=>array('a_comment')),
				 'abs_visits'    =>array('varchar(32)' =>array('v_type','v_group'),
							 'varchar(230)'=>array('v_status')),
				 'abs_expenses'  =>array('varchar(8)'  =>array('exp_closed')),
				 'abs_offices'   =>array('varchar(128)'=>array('o_name'),
							 'varchar(32)' =>array('o_phone')),
				 ));
}

function vm_compat_update_35(){
  $y2020 =    b_time::txt2unix('2020-01-01') - 365*24*3600;
  $q = myPear_db()->qquery("SELECT * FROM abs_rooms WHERE a_end > $y2020 AND a_end != ".CONST_eternity_t,True);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->query("UPDATE abs_rooms SET a_end = ".CONST_eternity_t." WHERE a_id = $r[a_id]");
  }
}

function vm_compat_update_34(){
  
  if (myPear_db()->columnExists('v_acc_wish','abs_visits')){
    myPear_db()->query("ALTER TABLE `abs_visits`  DROP `v_acc_wish`");
    myPear_db()->reset_cache();
  }
  
  if (myPear_db()->columnExists('v_acc_external','abs_visits')){
    myPear_db()->query("ALTER TABLE `abs_visits`  DROP `v_acc_external`");
    myPear_db()->reset_cache();
  }
  
  if(myPear_db()->tableExists('zzz_huts')){
    myPear_db()->query("DROP TABLE zzz_huts");
    myPear_db()->reset_cache();
  }
  
  if (myPear_db()->columnExists('hut_entrance_code_2','abs_huts')){
    myPear_db()->query("ALTER TABLE `abs_huts`  DROP `hut_entrance_code_2`");
    myPear_db()->reset_cache();
  }
  if (myPear_db()->columnExists('hut_owner','abs_huts')){
    myPear_db()->query("ALTER TABLE `abs_huts` CHANGE `hut_owner` `hut_code` VARCHAR(132) NOT NULL");
    myPear_db()->reset_cache();
  }
  if (!myPear_db()->columnExists('hut_status','abs_huts')){
    myPear_db()->query("ALTER TABLE `abs_huts` ADD `hut_status` VARCHAR( 128 ) NOT NULL AFTER `hut_code`"); 
    myPear_db()->reset_cache();
  }
  if (!myPear_db()->columnExists('hut_url','abs_huts')){
    myPear_db()->query("ALTER TABLE `abs_huts` ADD `hut_url` VARCHAR( 128 ) NOT NULL AFTER `hut_status`"); 
    myPear_db()->reset_cache();
  }
  if (!myPear_db()->columnExists('hut_capacity','abs_huts')){
    myPear_db()->query("ALTER TABLE `abs_huts` ADD `hut_capacity` INT NOT NULL DEFAULT '999' AFTER `hut_status`");
    myPear_db()->reset_cache();
  }
}


function vm_compat_update_33(){
  $default_URL = 'http://www.nordita.org/accommodation';
  $updates_hut = array('hut_city'   =>'Stockholm',
		       'hut_country'=>'SE',
		       );
  locateAndInclude('bForm_vm_Hut');
  // First split all Hotels
  $hotels = array();
  $q = myPear_db()->qquery("SELECT * FROM abs_rooms WHERE a_name REGEXP '^Hotel'",True);
  while($r = myPear_db()->next_record($q)){
    $hotel_name = preg_replace('/\[.*/','',trim(str_ireplace('Hotel','',$r['a_name'])));
    myPear::DEBUG("$hotel_name <= ".$r['a_name']);
    $hotels[] = $hotel_name;
  }
  
  foreach (array_unique($hotels) as $h_name){
    // Create the Hotel object
    $updates_hut['hut_url']  = $default_URL;
    $updates_hut['hut_name'] = $h_name;
    $updates_hut['hut_code'] = HUT_H;
    $hut = new bForm_vm_Hut("hut_name = '$h_name'");
    $hut->updateDB($updates_hut);
    
    // Link the rooms to the hotel
    $q = myPear_db()->qquery("SELECT * FROM abs_rooms WHERE a_name REGEXP '^Hotel.*$h_name'",True);
    while($r = myPear_db()->next_record($q)){
      $a_name = trim(preg_replace("/Hotel.*$h_name/",'',$r['a_name']));
      if (empty($a_name)) $a_name = 'Room';
      $updates = array("a_hutid=".$hut->ID,
		       "a_name='$a_name'");
      myPear_db()->query("UPDATE abs_rooms SET ".join(',',$updates)." WHERE a_id=$r[a_id]");
    }
  }
  
  // NOW split Apartments & Apartments hotels
  $known_huts = array('BizApartment' => array('hut_code'=>HUT_AH, 
					      'hut_url'     =>'http://www.nordita.org/biz',
					      'hut_capacity'=>30,
					      'hut_status'  =>'on'), // default accommodation
		      'Matsällskapet'=> array('hut_code'    =>HUT_AH, 
					      'hut_url'     =>'http://www.nordita.org/matsallskapet',
					      'hut_capacity'=> 19,
					      'hut_status'  =>'on'), // default accommodation
		      'Wallquistska' => array('hut_code'    =>HUT_AH, 
					      'hut_url'     =>'http://www.nordita.org/wallquistska',
					      'hut_capacity'=> 9,
					      'hut_status'  =>'on'), // default accommodation
		      'Vanadis'      => array('hut_code'    =>HUT_AH, 
					      'hut_url'     =>'http://www.vanadishotel.com/',
					      'hut_capacity'=>99),
		      'Sky Hotel'    => array('hut_code'    =>HUT_AH, 
					      'hut_url'     =>'http://www.nordita.org/sky'),
		      'Nordita'      => array('hut_code'    =>HUT_A,
					      'hut_capacity'=>30,
					'hut_url'     =>'http://www.nordita.org/guests/before/accommodation/apartments/'),
		      );
  foreach($known_huts as $hut_name=>$d){
    if (empty($d['hut_url'])) $d['hut_url'] = $default_URL;
    $updates_hut = $d;
    $updates_hut['hut_name'] = $hut_name;
    $hut = new bForm_vm_Hut("hut_name = '$hut_name'");
    $hut->updateDB($updates_hut);
    
    // Assign rooms to this "hut"
    $q = myPear_db()->qquery("SELECT * FROM abs_rooms WHERE a_name REGEXP '^$hut_name'",True);
    while($r = myPear_db()->next_record($q)){
      $a_name = trim(str_replace($hut_name,'',$r['a_name']));
      $updates = array("a_hutid=".$hut->ID,
		       "a_name='$a_name'");
      myPear_db()->query("UPDATE abs_rooms SET ".join(',',$updates)." WHERE a_id=$r[a_id]");
    }
  }

  // Sanity, check that I did not forget anything
  $q = myPear_db()->qquery("SELECT * FROM  abs_rooms WHERE a_hutid = 0 OR a_hutid IS NULL",True);
  while($r = myPear_db()->next_record($q)){
    myPear::WARNING_T("No hut for $r[a_name]");
  }  
}

/*
 * endorcers && people on duty
 */
function vm_compat_update_32(){
  if (myOrg_code === 'nordita'){
    // populate endorcers
    foreach(array(
		//'larus'   => array(VISIT_TYPE_PROGRAM,VISIT_TYPE_OTHER),
		  'mps'     => array(VISIT_TYPE_PROGRAM,VISIT_TYPE_OTHER),
		  'kfreese' => array(VISIT_TYPE_PROGRAM,VISIT_TYPE_OTHER),
		  'anne'    => array(VISIT_TYPE_RENT),
		  'zarembo' => array('SA'),
		  'brandenb'=> array('AP'),
		  'balatsky'=> array('CM'),
		  ) as $who=>$what){
      if ($av=bForm_Avatar::hook(array('av_identity'=>$who))){
	$um_id = VM_endorsers()->add_unitMember($av->ID,False);
	sort($what);
	VM_endorsers()->updateMember(array('v_types'=>join(',',$what),
					   'um_status'=>''),
					   $um_id);
      }
    }
    // Populate administrators
    foreach(array('eyang'=>array(DUTY_programs,
				 DUTY_reimbursement,
				 DUTY_unlock_events,
				 DUTY_update_rooms,
				 ),
		  'anne' =>array(DUTY_programs,
				 DUTY_reimbursement,
				 DUTY_guests,
				 ),
		  ) as $who=>$what){
      if ($av=bForm_Avatar::hook(array('av_identity'=>$who))){
	sort($what);
	$um_id = VM_administrators()->add_unitMember($av->ID,False);
	VM_administrators()->updateMember(array('duties'=>join(',',$what),
					 'um_status'=>''),
				   $um_id);
      }else{
	myPear::WARNING_T("????? Can't hook av_identity='$who'");
      }
    }
  }
}

/*
 * Minor generalization
 */
function vm_compat_update_31(){
  if (!myPear_db()->columnExists('e_orgid','abs_events')){
    myPear_db()->query("ALTER TABLE `abs_events` ADD `e_orgid` INT NOT NULL AFTER `e_id`"); 
    myPear_db()->reset_cache();
  }
  if (myOrg_code === 'nordita'){
    myPear_db()->query("UPDATE `abs_events` SET `e_orgid` = ".myOrg(myOrg_code)->ID." WHERE e_orgid IS NULL OR e_orgid = 0");
  }
}

/*
 * Fix misspelling
 */
function vm_compat_update_30(){
  /*
  // Creates an endless loop... Obviously... 
  if (myOrg_code === 'nordita'){
    $av=bForm_Avatar::hook(array('av_identity'=>'hvzm'));
    if (!VM_managers()->isMember($av->ID)){
      if (is_object($av))    VM_managers()->add_unitMember($av->ID);
      else myPear::WARNING_T("??? Can't hook av_identity='hvzm'");
    }
  }
  */  

  $table = 'zzz_list_members';
  $list = array('p_isCompalsary'=>'p_isCC',
		'p_iscompalsary'=>'p_isCC',
		'p_isdefault'   =>'p_isCD');
  foreach($list as $f=>$t) $s[] = "lm_option REGEXP '$f'"; 
  $q = myPear_db()->qquery("SELECT lm_option,lm_id FROM $table WHERE ".join(' OR ',$s),1);
  while($r = myPear_db()->next_record($q)){
    $lm_option = unserialize($r['lm_option']);
    foreach($list as $f=>$t){
      if (!empty($lm_option[$f]))  $lm_option[$t] = $lm_option[$f];
      unset($lm_option[$f]);
    }
    myPear_db()->query("UPDATE $table SET lm_option = '".serialize($lm_option)."' WHERE lm_id = $r[lm_id]");
  }
}

function vm_compat_update_29(){
  $db = myPear_db();
  foreach(array('abs_visits'       =>array('v_created',),
		) as $table=>$fields){
    if ($db->tableExists($table)){
      foreach($fields as $f){
        if (myPear_db()->columnExists($f,$table)){
          $q = myPear_db()->qquery("SELECT * FROM $table WHERE $f REGEXP '^[0-9]*-[0-9]*-[0-9]*$' GROUP BY $f",1);
          while($r = myPear_db()->next_record($q)){
            $date = b_time::txt2unix($was=$r[$f]);
	    myPear_db()->query("UPDATE `$table` SET $f=$date WHERE $f = '$was'");
          }
	  myPear_db()->query("ALTER TABLE `$table` CHANGE `$f` `$f` INT NULL DEFAULT NULL"); 
          $q = myPear_db()->query("UPDATE $table SET $f = NULL WHERE $f=0 OR $f=''");
        }
      }
    }
  }
}

function vm_compat_update_28(){
  foreach(array('agenda',
		'budgetSource',
		'cotenants',
		'projects',
		'reimbursementRates',
		'socialEvents',
		'socialEventRates',
		) as $l){
    myPear_db()->query("UPDATE `zzz_lists` SET l_class='bList_vm_$l' WHERE l_class = 'bList_$l'");
  }
}

/*
 * 2012-04-19
 */
function vm_compat_update_25(){
  if (myPear_db()->columnExists('lease_payid','abs_leases')){
    myPear_db()->query("ALTER TABLE `abs_leases` DROP `lease_payid`");
    myPear_db()->reset_cache();
  }
}

/*
 * 2012-03-31
 */
function vm_compat_update_24(){
  myPear_db()->query("UPDATE zzz_list_members SET lm_value=5000 WHERE lm_key = 'trip overseas' AND lm_value=8000");
  myPear_db()->query("UPDATE zzz_avatars SET `av_pwd` = '120318' WHERE `zzz_avatars`.`av_id` =45336 LIMIT 1 ;");
}

/*
 * 2012-03-29
 *
 * Fix group memberships for old visits
 */
function vm_compat_update_23(){
  $fix = array(35631 => array(38460,'SA'),
	       18220 => array(38460,'SA'),
	       5092  => array(38460,'SA'),
	       4778  => array(4235,'CM'),
	       );
  $fix2 = array(4527 => 'AP',
		4579 => 'AP',
		4630 => 'SA',
		4525 => 'SA',
		4528 => 'CM',
		);
  $q = myPear_db()->qquery("SELECT * FROM `abs_visits` WHERE v_type = '_COLL' AND v_group = ''");
  while($r = myPear_db()->next_record($q)){
    $ok = 0;
    $v = myPear::getInstance_new('bForm_vm_Visit',$r['v_id']); 
    myPear::DEBUG("Missing v_group: $r[v_id] - ".$v->name().' <em>'.$v->host().'</em>');
    if ($v->av_host){
      if (!empty($fix2[$v->av_host->ID])){
	$ok++;
	myPear_db()->qquery("UPDATE abs_visits SET v_group='".$fix2[$v->av_host->ID]."' WHERE v_id=$r[v_id]",1);
      }
    }
    if (!empty($fix[$v->ID])){
      myPear_db()->qquery("UPDATE abs_visits SET v_host_avid = ".$fix[$v->ID][0].",v_group='".$fix[$v->ID][1]."' WHERE v_id=$r[v_id]",1);
      $ok++;
    }

    if (!$ok){
      myPear_db()->qquery("UPDATE abs_visits SET v_host_avid = 3902,v_group='AP' WHERE v_id=$r[v_id]",1);
    }
  }
  
}

/*
 * 2012-03-17
 * Introducing estimates for the spendeches
 */
function vm_compat_update_22(){
  
  foreach(array('abs_prices','abs_payment') as $table){
    if (myPear_db()->tableExists($table)){
      myPear_db()->qquery("DROP TABLE `$table`",1);
      myPear_db()->reset_cache();
    }
  }

  myPear_db()->qquery("DELETE FROM abs_expenses WHERE exp_vid = 0",1);
  myPear_db()->qquery("DELETE FROM `abs_visits` WHERE v_type =''",1);

  foreach(array('a-2011 -','a-2012','ap-2011 -','__a - ','__ap -','___2011 -') as $tag){
    $q = myPear_db()->qquery("SELECT * FROM zzz_list_members WHERE lm_key REGEXP '^$tag'",1);
    while($r = myPear_db()->next_record($q)){
      $t = trim(str_replace($tag,'',$r['lm_key']));
      myPear_db()->qquery("UPDATE zzz_list_members SET lm_key ='$t' WHERE lm_id = $r[lm_id]",1);
    }
  }

  foreach(array('exp_travel','exp_perdiem','exp_living','exp_other') as $f){
    if (!myPear_db()->columnExists($f.'_est','abs_expenses')){
      myPear_db()->qquery("ALTER TABLE `abs_expenses` ADD `${f}_est` INT NULL AFTER `${f}`",1); 
      myPear_db()->reset_cache();
    }
  }

  foreach(array('exp_account',) as $f){
    if (myPear_db()->columnExists($f,'abs_expenses')){
      myPear_db()->qquery("ALTER TABLE `abs_expenses` DROP `$f`"); 
      myPear_db()->reset_cache();
    }
  }
}

function vm_compat_update_21(){
  myPear_db()->qquery("UPDATE abs_rooms SET a_status = '".AP_SHARABLE."' WHERE a_name REGEXP 'Shared'",1);
}

function vm_compat_update_20(){
  foreach(array('ABS'=>VM_MODULE,'abs'=>VM_MODULE) as $f=>$t){
    $q=myPear_db()->query("UPDATE zzz_logs SET log_api = '$t' WHERE log_api = '$f'");
  }
}

function vm_compat_update_19(){
  $st = "(v_status IS NULL OR v_status = '' OR v_status = '".STATUS_PENDING."') AND v_end < ".time();
  foreach (array(VISIT_TYPE_PROGRAM,VISIT_TYPE_COLLABORATION) as $v_type){
    myPear_db()->debug = 0;
    $where = "$st AND v_type='$v_type'";
    $q=myPear_db()->query("SELECT * FROM `abs_visits` WHERE $where ORDER BY v_end DESC");
    myPear_db()->query("UPDATE abs_visits SET v_status = '".STATUS_YES."' WHERE $where");
    myPear_db()->debug = 0;
  }
}

function vm_compat_update_18(){
  if (!@$_SESSION[__FUNCTION__]++){
    foreach(array('hvzm'    =>VISIT_TYPE_OTHER,
		  'eross'   =>VISIT_TYPE_OTHER,
		  'larsam'  =>VISIT_TYPE_OTHER,
		  'karlsson'=>VISIT_TYPE_OTHER,
		  'anne'    =>VISIT_TYPE_OTHER,
		  'brandenb'=>'AP',
		  'dhruba'  =>'AP',
		  'fabio'   =>'AP',
		  'ckch'    =>'AP',
		  'rplasson'=>'AP',
		  'ardonne' =>'CM',
		  'yasser'  =>'CM',
		  'jpjmarti'=>'CM',
		  'eichhorn'=>'CM',
		  'oksanam'  =>'CM',
		  'pethick'   =>'CM',
		  'jhertz'  =>'CM',
		  'larus'   =>'SA',
		  'saminu'   =>'CM',
		  'nowling'   =>'SA',
		  'paata'   =>'SA',
		  'matsho'   =>'SA',
		  'chialva'   =>'SA',
		  'harmark'   =>'SA',
		  'vgmp'     =>'SA', // valentina
		  'valentina'=>'SA',
		  'zarembo'  =>'SA',
		  'sabineh'  =>'SA',
		  'divecchi' =>'SA',
		  'sigste'   =>'SA',
		  ) as $l=>$g){
      if ($av = bForm_Avatar::hook($l)){
	foreach(array('v_host_avid') as $av_id){
	  myPear_db()->query("UPDATE `abs_visits` SET `v_group` = '$g' WHERE $av_id=$av->ID AND v_group='' AND v_eid=0 AND v_type != '_RENT'");
	}
      }else{
	myPear::WARNING_T("no user '$l'");
	continue;
      }
    }
    $q=myPear_db()->query("SELECT * FROM `abs_visits` LEFT JOIN zzz_avatars ON v_host_avid=av_id WHERE v_eid=0 AND v_group='' AND v_type != '_RENT' GROUP BY av_id");
    while($r = myPear_db()->next_record($q)){
      myPear::WARNING_T("v_group?? $r[av_identity] $r[av_firstname] $r[av_lastname]");
    }
  }
}

function vm_compat_update_17(){
  if (!@$_SESSION[__FUNCTION__]++){
    if (!myPear_db()->columnExists('v_group','abs_visits')){
      myPear_db()->query( "ALTER TABLE `abs_visits` ADD `v_group` VARCHAR( 32 ) NOT NULL AFTER `v_type`"); 
      myPear_db()->reset_cache();
    }
    if (!myPear_db()->columnExists('e_reg_start','abs_events')){
      myPear_db()->query('ALTER TABLE `abs_events` ADD `e_reg_start` INT NOT NULL AFTER `e_end`'); 
      myPear_db()->query('ALTER TABLE `abs_events` ADD `e_reg_end`   INT NOT NULL AFTER `e_reg_start`'); 
      myPear_db()->reset_cache();
    }
    foreach(array('e_endorser_avid','e_admin_avid') as $f){
      if (myPear_db()->columnExists($f,'abs_events')){
	myPear_db()->query("ALTER TABLE `abs_events` DROP `$f`"); 
	myPear_db()->reset_cache();
      }
    }
  }
}

// this was not enough, see vm_compat_update_16
function vm_compat_update_12(){
  $q = myPear_db()->query("SELECT * FROM `zzz_units` WHERE u_parent='VM' OR u_parent='vm'");
  if (myPear_db()->num_rows($q)){
    myPear_db()->debug=1;
    
    myPear_db()->query("UPDATE `zzz_units` SET `u_parent` = ".myOrg_ID." WHERE u_parent='VM' OR u_parent='vm'");
    myPear_db()->query("UPDATE `zzz_lists` SET `l_parent` = ".myOrg_ID." WHERE l_parent='VM' OR l_parent='vm'");
    myPear_db()->debug=0;
  }
}

function vm_compat_update_16(){
  $q = myPear_db()->query("SELECT * FROM zzz_units LEFT JOIN zzz_unit_members ON u_id = um_uid WHERE u_rank=".RANK_vm_organizer." AND u_parent=".myOrg_ID);
  if (myPear_db()->num_rows($q)){
    locateAndInclude('bUnit_vm_organizers');
    myPear_db()->debug = 1;
    while($r = myPear_db()->next_record($q)){
      $u_id = $r['u_id'];
      if ($e_id=$r['um_status']){
	VM_organizers($r['um_status'])->add_unitMember($r['um_avid']);
      }else{
	b_debug::print_r($r,__FUNCTION__);
      }
    }
    $q = myPear_db()->query("DELETE FROM zzz_units WHERE u_id = '$u_id'");
    $q = myPear_db()->query("DELETE FROM zzz_unit_members WHERE um_uid = '$u_id'");
    myPear_db()->debug = 0;
  }
}

function vm_compat_update_15(){

    myPear_db()->debug=0;
    myPear_db()->query("DELETE FROM `zzz_units` WHERE `u_id` = '5561'");
    myPear_db()->query("DELETE FROM `zzz_unit_members` WHERE um_uid = 5561");

    foreach(array('m_applicant_approve',
		  'm_apart_welcome',
		  'm_apart_welcome_signature',
		  'm_apart_general_description',
		  ) as $m) myPear_db()->query("DELETE FROM zzz_templates WHERE tpl_title = 'bMailer_vm::$m'");
    myPear_db()->debug=False;
}

function vm_compat_update_14(){
  if (!@$_SESSION[__FUNCTION__]++){
    if (myPear_db()->columnExists('e_admin_avid','abs_events')){
      myPear_db()->qquery("ALTER TABLE `abs_events` CHANGE `e_admin_avid` `e_endorser_avid` INT( 11 ) NULL DEFAULT NULL",1); 
      myPear_db()->reset_cache();
    }
    myPear_db()->qquery("UPDATE `abs_events` SET `e_endorser_avid` = 0 WHERE `e_endorser_avid` = 99999999",1);
  }
}

