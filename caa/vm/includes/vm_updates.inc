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
    // Updates take time...
    ini_set("memory_limit", "-1");
    set_time_limit(0);

    bTiming()->cpu(__function__);
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

    if (False){
      // 2016-05-01 - 2017-01-20
      vm_compat_update_44();
      vm_compat_update_45();
      vm_compat_update_46();
      vm_compat_update_48();
    }

    if (False){ // Fixes for the version "Easter 2017"
      vm_compat_update_49();
      vm_compat_update_50();
    }

    if (False){ // Fixes June 2017
      vm_compat_update_51();
      vm_compat_update_52();
    }

    if (True){ // Fixes end June 2017
      vm_compat_update_53();
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
  if (!@$_SESSION[__function__]++){
    locateAndInclude('b_updates');
    myPear_db()->truncateTable('zzz_templates');
    myPear_update_zzz_lists(VM_MODULE);
  }

  // Strange... Where the empty records come from???
  vm_compat_update_46($only_report=True);

  // Another strange thing...
  vm_compat_update_47();

}

/*
 * This is EX private function set_buildInObjects(){}
 */
function vm_compat_update_53(){

  locateAndInclude('bForm_vm_Visit');
  
  myPear_db()->qquery(sprintf("UPDATE abs_leases JOIN abs_visits ON v_id=lease_vid  SET lease_aid=%s WHERE v_eid=272179 AND lease_aid=%s",5486,338272),True);
  
  myPear_db()->qquery("UPDATE zzz_lists SET l_name='Event accommodation options' WHERE l_name='Selected accommodation options'",True);

  foreach(VM_projects()->get_projects($buildInOnly=True) as $id=>$type){
    switch($type){
    case VISIT_TYPE_RENT:
      myPear_db()->query("UPDATE abs_visits SET v_projectid=$id WHERE v_type = '".VISIT_TYPE_RENT."'");
      break;
      
    case LODGING_OA:
      /*
      // assign visits without the project number to OA project                                                                                                                
      $q = myPear_db()->query(sprintf("SELECT v_id FROM abs_leases LEFT JOIN abs_visits ON lease_vid=v_id WHERE v_projectid=0 AND lease_aid='%s'",VM::$OA->ID));
      while ($r = myPear_db()->next_record($q)){
	$visit = new bForm_vm_Visit($r['v_id']);
	$visit->defineVariables();
	$visit->updateDB(array('v_projectid' => $id));
	}
      */
      break;
      
    case LODGING_BR:
      break;

    default:
      // assign the unknown project to OA 
      b_debug::_debug($msg="Unexpected project type='$type' id=$id",True);
      myPear::WARNING_T($msg);
      myPear_db()->query("UPDATE abs_visits SET v_projectid = 31026 WHERE v_projectid=$id",True);
      myPear_db()->query("DELETE FROM zzz_list_members WHERE lm_id=$id",True);
    }
  }
}

function vm_compat_update_52(){
  // There might be many projects for 1 event now...
  $q = myPear_db()->qquery("ALTER TABLE `abs_visits` CHANGE `v_projectid` `v_projectid` CHAR(32) CHARACTER SET utf8 COLLATE utf8_unicode_ci NULL DEFAULT NULL",cnf_dev);
 
  // Add "cotenant extra charge" to the room prices
  locateAndInclude('bList_vm_accommodationOptions');
  $q = myPear_db()->qquery("UPDATE abs_huts SET hut_name='BizApartment Gärdet' WHERE hut_id=247638",True);
  $q = myPear_db()->qquery("SELECT a_id,a_price,a_name FROM abs_rooms WHERE a_hutid = 247638 AND a_capacity > 1",True);
  while($r = myPear_db()->next_record($q)){
    $price = explode(',',$r['a_price']);
    if ((strpos($r['a_price'],'cot') == False) && (strpos($r['a_price'],'=') !== False)){
      $price = explode(',',$r['a_price']);
      $price[] = 'cot=100';
      myPear_db()->qquery(sprintf("UPDATE abs_rooms SET a_price='%s' WHERE a_id = %s",join(',',$price),$r['a_id']),True);
    }

    $id2xs= bList_vm_accommodationOptions::id2x($r['a_id']);
    $qq = myPear_db()->qquery(sprintf('SELECT lm_id,lm_option FROM zzz_list_members WHERE lm_option REGEXP "[[:<:]]%s[[:>:]]"',$id2xs),True);
    while($rr = myPear_db()->next_record($qq)){
      $lm_option = unserialize($rr['lm_option']);
      if ((strpos($lm_option[$id2xs],'cot') == False) && (strpos($lm_option[$id2xs],'=') !== False)){
	$lm_option[$id2xs] .= ',cot=100';
      }
      myPear_db()->qquery(sprintf("UPDATE zzz_list_members SET lm_option='%s' WHERE lm_id=%s",serialize($lm_option),$rr['lm_id']),True);
    }
  }
}

function vm_compat_update_51(){
  foreach(VM::$known_hut_codes as $code=>$descr){
    VM_hutCodes()->addMember(array('lm_key'   => $code,
				   'lm_value' => $descr['prefix'].'s'));
  }
  foreach(array(397690,
		397691) as $hut_id){
    myPear_db()->query("UPDATE abs_huts SET hut_code='".HUT_HS."' WHERE hut_id = $hut_id",True);
  }
}

// Set the "room is full flag" if there are <a_capacity> cotenants
// Set the "room is full flag" if there are there are accompaning persons
function vm_compat_update_50(){
  if (!VM::$isReady || !myPear_db()->columnExists('lease_option','abs_leases')) return;
  //  if (@$_SESSION[__function__]++) return;
  b_debug::xxx(' ');
  $leases = array();
  $dejaVu = array();
  foreach(VM_cotenants()->getMembers() as $lm_id=>$rec){
    $leases = explode(',',$rec['leases']);
    bList_vm_cotenants::set_options($leases);
    foreach($leases as $lease_id){
      $l = bForm_vm_Lease::_leaseInfo($lease_id,array('lease_aid','v_accompaning','a_capacity','lease_id'));
      bForm_vm_Lease::_v_accompaning($l);
    }
  }
}

function vm_compat_update_49_2_Kronor_Hostel(){
  static $h_2kr = 0;
  if (!empty($h_2kr)) return;

  b_debug::xxx();
  locateAndInclude('bForm_vm_Hut');
  
  $h_2kr = myPear::getInstance_new('bForm_vm_Hut',array('hut_name'=>'2 Kronor Hostel'),'strict');
  if (empty($h_2kr)){
    $h_2kr = new bForm_vm_Hut('empty');
    $h_2kr->updateDB(array('hut_name'=>'2 Kronor Hostel',
			   'hut_code'=>'H',
			   'hut_city'=>'Stockholm',
			   ));
  }
  
  $r = myPear::getInstance_new('bForm_vm_Room',array('a_hutid'=>$h_2kr->ID),'strict');
  if (empty($r)) {
    $r = myPear::getInstance_new('bForm_vm_Room',array('a_name'=>'2 Kronor Hostel'),'strict');
    $r->updateDB(array('a_name' => 'Bed',
		       'a_hutid'=> $h_2kr->ID,
		       ));
  }
  //  vm_compat_update_49_visits("a_id = ".$r->ID);

  
  $h_2kr = new bForm_vm_Hut('empty');
  $h_2kr->updateDB(array('hut_name'=>'Interhostel AB',
			 'hut_city'=>'Stockholm',
			 'hut_code'=>'H',
			 ));
  $r = myPear::getInstance_new('bForm_vm_Room',5495);
  $r->updateDB(array('a_name' => 'Bed',
		     'a_hutid'=> $h_2kr->ID,
		     ));
  //  vm_compat_update_49_visits("a_id = 5495");
  //  vm_compat_update_49_visits("lease_option = 'a:0:{}'");
}

function vm_compat_update_49_visits($where){
  b_debug::xxx();
  ob_start();
  locateAndInclude('bHolder_vm_Visits');
  $holder = new bHolder_vm_Visits();
  $holder->noSort= True;
  $holder->where[] = $where;
  $holder->listEntries();
  $list = ob_get_contents();
  ob_end_clean();
  myPear::MESSAGE($list);
}


/*
 * Preparation for the new rooms classification
 */
function vm_compat_update_49(){
  static $dejaVu = 0;
  if ($dejaVu++) return;
  b_debug::xxx();
  if (myPear_db()->columnExists('a_quantity','abs_rooms')){
    myPear::MESSAGE("Conversion is already done!");
    return;
  }

  vm_compat_update_49_check_lease_aid('update starts');

  // Fixes  
  myPear_db()->qquery("UPDATE abs_huts SET hut_url='http://www.elite.se' WHERE hut_id=271758",True);

  // Add a few new columns
  if (!myPear_db()->columnExists('a_quantity','abs_rooms')){
    myPear_db()->qquery($sql="ALTER TABLE `abs_rooms` ADD `a_quantity` VARCHAR(200) NULL DEFAULT '".VM_default_a_quantity."' AFTER `a_capacity`",True); 
    b_debug::xxx($sql);
  }

  if (!myPear_db()->columnExists('lease_option','abs_leases')){
  //myPear_db()->qquery($sql="ALTER TABLE `abs_leases` ADD `lease_option` VARCHAR(200) DEFAULT '".serialize(array('pay'=>VM_default_lease_pay))."' AFTER `lease_end`",True);
    myPear_db()->qquery($sql="ALTER TABLE `abs_leases` ADD `lease_option` VARCHAR(200) DEFAULT '".serialize(array())."' AFTER `lease_end`",True);
    b_debug::xxx($sql);
  }

  // Remove "a_code", it is redundant...
  if (myPear_db()->columnExists('a_code','abs_rooms'))
    myPear_db()->qquery("ALTER TABLE `abs_rooms` DROP `a_code`",True);
  if (myPear_db()->columnExists('a_price_cleaning','abs_rooms'))
    myPear_db()->qquery("ALTER TABLE `abs_rooms` DROP `a_price_cleaning`",True);
  if (myPear_db()->columnExists('hut_capacity','abs_huts'))
    myPear_db()->qquery("ALTER TABLE `abs_huts` DROP `hut_capacity`",True);

  //
  // decrease number of default hotels
  //
  foreach(array(271758,268297,271673,271758) as $hut_id) {
    myPear_db()->qquery("UPDATE abs_huts SET hut_status = 'off' WHERE hut_id=$hut_id",True);    
  }
  
  //
  // Remove obsolete entries
  // 
  $a_id2remove = array(42323,34518,271674,100748,126131,100746,100747,125270);
  $a_id2remove = array(42323,34518,271674);
  
  if (!function_exists('vm_compat_update_49_cleanup')){
    function vm_compat_update_49_cleanup($f,$ids){
      $table = 'abs_leases';
      if (strpos($f,'hut_') === 0) $table = 'abs_huts';
      if (strpos($f,'a_')   === 0) $table = 'abs_rooms';
      foreach($ids as $id){
	myPear_db()->qquery("DELETE FROM $table WHERE $f = '$id'",True);
	if ($f == 'a_id')  vm_compat_update_49_cleanup('lease_aid',array($id));
	if ($f == 'hut_id')vm_compat_update_49_cleanup('a_hutid',  array($id));
      }	
    }
  }
  foreach(array('a_id'    =>$a_id2remove,
		'hut_id'  =>array(247626),
		) as $f=>$ids){
    b_debug::xxx("removing $f=".join(',',$ids));
    vm_compat_update_49_cleanup($f,$ids);
  }
  
  vm_compat_update_49_2_Kronor_Hostel();
  unset($_SESSION['a_price_myOrg']);

  //
  // Parse all the rooms names, extract options and write them to the database
  //
  $pivots = $table = array();
  $q = myPear_db()->qquery("SELECT hut_id,hut_name,hut_code,a_id,a_capacity,a_name,a_comment,a_price,a_status,lease_end,lease_id ".
			   " FROM abs_rooms ".
			   " LEFT JOIN abs_huts ON a_hutid=hut_id ".
			   " LEFT JOIN abs_leases ON lease_aid=a_id ".
			   // " WHERE lease_aid IS NOT NULL ".
			   " GROUP BY a_id ".
			   " ORDER BY CONCAT(lease_end,hut_name) DESC"
			   ,True);
  while($r = myPear_db()->next_record($q)){
    if ($r['hut_name'] == HUT_BI) continue;
    if (empty($r['a_id'])){
      b_debug::print_r($r,'??????????????????????????????????');
      continue;
    }
    vm_compat_update49_pack_me($r['a_price']);

    // parse...
    list($a_name,$a_pay,$sharable,$a_comment,$pay_comment) = vm_compat_update_49_parser($r,$table);
    if (empty($a_pay)) $a_pay = VM_default_lease_pay;
    @$a_pays[$a_pay]++;
   
    vm_compat_update_49_set($r['a_id'],$a_name,$a_pay,$a_comment,$r['a_capacity']);

    // See the usage of the room
    $qqq = myPear_db()->qquery("SELECT lease_end FROM abs_leases WHERE lease_aid=".$r['a_id'],True);
    $latest_lease = $n = 0;
    while($rrr = myPear_db()->next_record($qqq)){
      $n++;
      $latest_lease = max($latest_lease,$rrr['lease_end']);
    }
    $room = array("Usage $n".($n>0?', last '.date('Y-m-d',$latest_lease):''),
		  $r['a_capacity'],
		  'price='.$r['a_price']);
    if ($a_comment)       $room[] = $a_comment;
    if ($v=@$r['a_start'])$room[] = date('Y-m-d',$v);
    if ($v=@$r['a_end'])  $room[] = date('Y-m-d',$v);
    if ($sharable)        $room[] = 'shared';
    if ($a_pay)           $room[] = "a_pay=$a_pay";

    // Build "shrinking table"
    if (empty($pivots[$r['hut_name']][$a_name]['root']) 
	&& !$sharable 
	&& ($n>0) 
	&& ($r['a_price'] !== '999=0')
	&& empty($a_pay) 
	){
      array_unshift($room,$r['a_id']);
      $pivots[$r['hut_name']][$a_name]['root'] = $room;
    }else{
      if (($root_capac=@$pivots[$r['hut_name']][$a_name]['root'][2]) && ($room[1] == $root_capac)) unset($room[1]);
      if (($root_price=@$pivots[$r['hut_name']][$a_name]['root'][3]) && ($room[2] == $root_price)) unset($room[2]);
      if($sharable)
       $shrbl[$r['hut_name']][$a_name][$r['a_id']] = $room;
      $pivots[$r['hut_name']][$a_name][$r['a_id']] = $room;
    }
  }
  //  b_debug::print_r($table);
  b_debug::print_r($a_pays,'a_pays','nosort');
  // b_debug::print_r($pivots,'pivots','nosort');
  // b_debug::print_r($shrbl, 'shrbl', 'nosort');

  if (!function_exists('update49_get_pivot')){
    function update49_get_pivot($d1){
      $reply = @$d1['root'][0];
      if (empty($reply)){
	$reply = array_keys($d1);
	$reply = $reply[0];
      }
      return $reply;
    }
  }

  //
  // Matsällskapet has 5 rooms with optional extra bed 
  //
  $hutm = myPear::getInstance_new('bForm_vm_Hut',array('hut_name'=>'Matsällskapet'),array('fatal'));
  $a_name = 'Room with kitchenet & extra bed';
  b_debug::xxx("Creating <em>$a_name</em> in ".$hutm->name());
  $r = new bForm_vm_Room('empty');
  $r->updateDB(array('a_name'    => $a_name,
		     'a_price'   => '999=680',
		     'a_area'    => 25,
		     'a_capacity'=> 2,
		     'a_hutid'   => $hutm->ID));

  // Complete the story
  locateAndInclude('bForm_vm_Room');
  locateAndInclude('bList_vm_accommodationOptions');
  foreach ($pivots as $hut_name=>$d0){
    foreach ($d0   as $a_name=>$d1){
      // See the pivot
      $a_root = new bForm_vm_Room(update49_get_pivot($d1));
      if (count(array_keys($d1)) == 1){ //  && !empty($d1['root'])){
	myPear::MESSAGE(x('h3',"OK=".$a_root->ID."=========================$hut_name.$a_name "));
	$ids  = array(0);
	$id2xs= array(bList_vm_accommodationOptions::id2x($a_root->ID));
      }else{
	if (0){
	  // Show the room description in the MESSAGE pane
	  ob_start();
	  $a_root->getForm();
	  $ob = ob_get_contents();
	  ob_end_clean();
	  myPear::MESSAGE(x('h3','============================'.$a_root->name()));
	  myPear::MESSAGE($ob);
	}
	$id2xs = $ids = array();
	foreach ($d1 as $a_id=>$room){
	  if ($a_id == 'root') continue;
	  b_debug::print_r($room,"$a_id --> ".$a_root->ID);
	  $ids[] = $a_id;
	  $id2xs[] = bList_vm_accommodationOptions::id2x($a_id);
	}
      }
      sort($ids);
            myPear::MESSAGE('a_ids='.implode(',',$ids));	

      myPear_db()->qquery("UPDATE abs_leases SET lease_aid = ".$a_root->ID." WHERE lease_aid = ".join(' OR lease_aid = ',$ids),True);
      $ids2delete = array_diff($ids,array($a_root->ID));
      myPear_db()->qquery("DELETE FROM abs_rooms WHERE a_id = ".join(' OR a_id = ',$ids2delete),True);

      if (True){
	//
	// Just DELETE all the bList_vm_accommodationOptions, it is too complicated t
	// Lets assume that the last ones are correct 
	//
	$q = myPear_db()->qquery("SELECT l_id FROM zzz_lists WHERE l_class = 'bList_vm_accommodationOptions'",True);
	while($r = myPear_db()->next_record($q)){
	  myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_lid=$r[l_id]",True);
	  myPear_db()->qquery("DELETE FROM zzz_lists WHERE l_id=$r[l_id]",True);
	}
      }elseif(False){
	vm_compat_update_49_convert_accommodationOptions($r,$ids,$id2x,$a_id2remove);
      }
    }
  }
  unset($_SESSION['a_price_myOrg']);

  //
  // Set the known acc. options
  //
  vm_compat_update_49_set_prices(Null,
				 array('BizApartment' => array('1'   => array('a_quantity'=>15),
							       '2'   => array('a_quantity'=>10),
							       '3'   => array('a_quantity'=>VM_quantity2request),
							       '4'   => array('a_quantity'=>VM_quantity2request),
							       ),
				       'Arcadia'      => array('1'   => array('a_price' => 34465,
									      'a_name'  => 'Superior Single'),
							       '2'   => array('a_price' => 40550,
									      'a_name'  => 'Superior'),
							       ),
				       'Vanadis'      => array('1'   => array('a_price' => "999=575.89"),
							       ),
				       'Sky Hotel'    => array('1'   => array('a_price' => "3=1201,29=961,999=894"),
							       ),				     
				       'Matsällskapet'=> array('any' => array('a_quantity'=>VM_quantity_total.'=5'),
							       '1'   => array('a_price' => "999=580"),
							       '2'   => array('a_price' => "999=680"),
							       ),
				       'Nordita'      => array('any' =>array('a_quantity'=>1),
							       ),
				       ));
  foreach(array(275657 => array( // Nov 2017 - International Workshop on Multifunctional Dirac Materials
				 ),
		276145 => array( // Oct 2017 - Many-Body Quantum Chaos, Bad Metals and Holography
				   ),
		267926 => array( // Sep 2017 - Current and Future Trends in Stochastic Thermodynamics
				   ),
		272180 => array( // Aug 2017 - Exascale Thinking of Particle Energization Problems
				 ),
		271769 => array( // Aug 2017 - Multicomponent and Strongly Correlated Superconductivity
				   ),
		272179 => array( // Aug 2017 - Nordita Master Class 2017
				 ),
		267917 => array( // Jul 2017 - Topological Phases in Cold Atom Systems
				   ),
		272163 => array( // Jul 2017 - Quantum Connections in Sweden
				   ),
		267906 => array( // Jul 2017 - Advances in Theoretical Cosmology in Light of Data
				   ),
		259860 => array( // Jun 2017 - The Physics of Extreme-Gravity Stars
				'BizApartment' => array('1'   => array('a_price'=>'3=938,999=675'),
							'2'   => array('a_price'=>'3=974,29=791'),
							'3'   => array('a_quantity'=>VM_quantity2request),
							'4'   => array('a_price'=>'3=3752,999=2700')
							),
				 ),
		267898 => array( // May 2017 - Phase Transitions in Astrophysics, from ISM to Planets
				'BizApartment' => array('1'   => array('a_price'=>'3=938,999=697'),
							'2'   => array('a_price'=>'3=974,999=813'),
							'3'   => array('a_quantity'=>VM_quantity2request),
							'4'   => array('a_quantity'=>VM_quantity2request),
							),
				 ),
		272157 => array( // May 2017 - Frontiers of topological quantum matter
				   ),
		) as $accOpt=>$array){
    vm_compat_update_49_set_prices($accOpt,$array);
  }
  
  unset($_SESSION['a_price_myOrg']);

  // Check the result
  vm_compat_update_49_check_lease_aid('update completed');
}

function vm_compat_update_49_set_prices($accOptions=Null,$array){
  b_debug::xxx();
  foreach($array as $hut_name=>$hut_data){
    if (empty($hut_data)) continue;
    $hut = myPear::getInstance_new('bForm_vm_Hut',array('hut_name'=>$hut_name),array('fatal'));
    b_debug::xxx("Updating options for ".$hut->name());
    if (!empty($accOptions) && !is_object($accOptions)){
      myPear::DEBUG(__function__."($accOptions): Activate VM_accommodationOptions($accOptions)"); 
      $accOptions = VM_accommodationOptions($accOptions);
    }
    foreach($hut_data as $a_capacity=>$data){
      $where = "a_hutid = ".$hut->ID . ($a_capacity==='any'
					? ''
					: " AND a_capacity='$a_capacity'");
      foreach($data as $column=>$value){
	if (is_object($accOptions) && in_array($column,bList_vm_accommodationOptions::$whatIsSaved)){
	  $q = myPear_db()->qquery("SELECT a_id FROM abs_rooms WHERE $where",True);
	  while($r = myPear_db()->next_record($q)){
	    $accOptions->set_a_value($value,$r['a_id'],$hut->ID,$column);
	  }
	}else{
	  myPear_db()->qquery("UPDATE abs_rooms SET $column = '$value' WHERE $where",True);
	}
      }
    }
  }
}

function vm_compat_update_49_check_lease_aid($id){
  b_debug::xxx();
  $q = myPear_db()->qquery("SELECT * FROM abs_leases LEFT JOIN abs_rooms ON lease_aid = a_id WHERE lease_aid > 0 GROUP BY lease_aid",True);
  while($r = myPear_db()->next_record($q)){
    if (empty($r['a_id'])) b_debug::print_r($r,'????????????? empty $a_id '.$id);
  }
}

/*
 *
 */
function vm_compat_update_49_convert_accommodationOptions($r,$ids,$id2xs,$a_id2remove){
  
  b_debug::xxx();
    myPear::MESSAGE('id2xs='.implode(',',$id2xs));	

  $q = myPear_db()->qquery("SELECT lm_id,lm_key,lm_option FROM zzz_list_members LEFT JOIN zzz_lists ON lm_lid=l_id".
			   " WHERE l_class = 'bList_vm_accommodationOptions' AND lm_option REGEXP '".join('|',$id2xs)."'", True);
  $q = myPear_db()->qquery("SELECT lm_id,lm_key,lm_option FROM zzz_list_members LEFT JOIN zzz_lists ON lm_lid=l_id".
			   " WHERE l_class = 'bList_vm_accommodationOptions'",True);
  while($r = myPear_db()->next_record($q)){
    $lm_option = unserialize($r['lm_option']);
    
    // Check that the record is not empty
    unset($lm_option['nrooms']);
    foreach($a_id2remove as $a_id) unset($lm_option[bList_vm_accommodationOptions::id2x($a_id)]);
    if (empty($lm_option)){
      myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_id=$r[lm_id]",True);
      continue;
    }
    
    if ($matches=array_intersect($id2xs,array_keys($lm_option))){
      $hut = new bForm_vm_Hut($r['lm_key']);
      // First unset removed rooms
      foreach($id2xs as $id) unset($lm_option[$id]);
      // Set the pivot
      $lm_option[bList_vm_accommodationOptions::id2x($a_root->ID)] = 33333; // some strange value
      ksort($lm_option);
      b_debug::print_r($lm_option,'pivot='.bList_vm_accommodationOptions::id2x($a_root->ID),'nosort');
      if (0) var_dump(($matches=array_intersect($id2xs,array_keys($lm_option))),
		      array_diff(array_keys($lm_option),$matches));
      myPear_db()->qquery("UPDATE zzz_list_members SET lm_option = '".serialize($lm_option)."' WHERE lm_id=$r[lm_id]",True);
      //      myPear::MESSAGE($hut->name()."-$r[lm_key]; lm_option=".b_fmt::joinX(', ',$lm_option));
      //      myPear::MESSAGE("matches=".join(', ',$matches));
    }
  }
  
  // Check duplicated accommodation records
  $q = myPear_db()->qquery("SELECT COUNT(*) as count, hut_name,lm_key,lm_id,lm_option FROM zzz_list_members ".
			   " LEFT JOIN zzz_lists ON l_id=lm_lid ".
			   " LEFT JOIN abs_huts ON hut_id=lm_key ".
			   " WHERE l_class = 'bList_vm_accommodationOptions' ".
			   " GROUP BY CONCAT(lm_key,lm_option)  ".
			   " HAVING count>1 ORDER BY lm_timestamp DESC",True);
  $lm_option = array();
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM zzz_list_members ".
			" WHERE lm_key = $r[lm_key] AND lm_id != $r[lm_id] AND lm_option = '$r[lm_option]'",True);
    $r['lm_option'] = unserialize($r['lm_option']);
    $lm_option[$r['lm_key']] = $r['lm_option'];
    b_debug::print_r($r,'DUPLICATIONS');
  }
  
  $q = myPear_db()->qquery("SELECT COUNT(*) as count, hut_name,lm_key,lm_id,lm_option FROM zzz_list_members ".
			   " LEFT JOIN zzz_lists ON l_id=lm_lid ".
			   " LEFT JOIN abs_huts ON hut_id=lm_key ".
			   " WHERE l_class = 'bList_vm_accommodationOptions' ".
			   " GROUP BY CONCAT(lm_key)  ".
			   " HAVING count>1 ORDER BY concat(hut_name)",True);
  while($r = myPear_db()->next_record($q)){
    unset($r['count']);
    b_debug::print_r($r,'multiple rooms');
    $qq = myPear_db()->qquery("SELECT hut_name,lm_key,lm_option FROM zzz_list_members ".
			      " LEFT JOIN abs_huts ON hut_id=lm_key ".
			      " WHERE lm_key=$r[lm_key] ORDER BY lm_key",True);
    while($rr = myPear_db()->next_record($qq)){
      b_debug::print_r($rr);
    }
  }
}

/*
 *
 */
function vm_compat_update_49_set($a_id,$a_name,$a_pay,$a_comment,$a_capacity){
  locateAndInclude('bForm_vm_Lease');
  
  // Set the "pay" option
  foreach(array('nothing','one','two',VM_default_lease_pay) as $o){
    if (strpos($a_pay,$o) !== False) $pay_option = $o;
  }
  if (empty($pay_option)) $pay_option = VM_default_lease_pay;

  // Save it in the lease
  b_debug::xxx($pay_option);
  $q = myPear_db()->qquery("SELECT lease_id FROM abs_leases WHERE lease_aid = $a_id");
  while($r = myPear_db()->next_record($q)) bForm_vm_Lease::_set_who_pays($r['lease_id'],$pay_option);

  // Set room name & comment
  foreach (array('a_name','a_comment') as $f){
    $v = ucfirst(strtolower($$f));
    myPear_db()->qquery("UPDATE abs_rooms SET $f = '$v' WHERE a_id = $a_id",True);
  }
}

function vm_compat_update49_pack_me(&$text){
  $text = trim(str_replace('  ',' ',
			   str_replace('  ',' ',
				       str_ireplace(array('A bed in Double','Usually not a fixed rate'),'',
						    str_replace(array(';','-',',','[',']','(',')','"'),' ',
								$text)))));
}

function vm_compat_update_49_parser($r,&$table){
  b_debug::xxx();
  $std_names = array('Superior Single',
		     'Superior',
		     'Studio Small',
		     'Studio Medium',
		     // 'Studio Standard',
		     'Studio Large',
		     'One Bedroom Apartment',
		     'Studio',
		     'Guest apartment',
		     );

  $a_name    = $r['a_name'];
  $a_comment = $x = $r['a_comment'];
    
  // Fix "syntax errors"
  $fixes = array(
		 '/ *\]/' => ']',
		 '/\(ex \d*\)/' => '',
		 '/\[\d[RL\*]*\]/' => '',    
		 '/^\[(\d)([;\s])*(\w)/' => '[\\1] [\\3',
		   );  
  $a_name    = trim(preg_replace(array_keys($fixes),array_values($fixes),$a_name));
  $a_comment = trim(preg_replace('/Studio Standard/i','Studio Medium',$a_comment));
  if ($a_comment != $x) myPear::DEBUG("a_comment:<br>$x<br>$a_comment");
  
  // Strip out the capacity from the room name
  $a_name= trim(str_replace('['.$r['a_capacity'].']','',$a_name));
  if (empty($r['a_capacity'])) b_debug::print_r($r,'empty a_capacity');
  
  // Extract payments
  //     from a_name
  $pay = '/((Nordita)?[\w ]*pay(s|ed)[\w\s]+)/i';
  $pay = '/([\w ]*pay(s|ed)[\w\s]+)/i';
  preg_match($pay,$a_name,$match1);
  if ($a_pay = trim(@$match1[0])){
    $a_name = str_replace($a_pay,'',$a_name);
  }
  //     from a_comment
  preg_match($pay,$r['a_comment'],$match2);
  if ($pay_comment = @$match2[0]) $a_comment = trim(str_ireplace($pay_comment,'',$a_comment));
  //     from a_price
  if ($r['a_price']==='999=0') $a_pay = 'nothing';

  // Extract the room name
  if    (preg_match('/('.join('|',$std_names).')/i',$a_name,$match))    $a_name = $match[1];
  elseif(preg_match('/('.join('|',$std_names).')/i',$a_comment,$match)) $a_name = $match[1];
  if ($a_name == 'Superior') $a_name .= ' Double';
  $a_comment = str_ireplace($std_names,'',$a_comment);
  
  // Sharable room
  $sharable = ($r['a_status'] == 'shared');
  $a_name   = str_ireplace('shared','',$a_name);
  $a_comment= str_ireplace('shared','',str_replace(array('A bed in','Double'),'',$a_comment));
  
  // Cosmetics...
  $x = 'PAID BY THE TENANT';
  if (stripos($a_comment,$x)!== False){
    $pay_comment = $x;
    $a_comment = str_ireplace($x,'',$a_comment);
  }
  
  // Check what we got as the room name...
  vm_compat_update49_pack_me($a_name);
  //  if (($r['hut_name'] == 'BizApartment') && (strToLower($a_name) == 'studio standard')) $a_name = 'Studio Medium'; 
  //  if ((strToLower($r['hut_name']) == '2 kronor hostel') && (strToLower($a_name) == 'studio standard')) $a_name = 'Studio Medium'; 
  if ((strToLower($r['hut_name']) == 'wenner-gren center') && (strToLower($a_name) == 'room')) $a_name = 'Room single'; 
  if ((strToLower($r['hut_name']) == 'matsällskapet')){
    global $m_hut_id;
    $m_hut_id = $r['hut_id'];
    $a_name = 'Room with kitchenet'; 
    $a_comment = '';
  }

  if (empty($a_name)){
    switch ($r['a_capacity']){
    case 0:
      b_debug::print_r($r,"??? capacity 0");
    case 1: 
      $a_name = 'Room Single';
      if ($r['hut_name'] == 'Sky Hotel')    $a_name = 'Studio Small';
      break;
    case 2: 
      $a_name = 'Room Double';
      if ($r['hut_name'] == 'BizApartment') $a_name = 'Studio Medium'; 
      if ($r['hut_name'] == 'Wallquistska') $a_name = 'apartment';
     break;
    case 3:
      $a_name = 'Studio Large';
      break;
    case 4: 
      if ($r['hut_name'] == 'BizApartment') $a_name = 'One Bedroom Apartment';
      break;
    default: b_debug::internalError("??? capacity ".$r['a_capacity']);
    }
  }
    
  //
  // Print summary
  //
  $name = array();
  
  vm_compat_update49_pack_me($a_comment);
  vm_compat_update49_pack_me($a_name);

  if (!$a_name) $a_name = '???a_name';
  if ($a_name){
    $name[] = $r['a_capacity'];
    $name[] = $a_name;
    $id = bForm_vm_Room::_a_name($r,False);
    @$table[$r['hut_name']][$id]++;
    if (0){
      $qq = myPear_db()->qquery("SELECT lease_id FROM abs_leases WHERE lease_aid=".$r['a_id'],True);
      @$table[$r['hut_name']][$id.'_used'] += myPear_db()->num_rows($qq);
    }
  }
  
  vm_compat_update49_pack_me($a_pay);
  if ($a_pay){
    $name[] = $a_pay;
    @$table['pay'][$a_pay]++;
  }
  
  if ($pay_comment){
    vm_compat_update49_pack_me($pay_comment);
    if (False)      $name[] = $pay_comment;
    @$table['pay_comment'][$pay_comment]++;
  }
  
  if ($sharable){
    $name[] = 'shared';
    @$table['shared']['shared']++;
  }

  $was = $now = array();
  if (!empty($r['a_name']))    $was[] = $r['a_name'];
  if (!empty($r['a_comment'])) $was[] = $r['a_comment'];
  if (!empty($name))           $now[] = join(' -- ',$name);
  if (!empty($a_comment))      $now[] = $a_comment;
  
  if (empty($a_name) || ($a_name == '???a_name')){
    myPear::DEBUG("a_pay=$a_pay;  a_name=$a_name");
    
    $w=join(' ### OOO ### ',$was);
    $n=join(' ### NNN ### ',$now);
    myPear::DEBUG($r['hut_name']."<br>$w<br>$n");
  }

  $reply = array($a_name,$a_pay,$sharable,$a_comment,$pay_comment);
  return $reply;
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
  $q = myPear_db()->qquery("SELECT hut_id FROM abs_huts WHERE (".join(' AND ',$w).")",True);
  while($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("DELETE FROM abs_rooms WHERE a_hutid = ".$r['hut_id'],True);
    myPear_db()->qquery("DELETE FROM abs_huts  WHERE  hut_id = ".$r['hut_id'],True);
  }
}

function vm_compat_update_46($only_report=False){
  foreach(array('bList_vm_accommodationOptions'=>array('lm_key'),
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
  //  if (@$_SESSION[__function__]++) return;

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
				 'abs_rooms'     =>array('int(11)'     =>array('a_area','a_price_cleaning','a_capacity'),
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
  myPear_db()->qquery("UPDATE abs_rooms SET a_status = '".'shared'."' WHERE a_name REGEXP 'Shared'",1);
}

function vm_compat_update_20(){
  foreach(array('ABS'=>VM_MODULE,'abs'=>VM_MODULE) as $f=>$t){
    $q=myPear_db()->query("UPDATE zzz_logs SET log_api = '$t' WHERE log_api = '$f'");
  }
}

function vm_compat_update_19(){
  $st = "(v_status IS NULL OR v_status = '' OR v_status = '".STATUS_PENDING."') AND v_end < ".time();
  foreach (array(VISIT_TYPE_PROGRAM,VISIT_TYPE_COLLABORATION) as $v_type){
    bDB::$_debug = False;
    $where = "$st AND v_type='$v_type'";
    $q=myPear_db()->query("SELECT * FROM `abs_visits` WHERE $where ORDER BY v_end DESC");
    myPear_db()->query("UPDATE abs_visits SET v_status = '".STATUS_YES."' WHERE $where");
    bDB::$_debug = False;
  }
}

function vm_compat_update_18(){
  if (!@$_SESSION[__function__]++){
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
  if (!@$_SESSION[__function__]++){
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
    bDB::$_debug = True;
    
    myPear_db()->query("UPDATE `zzz_units` SET `u_parent` = ".myOrg_ID." WHERE u_parent='VM' OR u_parent='vm'");
    myPear_db()->query("UPDATE `zzz_lists` SET `l_parent` = ".myOrg_ID." WHERE l_parent='VM' OR l_parent='vm'");
    bDB::$_debug = False;
  }
}

function vm_compat_update_16(){
  $q = myPear_db()->query("SELECT * FROM zzz_units LEFT JOIN zzz_unit_members ON u_id = um_uid WHERE u_rank=".RANK_vm_organizer." AND u_parent=".myOrg_ID);
  if (myPear_db()->num_rows($q)){
    locateAndInclude('bUnit_vm_organizers');
    bDB::$_debug = True;
    while($r = myPear_db()->next_record($q)){
      $u_id = $r['u_id'];
      if ($e_id=$r['um_status']){
	VM_organizers($r['um_status'])->add_unitMember($r['um_avid']);
      }else{
	b_debug::print_r($r,__function__);
      }
    }
    $q = myPear_db()->query("DELETE FROM zzz_units WHERE u_id = '$u_id'");
    $q = myPear_db()->query("DELETE FROM zzz_unit_members WHERE um_uid = '$u_id'");
    bDB::$_debug = False;
  }
}

function vm_compat_update_15(){

    bDB::$_debug = False;
    myPear_db()->query("DELETE FROM `zzz_units` WHERE `u_id` = '5561'");
    myPear_db()->query("DELETE FROM `zzz_unit_members` WHERE um_uid = 5561");

    foreach(array('m_applicant_approve',
		  'm_apart_welcome',
		  'm_apart_welcome_signature',
		  'm_apart_general_description',
		  ) as $m) myPear_db()->query("DELETE FROM zzz_templates WHERE tpl_title = 'bMailer_vm::$m'");
    bDB::$_debug=False;
}

function vm_compat_update_14(){
  if (!@$_SESSION[__function__]++){
    if (myPear_db()->columnExists('e_admin_avid','abs_events')){
      myPear_db()->qquery("ALTER TABLE `abs_events` CHANGE `e_admin_avid` `e_endorser_avid` INT( 11 ) NULL DEFAULT NULL",1); 
      myPear_db()->reset_cache();
    }
    myPear_db()->qquery("UPDATE `abs_events` SET `e_endorser_avid` = 0 WHERE `e_endorser_avid` = 99999999",1);
  }
}

