<?php
locateAndInclude('bForm_vm');

define('as_cotenant',' (as a co-tenant)');
//define('as_cotenant','');

class bForm_vm_Lease extends bForm_vm{

  private static $isReady = False;
  public static $_dbg_color = 'maroonText';

  /*
   * Debugging....
   */  
  public static function print_lease_options(){
    if (!isset($_GET['_once'])) return;

    // vm_compat_update_50();

    $cotenants = array();
    foreach(VM_cotenants()->getMembers() as $lm_id=>$rec){
      foreach(VM_cotenants()->safe_explode(@$rec['leases']) as $lease_id) $cotenants[] = $lease_id;
    }

    $t = new b_table_zebra(array('room'  =>'room',
				 'lease_id'=>'lease_id',
				 'period'=>'period',
				 'who'   =>'who',
				 'acc'   =>'',		
				 'pay'   =>'',
				 'cot'   =>'',
				 'ful'   =>'',
				 'x' =>'',
				 ));

    $q = myPear_db()->qquery(sprintf("SELECT ".
				     "lease_id,lease_start,lease_end,lease_option,".
				     "hut_id,hut_code,hut_name,".
				     "a_name,a_capacity,".
				     "v_accompaning,".
				     "av_lastname,av_firstname ".
				     "FROM abs_leases ".
				     "LEFT JOIN abs_rooms ON a_id=lease_aid ".
				     "LEFT JOIN abs_huts ON hut_id=a_hutid ".
				     "LEFT JOIN abs_visits ON v_id=lease_vid ".
				     "LEFT JOIN zzz_avatars ON av_id=v_avid ".
				     "WHERE (hut_code != '%s') AND (v_status = '%s') ".
				     (is_object(VM::$e) ? " AND v_eid=".VM::$e->ID." " : "").
				     "AND ((lease_option REGEXP '%s' AND lease_option NOT REGEXP 'a:1:.s:3:.pay.;s:10:.everything.;.') ".		
				     "OR (lease_id IN (%s)) ".
				     "OR (v_accompaning>0)) ".
				     "ORDER BY CONCAT(hut_name,a_capacity,av_lastname)",
				     HUT_BI,STATUS_YES,
				     join('|',array('pay',VM_room_is_full,VM_no_cotenants)),
				     join(', ',array_unique($cotenants))					    
				     ),cnf_dev);
    while($r = myPear_db()->next_record($q)){
      $o = unserialize($r['lease_option']);
      if (@$o['pay'] === 'everything') $o['pay'] = '';
      @$pays[' '.b_fmt::joinX(',',$o)]++;
      $room = str_replace('.',' ',str_replace(' ','_',bForm_vm_Room::_a_name($r,True,False)));
      $cotenants = VM_cotenants()->get_cotenants($r['lease_id'],False,'names'); 
      $t->prt(array('room'  =>($room == @$room_was ? '' : $room),
		    'who'   =>bForm_Avatar::_fmtName('Lf',$r),
		    'period'=>b_time::period($r['lease_start'],$r['lease_end']),
		    'pay'   =>@$o['pay'],
		    'cot'   =>(!empty($o[VM_no_cotenants]) ? VM_no_cotenants : ''),
		    'ful'   =>(!empty($o[VM_room_is_full]) ? VM_room_is_full : ''),
		    'x'     =>implode(',',$cotenants),
		    'acc'   =>(($n=$r['v_accompaning'])?"me+$n":""),
		    'lease_id'=>$r['lease_id'],
		    ));
      $room_was = $room;
    }
    $t->close();
    // b_debug::print_r($pays,'who pays');
  }
  
  /*
   * Create menu "Nordita pays for N tenants only"
   * 'lease_option' will be a serialized array
   */
  public static function set_lease_options($args,$data){
    $reply = self::get_lease_options($args,'set',$data,False);
    b_debug::_debug($reply,cnf_dev);
  }

  public static function get_lease_options($lease_id,$mode,$data=array(),$verbose=False){

    // Parse input arguments
    if ($is_array=is_array($lease_id)){
      $args = $lease_id;
      foreach($args as $k=>$v) {
	switch ($k){
	case 'lease_aid':
	  // lease_id has the precedence
	  if (!empty($column)) break;
	case 'lease_id':
	  $column = $k; 
	  $lease_id = $v; 
	  break;
	}
      }
    }else{
      $column   = 'lease_id'; 
      $args = array($column => $lease_id);
    }    

    // Silently refuse empty & "own accommodation" records   
    if (isset($args['lease_aid'])&& VM_self_accommodated($args)) return Null;
    if (isset($args['v_status']) && ($args['v_status'] !== STATUS_YES))  return Null;
    
    bTiming()->cpu(__function__);

    // Check the cache
    static $cache = array(),$cache_count = array();
    if (strpos($mode,'set') === False) { 
      $cacheID = serialize(array($column,$lease_id,$mode,$data));
      $msg = "call #".(int)(@$cache_count[$cacheID]++);
      if (isset($cache[$cacheID])) {
	//	if (!empty($cache[$cacheID]))
	b_debug::_debug("$msg, reply=".b_debug::parse_value($cache[$cacheID]),$verbose);
	bTiming()->cpu();
	return $cache[$cacheID];
      }
    }
    
    // Complete the arguments for the recursive invocations
    $select = array();
    foreach(array_merge(array($data,
			      'v_status',
			      'hut_code',
			      'a_capacity',
			      'lease_id',
			      'lease_aid',
			      'lease_option'),
			($mode == 'pay_menu' // Those extra fields needed for the menu only
			 ? array('av_lastname','av_firstname')
			 : array())) as $item){
      if (is_string($item) && !isset($args[$item]) && myPear_db()->columnExists($item)) $select[] = $item;
    }
    

    // Get sure that all the fields are read    
    if (!empty($select)){ 
      //      if ($is_array) b_debug::traceBack();
      b_debug::_debug('select='.join(',',$select),cnf_dev);
      $q = myPear_db()->qquery($sql="SELECT ".join(',',$select)." FROM abs_leases ".
			       " LEFT JOIN abs_visits ON v_id=lease_vid ".
			       " LEFT JOIN abs_rooms ON a_id=lease_aid ".
			       " LEFT JOIN abs_huts  ON hut_id=a_hutid ".
			       ($mode == 'pay_menu' ? 'LEFT JOIN zzz_avatars  ON av_id=v_avid' : '').
			       " WHERE hut_code != '".HUT_BI."' AND v_status='".STATUS_YES."' AND $column = '$lease_id'",cnf_dev);
      while($r = myPear_db()->next_record($q)){
	foreach($select as $item) $args[$item] = $r[$item];
      }
    }

    // Silently refuse empty records    
    if ((@$args['v_status'] !== STATUS_YES) || VM_self_accommodated($args)){
      if (isset($cacheID)) $cache[$cacheID] = Null;
      bTiming()->cpu();
      return Null;
    }	       
 
    $lease_option = (empty($args['lease_option']) 
		     ? array('pay'=>VM_default_lease_pay)
		     : unserialize($args['lease_option']));
    
    //
    // Do the work
    //
    switch($mode){
      
    case 'get':
      // Sic, the reply is an array !!
      $reply = $lease_option;
      if (is_string($data))     $reply[$data] = $args[$data];
      if (empty($reply['pay'])) $reply['pay'] = VM_default_lease_pay;
      break;
      
    case 'reset':
      $verbose = $verbose || cnf_dev;
      if (empty($data)){
	$lease_option = array('pay'=>VM_default_lease_pay);
      }else{
	$lease_option = self::get_lease_options($args,'get');
	if (!is_array($data)) $data = array($data);
	foreach($data as $item) unset($lease_option[$item]);
	ksort($lease_option);
      }
      myPear_db()->qquery("UPDATE abs_leases SET lease_option='".serialize($lease_option)."' WHERE $column='$lease_id'",cnf_dev);
      $reply = b_fmt::joinX(', ',$lease_option);
      break;
      
    case 'set':
      // Get the changes
      foreach($data as $k=>$v){
	$lease_option[$k] = $v;
	$args[$k] = $v;
	if (strToLower($v) === 'null') b_debug::internalError("'null' to be written");
      }
      // Update
      ksort($lease_option);
      myPear_db()->qquery($sql="UPDATE abs_leases SET lease_option='".serialize($lease_option)."' WHERE $column='$lease_id'",cnf_dev);

      // Sanity
      if (array_intersect(array_keys($data),array(VM_no_cotenants))){
	if(!bForm_vm_Room::_mightBeSharable($args)){
	  b_debug::internalError("setting VM_no_cotenants, but the room is not _mightBeSharable");
	}
      }

      // Set flag "No cotenants" if the organization pays only for X tenants
      // (i.e. the remaining beds to be paid by the tenant, the room gets fully occupied)
      if (@$data['pay'] == VM_pay_something){
	if (!function_exists('aid2leases')){
	  // helper...
	  function aid2leases($column,$lease_id){
	    $leases = array();
	    $q = myPear_db()->qquery("SELECT lease_id FROM abs_leases WHERE $column='$lease_id'",True);
	    while($r = myPear_db()->next_record($q)) $leases[] = $r['lease_id'];
	    return array_unique($leases);
	  }
	}
	foreach(aid2leases($column,$lease_id) as $l_id){
	  $_leaseInfo = self::_leaseInfo($l_id);
	  self::set_lease_options($_leaseInfo,array(VM_room_is_full=>1),True);
	  if (cnf_dev){
	    $room = myPear::getInstance_new('bForm_vm_Room',$_leaseInfo['lease_aid'],'strict');
	    myPear::WARNING("Impose VM_room_is_full ".$room->name(True));
	  }
	  if (bForm_vm_Room::_mightBeSharable($_leaseInfo)){
	    if (($n = count(VM_cotenants()->get_cotenants($l_id,True,'names'))) && !self::get_lease_options($_leaseInfo,VM_no_cotenants)){
	      self::set_lease_options($_leaseInfo,array(VM_no_cotenants=>1),True);
	      if (cnf_dev){
		$room = myPear::getInstance_new('bForm_vm_Room',$_leaseInfo['lease_aid'],'strict');
		myPear::WARNING("Impose VM_no_cotenants  $n/$_leaseInfo[a_capacity]".$room->name(True));
	      }
	    }
	  }
	}
      }
      $reply = b_fmt::joinX(',',$lease_option);
      break;
      
    case 'get_pay':
      $r = self::get_lease_options($args,'get');
      switch($r['pay']){ 
      case VM_pay_nothing:
      case VM_pay_everything:
        $reply = $r['pay'];
	break;
      case VM_pay_something:
	$reply = array($args['lease_id']=>bForm_Avatar::_fmtName('Lf',$args));
	foreach(VM_cotenants()->get_cotenants($args,False,'info') as $l_id=>$_fmtName){
	  $rr = self::get_lease_options($l_id,'get');
	  if ($rr['pay'] == VM_pay_something) $reply[$l_id] = $_fmtName;
	}
	ksort($reply);
	break;
      default:
	b_debug::internalError("??? pay='$r[pay]' is rather unexpected...",$r);
      }
      break;
      
    case 'pay_room':
    case 'pay_room_for_applicant':
      switch($get_pay = bForm_vm_Lease::get_lease_options($args,'get_pay')){
      case VM_default_lease_pay:
	$reply = ($mode == 'pay_room_for_applicant'
		  ? 'the cost will be billed to '.myOrg_name
		  : ''); 
	break;
      case VM_pay_nothing:
	$reply = ($mode=='pay_room_for_applicant'
		  ? 'the cost will be billed to you'
		  : myOrg_nameS." pays ".VM_pay_nothing);
	break;
      default:
	$reply = ($mode=='pay_room_for_applicant'
		  ? myOrg_nameS." pays only for ".join('+',array_values($get_pay)).", the cost for other tenant(s) will be billed to you"
		  : myOrg_nameS." pays for ".count($get_pay));
      }
      break;

    case 'pay_menu':
      // Menu selector
      $reply = array();
      $RW = ($data === 'RO' ? False : True);
      //      $RW = True;
      
      $cotenants = bForm_vm_Room::_mightBeSharable($args)
	? VM_cotenants()->get_cotenants($args['lease_id'],True,'info')
	: array();
      if ($RW){
	// RW mode
	b_debug::_debug("n_cotenants = ".count($cotenants).", a_capacity=".$args['a_capacity'],cnf_dev);
	$reply = array('*select_name*'     => 'who pays accommodation',
		       VM_pay_nothing => myOrg_nameS.' pays '.VM_pay_nothing);
	if ($args['a_capacity'] > 1){
	  if (empty($cotenants)){
	    $reply[$args['lease_id']] = myOrg_nameS.' pays only for '.bForm_Avatar::_fmtName('Lf',$args);
	  }else{
	    $leases[$args['lease_id']] = bForm_Avatar::_fmtName('Lf',$args);
	    foreach($cotenants as $l_id=>$_fmtName) $leases[$l_id] = $_fmtName;
	    while (!empty($leases)) {
	      ksort($leases);
	      if (count($leases) < $args['a_capacity']) $reply[join(',',array_keys($leases))] = myOrg_nameS.' pays for '.join(' + ',array_values($leases));
	      array_pop($leases);
	    }
	  }
	}
	$reply[VM_default_lease_pay] = "Room paid by ".myOrg_nameS;
      }else{
	// RO mode
	$get_pay = self::get_lease_options($args,'get_pay');
	switch($get_pay){
	case VM_pay_nothing:
	  $reply = myOrg_nameS.' pays '.VM_pay_nothing;
	  break;
	case VM_pay_everything:
	  $reply = '';
	  break;
	default:
	  $names = array();
	  foreach($get_pay as $l_id=>$_fmtName) $names[] = $_fmtName;
	  $reply = (empty($names)
		    ? ''
		    : myOrg_nameS.' pays'.(count($get_pay)==1?' only':'').' for '.join(' + ',$names));
	}
      }
      break;
      
    case VM_no_cotenants:
    case VM_room_is_full:
      $reply = isset($lease_option[$mode]);
      break;
      
    default:
      b_debug::internalError("Unexpected mode = \"$mode\"");
    }
    if (!empty($reply)) b_debug::_debug($reply,$verbose);
    if (isset($cacheID)) $cache[$cacheID] = $reply;
    bTiming()->cpu();
    return $reply;
  }

  /*
   *
   */
  public static function shared_room_no_cotenants_wanted($args,$verbose=False){
    $reply = self::get_lease_options($args,VM_no_cotenants);
    b_debug::_debug($reply,$verbose&&$reply);
    return $reply;
  }

  /*
   *
   */
  public static function _room_is_full($args,$verbose=False){
    $reply = (//($args['a_capacity'] == 1 && !empty($args['lease_aid'])) ||
	      self::get_lease_options($args,VM_room_is_full) ||
	      self::shared_room_no_cotenants_wanted($args,False));
    b_debug::_debug($reply,$verbose&&$reply);
    return $reply;
  }

  /*
   * Check all the leases and (re)set flag VM_room_is_full
   */
  public static function update_fullRoomFlag($leases=array()){
    myPear::WARNING_T("Ignoring ".__function__.x('(',join(',',$leases)));    return;
    // Get the list of leases
    if (empty($leases)){
	$q = myPear_db()->qquery("SELECT lease_id,lease_option,a_capacity ".
				 " FROM abs_leases ".
				 " LEFT JOIN abs_rooms ON a_id=lease_aid ".
				 " WHERE a_capacity > 1",True);
	while($r = myPear_db()->next_record($q)) $leases[] = $r['lease_id'];
    }
    
    // Walk thru the leases, set the value
    foreach(array_unique($leases) as $lease_id){
      self::update_fullRoomFlag_exec($lease_id);
    }
  }

  private static function update_fullRoomFlag_exec($lease_id,$r=array()){
    myPear::WARNING("Ignoring ".__function__."($lease_id)"); return;
    if (empty($r)) $r = self::_leaseInfo($lease_id);
    foreach(VM_cotenants()->get_cotenants($lease_id,True) as $lm_id=>$cotenants){
      $n_cotenants = count($cotenants);
      $VM_room_is_full = self::get_lease_options($r,VM_room_is_full);
      if ($n_cotenants == $r['a_capacity']){
	if (!$VM_room_is_full) self::set_lease_options($r,array(VM_room_is_full=>1));
      }elseif($n_cotenants < $r['a_capacity']){
	if ( $VM_room_is_full) self::get_lease_options($r,'reset',VM_room_is_full);
      }else{
	myPear::ERROR(sprintf("a_capacity=%s but %s cotenants",$r['a_capacity'],$n_cotenants));
      }
    }
  }
  
  /*
   * Return the visit for the given lease
   */
  public static function visit($lease_id){
    $q = myPear_db()->query("SELECT lease_vid FROM abs_leases WHERE lease_id = '$lease_id'");
    while($r = myPear_db()->next_record($q)){
      $reply = new bForm_vm_Visit($r['lease_vid']);
    }
    return @$reply;
  }

  /*
   * Sanity, check and adjust the leases start/end
   */
  static function adjust_all($visit,$pivot_leaseID=0,$returnMessage=False){
    if (empty($visit))  return;
    
    $leases = array();
    foreach(self::_get_allLeases($visit) as $lease_id=>$hut_code) $leases[] = new bForm_vm_Lease($lease_id);

    // Find the pivot module and set the start/end values for the neighboring leases
    if ($pivot_leaseID){
      foreach($leases as $n=>$lease){
	if ($pivot_leaseID != $lease->ID) continue;
	
	if (($lease_start = $lease->getValue('lease_start')) && ($n > 0)){
	  $leases[$n-1]->setDate(array('lease_end'=>$lease_start),!$returnMessage);
	}
	
	if (($lease_end = $lease->getValue('lease_end')) && ($n < count($leases)-1)){
	  $leases[$n+1]->setDate(array('lease_start'=>$lease_end),!$returnMessage);
	}
      }
    }
    
    // Now this should be (??) just a sanity check 
    $toReportChanges = False;
    foreach($leases as $n=>$lease){
      $changes = $changesP = $changesN = 0;

      // First lease, the start must be the start of the visit
      if ($n == 0){
	$changes += $lease->setDate(array('lease_start'=>$visit->getValue('v_start',1)),!$returnMessage);
      }
      
      // Last lease, the end must be the end of the visit
      if ($n == count($leases)-1){
	$changes += $lease->setDate(array('lease_end'=>$visit->getValue('v_end',1)),!$returnMessage);
      }

      // "next" lease, synchronize its end_date with the start_date of the next one
      if (($n < count($leases)-1) && ($lease_end = $lease->getValue('lease_end'))){
	$lease_end = max($lease_end , $lease->getValue('lease_start') + 24*3600);
	$changes  += $leases[$n]  ->setDate(array('lease_end'  =>$lease_end),!$returnMessage);
	$changesN += $leases[$n+1]->setDate(array('lease_start'=>$lease_end),!$returnMessage);
      }

      if ($changes > 0 || $changesN > 0) $toReportChanges = True; 
      if (cnf_dev && !$returnMessage){
	if ($changes)  myPear::DEBUG($leases[$n]->  _periodToString("lease-".($n  )));
	if ($changesN) myPear::DEBUG($leases[$n+1]->_periodToString("lease-".($n+1)));
	myPear::MESSAGE($lease->_periodToString("lease-$n ($lease->ID)"));
      }
    }

    $reply = array();
    if ($toReportChanges){
      foreach($leases as $lease) $reply[] = $lease->_periodToString();
    }
    if ($returnMessage) return $reply;
  }

  /* 
   * Get the lease_id(s) for the given lease(es)
   */
  public static function get_ids($lease=array()){
    $leases = array();
    $q = myPear_db()->query("SELECT lease_id FROM abs_leases WHERE ".b_fmt::joinX(' AND ',$lease,"'")." ORDER BY lease_start");
    while($r = myPear_db()->next_record($q)) $leases[] = $r['lease_id'];
    return $leases;
  }
  
  /*
   * Return the list of leases & hut_code's for the given visit
   */
  public static $_get_allLeases_reset = False;
  public static function _get_allLeases($visit){
    static $cache = array(), $cache_count = array();

    if (self::$_get_allLeases_reset){
      b_debug::_debug('RESET',True);
      $cache = $cache_count = array();
      self::$_get_allLeases_reset = False;
    }

    if (is_object($visit)) $v_id = $visit->ID;
    else                   $v_id = $visit;

    b_debug::_debug("call #".(int)@$cache_count[$v_id]++,cnf_dev);
    if (isset($cache[$v_id])){
      return $cache[$v_id];
    }else{    
      $leases = array();
      $q = myPear_db()->query("SELECT lease_id,hut_code,a_id,a_name FROM abs_leases ". 
			      " LEFT JOIN abs_rooms ON a_id = lease_aid ".
			      " LEFT JOIN abs_huts ON hut_id = a_hutid ".
			      " WHERE lease_vid = '$v_id' ORDER BY lease_start");
      while($r = myPear_db()->next_record($q)){
	if (!is_numeric($r['lease_id'])) b_debug::internalError('?????????????? '.__method__,$r);
	$leases[$r['lease_id']] = self::_hut_code($r);
	// Sanity. Huge performance overhead...
	//      if (cnf_dev && (($l=@myPear::getClassFromID($r['lease_id']),True,True)){
	//     	myPear::WARNING_T("Bogus lease $l($r[lease_id]), not 'bForm_vm_Lease'");
	//      }
      }
      
      // Create an empty lease on the fly is there is no-one
      if (empty($leases) && is_object($visit) && ($visit->getValue('v_status',True) == STATUS_YES)){
	$lease_id = self::create_firstLease(0,$visit);
	if (!empty($lease_id)){
	  $l = new bForm_vm_Lease($lease_id);
	  $leases[$l->ID] = $l->hut_code();
	}
      }
      $cache[$v_id] = $leases;
      b_debug::_debug($leases,cnf_dev);
    }
    return $leases;
  }
  
  /*
   * The lease object is expected to exists
   */
  private static function create_firstLease($lease_id,$visit){

    // No leases for the rejected visits
    if ($visit->visitOutOfScope() || $visit->isDenied() || $visit->deletingBooking) return False;

    $q = myPear_db()->query("SELECT lease_vid FROM abs_leases WHERE lease_vid='$visit->ID'");
    if (!myPear_db()->num_rows($q)){
      if (!(int)$lease_id)  $lease_id = bMaster()->reserve_masterCounter();
      b_debug::_debug($visit->name(),cnf_show_db||cnf_dev);
      myPear_db()->query("INSERT INTO abs_leases SET ".
			 join(',',array('lease_start='. b_time::check_in ($visit->getValue('v_start',1)),
					'lease_end='  . b_time::check_out($visit->getValue('v_end',1)),
					'lease_vid='  . $visit->ID,
					'lease_id='   . $lease_id)));
    }
    return $lease_id;
  } 


  /*
   * Return the lease name without instantiating the Lease class
   */
  public static function _queryLease($lease_ids,$select='*',$leftJoin=(" LEFT JOIN abs_visits     ON lease_vid = v_id ". 
								       " LEFT JOIN zzz_avatars    ON v_avid = av_id ")){ 
    bTiming()->cpu(__function__);
    if (!is_array($lease_ids)) $lease_ids = array($lease_ids);
    foreach($lease_ids as $lease_id){
      if (empty($lease_id) || !is_numeric($lease_id)) b_debug::internalError('??? lease_id == '.var_export($lease_id,True),$lease_ids);
      $where[] = "lease_id = $lease_id";
    }
    $reply = myPear_db()->qquery("SELECT $select FROM abs_leases ".
				 " LEFT JOIN abs_rooms ON lease_aid = a_id ". 
				 " LEFT JOIN abs_huts ON a_hutid = hut_id ". 
				 $leftJoin.
				 " WHERE " . join(' OR ',$where),cnf_dev);
    bTiming()->cpu();
    return $reply;
  }
  
  public static function _leaseInfo($lease_id,$name_only=False,$message=''){
    bTiming()->cpu(__function__);
    $reply = array();

    if (is_array($name_only)){
      $select = array_unique(array_merge(array('av_lastname','av_firstname'),$name_only)); sort ($select);
      $name_only = False;
    }elseif($name_only){
      $select = array('av_firstname','av_lastname');
    }else{
      $select = array('*');
    }
    sort($select);

    static $cache=array(), $case = array();
    $cache_id = serialize($select).$lease_id;
    b_debug::_debug("call #".(int)(@$case[$cache_id]++));
    
    if (empty($cache[$cache_id])){    
      if (empty($lease_id) || !is_numeric($lease_id)) b_debug::internalError('??? lease_id == '.var_export($lease_id,True));
      $q = self::_queryLease($lease_id,join(',',$select));
      while($r = myPear_db()->next_record($q)) $reply = $r;
      if ($name_only){
	$reply =  bForm_Avatar::_fmtName('Lf',$reply);
	b_debug::_debug("$reply=$message",(cnf_dev && !empty($message)));
      }else{
	$reply['lease_name'] =  bForm_Avatar::_fmtName('Lf',$reply);
      }
      $cache[$cache_id] = $reply;
      b_debug::_debug($reply);
    }else{
      $reply = $cache[$cache_id];
    }
    bTiming()->cpu();
    return $reply;
  }

  /*
   * Helper
   */
  public static function _set_who_pays($lease_id,$pay_option){
    b_debug::_debug('',cnf_dev);
    $cotenants=VM_cotenants()->get_cotenants($lease_id,True,'info');
    if (in_array($pay_option,array(VM_default_lease_pay,VM_pay_nothing))){
      self::set_lease_options($lease_id,array('pay'=>$pay_option));
      foreach($cotenants as $l_id=>$_fmtName){
	self::set_lease_options($l_id,array('pay'=>$pay_option));
      }
    }else{
      $paid_for = explode(',',$pay_option); 
      // if (count($paid_for) >= $a_capacity) self::_set_who_pays($lease_id, VM_default_lease_pay);
      foreach($paid_for as $l_id)                                   self::set_lease_options($l_id,array('pay'=>VM_pay_something));
      foreach(array_diff(array_keys($cotenants),$paid_for) as $l_id)self::set_lease_options($l_id,array('pay'=>VM_pay_nothing));
    }
  }
  
  /*
   *
   */
  function __construct($id,$visit=Null){
    parent::__construct($id);
    if ($visit instanceof bForm_vm_Visit){
      $this->hookVisit($visit);
    }
    $this->isEmbedded(); // call it here to prevent redefining the MODE by the "isEmbedded" function

    //
    // apply the accommodation if requested
    if (@$_POST['_post_bForm_ID_once'] == $this->ID){

      if (isset($_POST['_virt_pay'])){
	// assign "who pays for the accommodation"
	self::_set_who_pays($this->ID,$_POST['_virt_pay']);
      }
      
      if (isset($_POST['lease_aid'])){
	// assign apartment to the lease
	$lease_aid = $_POST['lease_aid'];
	$this->hookRoom($lease_aid,$verbose=True);
	
	// assign cotenant(s) to the lease
	if (is_numeric($lease_aid) && ($a = myPear::getInstance_new('bForm_vm_Room',$lease_aid,'strict'))){
	  $cotenants = array();
	  foreach(preg_grep('/cotenant\d+$/',array_keys($_POST)) as $k){
	    if ($_POST[$k] === VM_no_cotenants){
	      VM_cotenants()->set_cotenants(array(VM_no_cotenants,$this->ID));
	    }elseif(!empty($_POST[$k])){
	      foreach(explode(',',$_POST[$k]) as $l_id){
		if (empty($cotenants))	  $cotenants = array($this->ID);
		$cotenants[] = $l_id;
	      }
	    }
	    unset($_POST[$k]);
	    unset($_REQUEST[$k]);
	  }
	  VM_cotenants()->set_cotenants($cotenants);
	}
      }
    }
  }

function __clean_cache(){}

  /*
   * Call the dedicated function  change the accommodation 
   */
  private $updateDB_preflight_done = 0;
  function updateDB_preflight(&$updates=array()){
    if (!$this->updateDB_preflight_done++){
      if (!empty($updates['lease_aid'])) $this->hookRoom($updates['lease_aid'],True);
    }
    return parent::updateDB_preflight();
  }

  /*
   *
   */
  function updateDB($data=array()){
    if (isset($data['lease_start'])) $data['lease_start'] = b_time::check_in ($data['lease_start']);
    if (isset($data['lease_end']))   $data['lease_end']   = b_time::check_out($data['lease_end']);
    parent::updateDB($data);
  }

  /*
   *
   */
  function getValue($name,$asIs=True,$isMBE=True){

    // may be the value is in the cache already
    if (($value = $this->getValue_fromCache($name,$asIs,True)) !== Null) return $value;
    
    // not in the cache, so look for it
    switch($name){
      // The lease can't outside the visit
    case 'lease_start': 
    case 'lease_end': 
      if ($p_value = parent::getValue($name,True,True)){
	$this->hookVisit();
	$v_start = $this->parentForm->getValue('v_start',1,1);
	$v_end   = $this->parentForm->getValue('v_end',  1,1);
	if ($name == 'lease_start') $value = b_time::check_in (max($p_value,$v_start));
	else                        $value = b_time::check_out(min($p_value,$v_end));
	$this->dbg($value);
      }
      break;
      
    default:   
      return parent::getValue($name,$asIs,True);
    }
    
    // We got the value. Now check how good is it and store in the cache
    return $this->getValue_toCache($name,$value);
  }

  /*
   *
   */
  function readDefRecord() {
    $this->formDB = array();
    $q = myPear_db()->query("SELECT ".
			    "lease_id,lease_start,lease_end,lease_vid,lease_aid,lease_option,".
			    "v_id,v_eid,v_status,v_start,v_end,v_group,".
			    "a_id,a_name,a_capacity,a_quantity,a_price,".
			    "hut_id,hut_code,".
			    "av_id,av_lastname,av_firstname ".
			    " FROM $this->TABLE ".
			    " LEFT JOIN abs_rooms  ON a_id=lease_aid ".
			    " LEFT JOIN abs_visits ON v_id=lease_vid ".
			    " LEFT JOIN abs_huts   ON a_hutid=hut_id ".
			    " LEFT JOIN zzz_avatars   ON av_id=v_avid ".
			    " WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q))   $this->formDB = $r;
  }

  /*
   * Get the apartment type
   */
  function hut_code(){
    return self::_hut_code(array('a_id'    => $this->getValue('a_id',True,True),
				 'a_name'  => $this->getValue('a_name',True,True),
				 'hut_code'=> $this->getValue('hut_code',True,True),
				 ));
  }
  
  /*
   * Hut code for the "internal accommodation types" is 
   * defined by the apartment "code" field (own accommodation, break in the visit, etc.)
   */
  static function _hut_code($buffer){
    $reply = Null;
    if (@$buffer['a_id']){
      $hut_code = @$buffer['hut_code'];
      if ($hut_code === HUT_BI){
	if    (@$buffer['a_name'] === LODGING_OA_TXT) $reply = LODGING_OA;
	elseif(@$buffer['a_name'] === LODGING_BR_TXT) $reply = LODGING_BR;
	elseif(@$buffer['a_name'] === LODGING_RM_TXT) $reply = LODGING_RM;
	else {
	  b_debug::internalError("Unknown Built-in object \"".@$buffer['a_name']."\"",$buffer);
	}
      }elseif(!empty($hut_code)){
	$reply = $hut_code;
      }
    }
    return $reply;
  }

  /*
   *
   */
  function hookVisit($visit=Null){
    if (!($this->parentForm instanceof bForm_vm_Visit)){
      $this->parentForm = ($visit instanceof bForm_vm_Visit
			   ? $visit
			   : myPear::getInstance_new('bForm_vm_Visit',$this->getValue('lease_vid',1),'strict'));
      /*
      if (!$this->embedded && is_object($visit)) $this->isEmbedded($visit);
      if (!$this->embedded)   $this->isEmbedded(myPear::getInstance_new('bForm_vm_Visit',$this->getValue('lease_vid',1)),'strict');
      if (!$this->embedded)   b_debug::internalError("visit is not set",$this);
      */      
      // Create a default lease if not yet done
      if ((int)$this->parentForm->ID && !self::$isReady){
	self::create_firstLease($this->ID,$this->parentForm);
	self::$isReady = True;
      }
    }
    return ($this->parentForm instanceof bForm_vm_Visit);
  }
  
  function defineVariables() {
    // Note, there is a second call to 'defineVariable' below, hence no check on number of calls
    $this->defineVariable(array('Accommodation'=>'lease_aid',
				' '            =>'l_period',
				'From'         =>'lease_start',
				'To'           =>'lease_end'));
    
    $this->defineTypes(array('isVITAL_ifADMIN'=> array('_cotenant'=> (int)$this->ID,
						       'lease_aid'=> (int)$this->ID),
			     'isVITAL'        => array('lease_aid'=> (int)$this->ID),
			     'isRO'           => array('l_period'),
			     'isMBE'          => array('lease_start',
						       'lease_end'),
			     ));
  }

  /*
   *
   */
  function accommodationCost(){
    $this->readDefRecord();
    $receipt = ($ok=($a_id = (int)$this->getValue('a_id')))
      ? bForm_vm_Room::_getReceipt($this->formDB,$this->nDays())
      : 0;
    $this->dbg($receipt);
    return array($receipt,$ok);
  }

  /*
   *
   */
  private function nDays(){
    return (int)((b_time::_($this->getValue('lease_end')) - 
		  b_time::_($this->getValue('lease_start')))/86400);
  }
  
  /*
   *
   */
  protected function formBlock_isVisible($block_ID,$fields=array()){
    if ($this->embedded){
      $this->hookVisit();
      return $this->parentForm->formBlock_isVisible($block_ID,$fields);
    }else{
      return True;
    }
  }

  function formBlock_isWritable($block_ID){
    $this->hookVisit();
    if (strpos($block_ID,VM_visit_lease) !== False) $block_ID = VM_visit_lease;
    $reply = $this->parentForm->formBlock_isWritable($block_ID);
    $this->dbg($reply);
    return $reply; 
  }

  /*
   *
   */
  protected function formBlock_setModeRW(){
    if (!VM_tooLateForAccommodation($this) && 
	(@$_GET['block_once'] == $this->block_ID)){
      $this->MODE = 'RW';
      $this->dbg("'block_once', set MODE=RW",True);
    }
  }

  /*
   *
   */
  function getForm(){

    // Process "delete lease" which comes from the "visit dialog"
    if ($this->myPost_bForm() && @$_POST['lease_aid'] === LODGING_RM){
      $this->hookRoom(LODGING_RM);
      return;
    }

    if ($this->parentForm->isApproved()){
      b_debug::_debug(starting);
      // check the start/end of the lease 
      if ($this->myPost_bForm()){
	$this->hookVisit();
	if (isset($_POST['lease_start']) || isset($_POST['lease_end'])){
	  $this->setDate(array('lease_start'=>$this->getValue('lease_start'),
			       'lease_end'  =>$this->getValue('lease_end')));
	  self::adjust_all($this->parentForm,$this->ID);
	}
      }
      $this->isRO['lease_start'] = ($this->getValue('lease_start') == b_time::check_in ($this->parentForm->getValue('v_start')));
      $this->isRO['lease_end']   = ($this->getValue('lease_end')   == b_time::check_out($this->parentForm->getValue('v_end')));
      
      if ($this->embedded){ // call it here to prevent redefining the MODE by the "isEmbedded" function
	$this->hookVisit();
      }
      
      
      foreach(array('lease_id','lease_aid','l_period','lease_start','lease_end') as $i){
	bForm::nameCounter('resetCounter',$i);
      }
      
      $this->getFormOpen();
      
      $block_header = array('Accommodation');
      
      // Optionally add button "split accommodation" & "remove splited accommodation"
      if ($this->isWritable() && $this->isRO('lease_aid')){
	$nLeases = count(self::_get_allLeases($this->getValue('lease_vid')));
	if ($nLeases  > 0 && 
	    $this->maySplit($this->hut_code()) && 
	    $this->hasAccommodation()){
	  $block_header[] = b_btn::submit_icon('i-arrow_divide', 
					       btn_splitLease,
					       b_url::same("?action_once=".btn_splitLease.
							   "&function=vm_edit_accommodation&resetcache_once=1&lease_id=$this->ID&v_id=".
							   $this->parentForm->ID),
					       $confirm=True,
					       '');
	  
	}
	if ($nLeases > 1){
	  $block_header[] = b_btn::submit_icon('i-drop',
					       btn_deleteBooking,
					       b_url::same("?action_once=".btn_deleteBooking.
							 "&function=vm_edit_accommodation&resetcache_once=1&lease_id=$this->ID&v_id=".
							   $this->parentForm->ID),
					       $confirm=True,
					       '');
	}
      }
      
      // Cancel the accommodation menu if the event is in the past 
      $dialog = array('lease_id' => array('textHidden'));
      if ((time() - AUTO_EXPIRATION*86400) > $this->parentForm->getValue('v_end',1)) $this->isRO['lease_aid'] = !superUser_here;
      
      $a_id = $this->getValue('lease_aid');
      if ($this->isRO('lease_aid') && (!$this->hasAccommodation() || !$a_id)){ 
	$this->getValue_reset('lease_aid',b_fmt::redText('No accommodation'));
	$dialog['lease_aid'] = array('textField');
      }else{
	$dialog['lease_aid'] = array('selectMenu',
				     $this->freeRooms(),
				     "onchange='submit()'");
      }
      
      if ($this->hasAccommodation()){
	// Accompaning persons
	$this->parentForm->dialog_accompaning($dialog,$this);

	// Cotenants
	if (bForm_vm_Room::_mightBeSharable($this->formDB)){
	  // Force the dialog in the RO mode
	  if (($this->MODE=='RO') && !VM_cotenants()->get_cotenants($this->ID) && !self::get_lease_options($this->formDB,VM_no_cotenants)){
	    $this->MODE = 'RW';
	    $this->dbg("Force mode ".$this->MODE,cnf_dev);
	  }
	  if (VM_tooLateForAccommodation($this)){
	    $this->MODE = 'RO';
	    $this->dbg("Too late, force mode ".$this->MODE,cnf_dev);
	  }
	  // Select a cotenant if there are candidates
	  $cotenant = self::_select_cotenant('_cotenant',$this,($this->MODE === 'RW'));
	  if (!empty($cotenant)) $dialog[b_rawData.'1'] = array('Cotenant',$cotenant);
	}
	
	// Who pays for the accommodation?
	$this->dbg('calling _select_who_pays',cnf_dev);
	$selector = self::_select_who_pays('_virt_pay',$this);
	if (!empty($selector)) $dialog[b_rawData.'2'] = array(' ',$selector);
	$this->dbg('exit _select_who_pays',cnf_dev);
	
	// Visit period
	if ($this->isRO('lease_start') && $this->isRO('lease_end')){
	  if (($lease_start=$this->getValue('lease_start')) && ($lease_end=$this->getValue('lease_end'))){
	    $this->defineVariable(b_time::period($lease_start,$lease_end),'lease_aid');
	  }
	}else{
	  $minmax = array($this->parentForm->getValue('v_start'),
			  $this->parentForm->getValue('v_end'));
	  $dialog['lease_start'] = array('datePickerField',$minmax);
	  $dialog['lease_end']   = array('datePickerField',$minmax);
	}
	
      }
      
      // Accommodation wish
      if (($this->MODE == 'RW' || !$this->hasAccommodation()) &&
	  ($_virt_wish=$this->parentForm->getValue('v_acc_wish',True)) &&
	  $this->parentForm->isVisitType_program()){
	$this->defineVariable(array('_virt_wish'=>b_fmt::redText('Accommodation wish','greyText')),True);
	$this->isRO['_virt_wish'] = True;
	$this->formDB['_virt_wish'] = b_fmt::redText($_virt_wish,'greyText');
	$dialog['_virt_wish'] = array('textField');
      }
    
      // print the block
      //      b_debug::print_r($dialog,'dialog');
      $this->formBlock(VM_visit_lease.$this->ID,
		       $block_header,
		       $dialog,
		       array('l'=>b_url::same("?function=vm_edit_accommodation&resetcache_once=1&lease_id=$this->ID&v_id=".
					      $this->parentForm->ID),
			     'i'=>'i-hotel',
			     'd'=>btn_edit_accommodation));
      b_debug::_debug(exiting);
    }
  }
  
  /*
   * May the admin split accommodation?
   */
  private function maySplit($hut_code){
    $reply = ($this->nDays() > 1) && $this->mayBook($hut_code);
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Get "rooms" for the build-in options
   */
  static function _get_BI($return_a_code=False){
    $reply = array();
    $q = myPear_db()->query("SELECT a_id,a_name,hut_code FROM abs_huts LEFT JOIN abs_rooms ON a_hutid = hut_id WHERE hut_code = '".HUT_BI."'");
    while($r = myPear_db()->next_record($q)){
      if ($return_a_code){
        locateAndInclude('bForm_vm_Lease');
        $reply[$r['a_id']] = self::_hut_code($r);
      }else{
        $reply[$r['a_id']] = $r['a_name'];
      }
    }
    return $reply;
  }

  /*
   *
   */
  private static function _mayBook($hut_code,$visit_class){
    $this->hookVisit();
    
    if (in_array($hut_code,array_merge(array(HUT_BI),array_values(bForm_vm_Lease::_get_BI())))){
      $reply = True;
    }elseif(!($reply = VM_manager_here)){
      switch($hut_code){
      case HUT_AH:      
	$reply = VM::hasRightTo('book_ah',$visit_class);
      case HUT_A:
      case HUT_H:   
	$reply = $reply || VM::hasRightTo('book',$visit_class);
      }
    }
    b_debug::_debug($reply);
    return $reply;
  }
  
  /*
   * Which type of aparts may book the user
   */
  private function mayBook($a_type=Null){
    return ($this->isWritable() || ((is_object($v=$this->parentForm)) && $v->isWritable()));
    /*
    $this->hookVisit();
    if (is_object($v=$this->parentForm)) return (self::_mayBook($a_type,$v) || $v->isOwner());
    else                                 return False;
    */
  }
  
  /*
   * Check the accommodation, optionally skipping the "own accommodation"
   */
  function hasAccommodation($exclude_OA=False){
    $reply = $lease_aid = (int)$this->getValue('lease_aid',True);
    if ($exclude_OA && VM_self_accommodated(array('lease_aid'=>$lease_aid))) $reply = False;
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * Set start / end dates for the lease
   */
  function setDate($dates,$verbose=True){
    
    $updates = array();
    foreach($dates as $key=>$date){
      if (empty($date)) continue;
      switch($key){
      case 'lease_start':$date_asIs = $this->formDB_modified[$key] = b_time::check_in ($date);   break;
      case 'lease_end':  $date_asIs = $this->formDB_modified[$key] = b_time::check_out($date);   break;
      }
      if (@$this->formDB[$key] != ($date_corrected=$this->getValue($key))){
	$updates[$key] = $date_corrected;
	$this->dbg("$key = ".date("Y-m-d H:i",$date_asIs).' -> '.date("Y-m-d H:i",$date_corrected));
      }
    }
    if (!empty($updates)){  
      $this->isVITAL = array();
      $this->updateDB($updates);
      if ($verbose) myPear::MESSAGE_add($this->_periodToString("Set lease ".(cnf_dev?$this->ID:'')." for"));
    }
    return count($updates);
  }

  /*
   *
   */  
  function clean_cotenants(){
    self::get_lease_options($this->ID,'reset');
    foreach(VM_cotenants()->get_cotenants($this->ID,True) as $lm_id=>$leases){
      $this->dbg("lm_id=$lm_id leases=".join(', ',$leases) ,cnf_dev);
      foreach($leases as $lease_id){
	$_leaseInfo = self::_leaseInfo($lease_id,array('lease_id','lease_option'));
	myPear::MESSAGE("Removing cotenant ".bForm_Avatar::_fmtName('Lf',$_leaseInfo));
	self::get_lease_options($lease_id,'reset');
      }
      VM_cotenants()->deleteMember($lm_id);
    }
  }

  /*
   * Hook room to the lease
   */
  function hookRoom($lease_aid,$verbose=True){
    if (cnf_dev) $verbose = True;

    if ($lease_aid === LODGING_RM){
      b_debug::_debug('REMOVE');

      // clean the cotenants list (if any) since the apartment is changed  
      $this->clean_cotenants();
      if ($verbose) myPear::MESSAGE_add($this->_periodToString('Remove accommodation for'));
      $this->getValue_reset('lease_aid',0);
      $this->hookVisit();
      self::get_lease_options($this->ID,'reset');
    }elseif (is_numeric($lease_aid) && (($old_aid=@$this->formDB['lease_aid']) != $lease_aid)){
      // sanity
      $room = myPear::getInstance_new('bForm_vm_Room',$lease_aid,'strict');
      if (!($room instanceof bForm_vm_Room)) {
	b_debug::internalError("$lease_aid is not bForm_vm_Room",$this);
      }
      $this->hookVisit();

      // clean the cotenants list (if any) since the apartment is changed  
      $this->clean_cotenants();

      if ($verbose) myPear::MESSAGE_add($this->_periodToString(($old_aid?'Change':'Set').' accommodation for',$old_aid));
      $this->getValue_reset_forced('lease_aid',$lease_aid);
      $this->readDefRecord();

      $this->v_accompaning();

      // Update the number of rooms...
      if(is_object(VM::$e)) self::_room_usage($lease_aid);
    }
  }
  
  /*
   * Check the accompaning persons, optionally set the accommodation "full" 
   */
  public function v_accompaning(){
    $this->hookVisit();
    return self::_v_accompaning(array_merge($this->formDB,$this->parentForm->formDB));
  }
  public static function _v_accompaning($rec){
    if ($reply=(!empty($rec['lease_aid']) && ($n = $rec['v_accompaning']) && ($n+1 >= $rec['a_capacity']))){
      if (self::get_lease_options($rec,'get_pay')===VM_default_lease_pay){
	myPear::WARNING_T("Impose Nordita pays for one ".($n+1).'/'.$rec['a_capacity']);
	self::set_lease_options($rec,array('pay'=>VM_pay_something));
      }
      if (!self::get_lease_options($rec,VM_room_is_full)){
	myPear::WARNING(__function__."($rec[lease_id]): impose VM_room_is_full ".($n+1).'/'.$rec['a_capacity']);
	self::set_lease_options($rec,array(VM_room_is_full=>1));
      }
    }
    b_debug::_debug(var_export($reply,True));
    return $reply;
  }

  /*
   *
   */
  function isReadable(){
    return bAuth::authenticated();
  }

  private $isOwner = Null;
  function isOwner(){
    $this->hookVisit();
    if ($this->isOwner === Null) $this->isOwner = (bool)b_cnf::check_conditions(array(True=>array('v_own'=> (is_object($this->parentForm) && !$this->parentForm->isOwner())),
										      'default' => parent::isOwner()),
										$this);
    return $this->isOwner;
  }

  public $not_isWritable_point = False;
  private $isWritable = Null;
  function isWritable(){
    bTiming()->cpu(__function__);
    $this->hookVisit();
    if ($this->isWritable === Null) $this->isWritable = (bool)!$this->not_isWritable_point && $this->parentForm->isWritable() && $this->parentForm->isApproved();
    bTiming()->cpu();
    return $this->isWritable;
  }

  /*
   *
   */
  function split($date=Null){
    $this->dbg(starting);
    $this->hookVisit(); 
    myPear::MESSAGE($this->_periodToString('Split lease'));
    if (!$date) $date = (int)((($e=$this->getValue('lease_end'))+($s=$this->getValue('lease_start')))/2);
    $this->setDate(array('lease_end'  => $date),True);

    $lnew = new bForm_vm_Lease('empty');
    $lnew->updateDB(array('lease_start'=> b_time::_($date,CONST_TIME_CI),
			  'lease_end'  => $e,
			  'lease_aid'  => 0,
			  'lease_vid'  => $this->parentForm->ID,
			  //'lease_id'   => (($id=(int)$this->ID) ? $id : bMaster()->reserve_masterCounter())));
			  'lease_id'   => bMaster()->reserve_masterCounter()));
    $this->dbg("New lease ".$lnew->__toString(),cnf_dev);
    
    $this->parentForm->get_allLeases(True);

    $this->dbg(join('<br/>',array($this->_periodToString(),
				  $lnew->_periodToString())),cnf_dev);
    
    // Adjust the new values
    self::adjust_all($this->parentForm,$lnew->ID);

    $this->dbg(exiting);
    return $lnew->ID;
  }
  
  /*
   * delete lease
   */
  function delete(){
    $this->dbg("Lease ".$this->ID." ".$this->getValue('a_name'),cnf_dev);
    $this->clean_cotenants();
    $this->formDB = array();
    $this->__clean_cache();
    myPear_db()->query("DELETE FROM abs_leases WHERE lease_id = '".$this->ID."'");
  }

  /*
   *
   */
  public function freeRooms(){

    $rec = array('lease_id'   =>$this->ID,
		 'lease_start'=>$this->getValue('lease_start'),
		 'lease_end'  =>$this->getValue('lease_end'),
		 'v_id'       =>$this->parentForm->ID,
		 'v_eid'      =>@$this->parentForm->e->ID);
    if ($a_id = $this->getValue('lease_aid')){
      $room = myPear::getInstance_new('bForm_vm_Room',$a_id,'strict');
      $rec['lease_aid'] = $a_id;
      $rec['hut_name']  = $room->hut->getValue('hut_name');
      $rec['hut_code']  = $room->hut->getValue('hut_code');
      $rec['a_name']    = $room->getValue('a_name');
      $rec['a_capacity']= $room->getValue('a_capacity');
    }
    return self::_freeRooms((is_object($e=$this->parentForm->e)
			     ? VM_accommodationOptions($e)
			     : Null),
			    $rec,
			    $this->hasAccommodation(False));
  }


  /*
   * Room usage by a program
   */
  public static function _room_usage($a_id,$update_counters=False){
    bTiming()->cpu(__function__);
    list($start,$end) = array(b_time::check_in (VM::$e->getValue('e_start')),
			      b_time::check_out(VM::$e->getValue('e_end')));
    
    $q = myPear_db()->qquery("SELECT lease_id,lease_start,lease_end,hut_id,a_name,a_capacity,hut_code,hut_name FROM abs_leases".
			     " LEFT JOIN abs_rooms  ON lease_aid = a_id ".
			     " LEFT JOIN abs_visits ON lease_vid = v_id ".
			     " LEFT JOIN abs_huts ON hut_id = a_hutid ".
			     " WHERE ".
			     join (' AND ',
				   array("lease_aid = $a_id ",
					 "lease_start <= $end ",
					 "lease_end   >= $start",
					 (accommodate_cotenants_from_different_events ? 1 : "v_eid = ".VM::$e->ID))),
			     cnf_dev);
    $n_tenants = 0;
    $entries = $matrix = array();
    while ($r = myPear_db()->next_record($q)){
      $hut_id = $r['hut_id'];
      $room_name = bForm_vm_Room::_a_name($r,True);
      $n_tenants++;
      $entry = array('day1' => b_time::check_in ($r['lease_start']),
		     'day9' => b_time::check_out($r['lease_end']),
		     'name' => $r['lease_id'],
		     );

      $cotenants = VM_cotenants()->get_cotenants($r['lease_id'],True,'names');
      if ($cotenants){
	$entry['name'] = join(' + ',$cotenants);
	$entries[join(',',$cotenants)] = $entry;
      }else{
	$entries[] = $entry;
      }
    }

    // Pack the leases by bCover    
    locateAndInclude('bCover');
    $solver = new bCover($entries,'day1','day9','n');
    $solution_array = $solver->solve();
    foreach($solution_array as $n=>$solution) $matrix[] = $solution;
    //    b_debug::print_r($matrix,'solution','nosort');

    // Update the counters 
    $n_rooms = count(array_keys($matrix));
    if (0)    if (!empty($hut_id) && VM::_MENU()->context() == 'event'){
	$a_quantity = VM_accommodationOptions(VM::$e)->get_a_value($a_id,$hut_id,'a_quantity');
	if ($n_rooms == $a_quantity){
	  myPear::WARNING("The last <em>$room_name</em> is booked");
	}elseif ($n_rooms > $a_quantity){
	  if ($update_counters){
	    VM_accommodationOptions(VM::$e)->set_a_value($n_rooms,$a_id,$hut_id,'a_quantity');
	    myPear::WARNING("Number of available <em>$room_name</em> is set $a_quantity ==> $n_rooms");
	  }else{
	    myPear::ERROR("Number of available <em>$room_name</em> is too small - used/available = $n_rooms/".max(0,$a_quantity).".<br>".
			  "Please update the accommodation options");
	  }
	}
      }

    // Return the reply
    b_debug::_debug("$n_tenants tenants in $n_rooms");
    bTiming()->cpu();
    return array((int)$n_tenants,(int)$n_rooms);
  }


  /*
   * Check the pool and get a list of available rooms in the hut for the given dates & event 
   */
  public static function _freeRooms($accommodationOptions,$rec,$hasAccommodation=False,$hut_code=Null){

    // Shortcuts
    if (isset($rec['v_status']) && ($rec['v_status'] !== STATUS_YES)){
      b_debug::_debug("Skip, ".$rec['v_status'],True);
      return array();
    }
    /*
    if (isset($rec['lease_aid']) && VM_self_accommodated($rec)){
      b_debug::_debug("Skip, ".VM::$OA->name(),True);
      $reply[VM::$OA->ID] = VM::$OA->name();
      return $reply;
    }
    */
    bTiming()->cpu(__function__);
    list($start,$end) = array(b_time::check_in($rec['lease_start']),
			      b_time::check_out($rec['lease_end']));
    b_debug::_debug(b_time::period($start,$end),cnf_dev);
    
    $huts = array();
    if (!empty($hut_code)){
      $where = array("a_end >= $start ",
		     "( a_start <= $end OR a_start = 0 OR a_start IS NULL) ",
		     (is_numeric($hut_code)
		      ? "hut_code  = '$hut_code'"
		      : "hut_code != '".HUT_BI."'"),
		     (empty($accommodationOptions)
		      ? "1"
		      : ' hut_id != '.join(' AND hut_id !=  ',$accommodationOptions))); // excludes
      $huts_select = array();
      $q = myPear_db()->qquery("SELECT hut_id,hut_name FROM abs_huts LEFT JOIN abs_rooms ON a_hutid=hut_id WHERE ".join(' AND ',$where),cnf_dev);
      while ($r = myPear_db()->next_record($q)){
	$huts_select[$r['hut_id']] = $r['hut_name'];
	$huts[$r['hut_name']] = $r['hut_id'];
      }
      if (!is_numeric($hut_code)){
	bTiming()->cpu(__function__);
	b_debug::_debug("reply=".join(', ',array_values($huts_select)),cnf_dev);
	return $huts_select;
      }
      $event_context = False;
      $accommodationOptions = False;
    }else{
      // Chase for the associated event ID, since VM::_MENU()->context() 
      // might give a wrong answer for the search results.
      if (!is_object($accommodationOptions) && ($e_id = @$rec['v_eid'])){
	b_debug::_debug("Recover e_id = $e_id");
	$accommodationOptions = VM_accommodationOptions($e_id);
      }
      $event_context = is_object($accommodationOptions);
      if ($event_context){
	// Use only those "huts" which are selected by the administration, i.e. listed in vm_accommodationOptions
	foreach($accommodationOptions->getMembers() as $x=>$r) $huts[] = $r['lm_key'];
      }else{
	// Check ALL available rooms
	foreach(bForm_vm_Hut::_get_allHuts() as $hut_id=>$hut_name) $huts[] = $hut_id;
      }
    }
    // Sanity
    foreach($huts as $k=>$v){
      if (empty($v)){
	if (cnf_dev) b_debug::internalError("??? empty hut element",$huts);
	else unset($huts[$k]);
      }
    }
    $huts_list = join(', ',array_values($huts));
    
    b_debug::_debug("context ".($event_context ? 'event '.$accommodationOptions->e->name() : 'org'),cnf_dev);
    b_debug::_debug('huts = '.$huts_list,cnf_dev);

    // General time limits
    $where = array((empty($huts_list) ? 1 : "hut_id IN ($huts_list)"),
		   "a_end >= $start",
		   "(a_start <= $end OR a_start=0 OR a_start IS NULL)",
		   "hut_code != '".HUT_BI."'"
		   );
    
    // See all the rooms
    $q = myPear_db()->qquery("SELECT a_id,a_price,a_name,a_quantity,a_capacity,a_quantity,hut_id,hut_code,hut_name ".
			     " FROM abs_rooms ".
			     " LEFT JOIN abs_huts ON a_hutid = hut_id ".
			     " WHERE ".join(' AND ',$where).
			     " ORDER BY hut_name ASC,a_capacity ASC",cnf_dev);
    $all_rooms = $booked_rooms = $available_beds = array();
    while ($r = myPear_db()->next_record($q)){
      if (!in_array($r['hut_id'],array_values($huts))) continue;
      $id = bForm_vm_Room::room_id($r);
      // How many rooms of this type have we for the event?
      $all_rooms[$id] = ($event_context
			 ? $accommodationOptions->get_a_value($r['a_id'],$r['hut_id'],'a_quantity')
			 : $r['a_quantity']);
      $booked_rooms[$id] = array();
      $room_capacities[$id] = 0;
    }      

    //    b_debug::print_r($all_rooms,'all rooms','nosort');

    // Check the booked rooms    
    if ($event_context){
      $q = myPear_db()->qquery(sprintf("SELECT lease_id,lease_start,lease_end,v_eid,v_status,lease_aid,lease_option,".
				       " av_firstname,av_lastname,".
				       " a_id,a_price,a_name,a_quantity,a_capacity,a_quantity,hut_code,hut_name,hut_id ".
				       " FROM abs_leases ".
				       " LEFT JOIN abs_rooms   ON lease_aid = a_id ".
				       " LEFT JOIN abs_huts    ON a_hutid = hut_id ".
				       " LEFT JOIN abs_visits  ON lease_vid = v_id ".
				       " LEFT JOIN zzz_avatars ON v_avid = av_id ". 
				       //" LEFT JOIN abs_events  ON v_eid = e_id ".
				       " WHERE ".join(' AND ',
						      array_merge($where,
								  array("lease_start <= $end ",
									"lease_end   >= $start",
									(ignore_external_leases_when_booking ? "v_eid = ".$accommodationOptions->e->ID : '')))).
				       // " GROUP BY lease_id ".
				       " ORDER BY CONCAT(hut_code,hut_name,a_capacity) ASC"),
			       cnf_dev);
      $bCover_input = array();    
      while ($r = myPear_db()->next_record($q)){
	$id = bForm_vm_Room::room_id($r);
	$cotenants = VM_cotenants()->get_cotenants($r['lease_id'],True,'info','L');
	$entry = array('day1' => b_time::check_in ($r['lease_start']),
		       'day9' => b_time::check_out($r['lease_end']),
		       'name' => $r['lease_id'],
		       );
	if ($cotenants){
	  $entry['name'] = join('+',array_values($cotenants));
	  $bCover_input[$id][join(',',array_keys($cotenants))] = $entry;
	}else{ 
	  $bCover_input[$id][] = $entry;  
	}
	$room_capacities[$id] = $r['a_capacity'];
	if (($r['a_capacity']>1) && 
	    (count($cotenants)+@$r['v_accompaning']<$r['a_capacity']) &&
	    !self::_room_is_full($r)) @$available_beds[$id]++;
      }
      arsort($available_beds);
      if (!empty($available_beds)) b_debug::print_r($available_beds,'available_beds','nosort');

      // Calculate the optimal leases distribution
      locateAndInclude('bCover');
      foreach($bCover_input as $id=>$entries){
	$solver = new bCover($entries,'day1','day9','n');
	$solution_array = $solver->solve();
	//	foreach($solution_array as $n=>$solution) $booked_rooms[$id][] = $solution;
	$booked_rooms[$id] = array_values($solution_array);
      }
    }
    
    // Prepare the answer
    $reply = array();
    if (empty($hut_code)){
      if (empty($rec['v_id'])) b_debug::print_r($rec,'??? empty $rec[v_id]');
      if (empty($rec['v_id'])) b_debug::traceBack('??? empty $rec[v_id]');
      if ($hasAccommodation) $reply[LODGING_RM] = LODGING_RM_TXT;
      foreach(bForm_vm_Lease::_get_BI() as $k=>$v) $reply[$k] = $v;
      if (count(self::_get_allLeases($rec['v_id']) < 2)) unset($reply[VM::$BR->ID]);
      if (!empty($reply)) $reply['separator1'] = '';
    }

    // Populate the reply with non-over-booked rooms
    $n_populated = 0;
    foreach($all_rooms as $id=>$a_quantity){
      list($a_id,$room_name) = explode('#',$id,2);
      $n_booked_rooms = count($booked_rooms[$id]);
      if ($a_quantity < $n_booked_rooms){
	// Disaster, something is wrong
	if ($n_booked_rooms) b_debug::_debug(sprintf("$id over-booked %s/%s !!!",$n_booked_rooms,$a_quantity),array('bold-text',cnf_dev));
      }elseif($a_quantity > $n_booked_rooms){
	b_debug::_debug($id,array('bold-text',cnf_dev));
	$n_populated++;
	$reply[$a_id] = $room_name;
      }elseif($available_beds){
	// All rooms have tenants, check may be there is a free bed...
	if($room_capacities[$id] > @$available_beds[$id]){
	  b_debug::_debug("$id a_capacity=".$room_capacities[$id].", available_beds=".(int)@$available_beds[$id],array('bold-text',cnf_dev));
	  $n_populated++;
	  $reply[$a_id] = $room_name . as_cotenant;
	}
      }
    }
    
    // However, use the available shares if there is no other chance...
    if (empty($n_populated)){
      $msg_overbooking = "WARNING, all the available room are booked.<br/>".
	"You may continue with shared accommodation";
      foreach($available_beds as $id=>$count){
	if (!$count) continue;
	b_debug::_debug("$id a_capacity=".$room_capacities[$id].", available_beds=".$available_beds[$id],cnf_dev);
	$n_populated++;
	list($a_id,$room_name) = explode('#',$id,2);
	$reply[$a_id] = $room_name.as_cotenant;
      }
    }
    if (empty($n_populated)) $msg_overbooking = "WARNING, all the available rooms are booked, including those with shared accommodation.<br>".
			       "You should contact the (hotels) administration in order to increase your quotas and/or reduce the number of participants.";
    if (0)    if (!empty($msg_overbooking)) myPear::ERROR($msg_overbooking);
    
    if (!empty($reply)) $reply['*select_name*'] = 'accommodation';

    // If the reference lease has a room attached, include it in the reply
    if (@$rec['lease_aid'] && empty($reply[$rec['lease_aid']])){
      b_debug::_debug("$id auto-added",array('bold-text',cnf_dev));
      $n_populated++;
      $reply[$rec['lease_aid']] = bForm_vm_Room::_a_name($rec,True);
    }
    
    b_debug::_debug("reply=".join(',&nbsp;',array_values($reply)),cnf_dev);
    bTiming()->cpu();
    return $reply;
  }

  function formOKspecific(){}
  
  function name(){
    if (is_object($o = $this->parentForm) && is_object($o->av)){
      return $o->av->fmtName();
    }else{     
      return parent::name();
    }
  }

  protected function _recordUpdated() {
    $this->_recordCreated();
  }
  
  protected function _recordCreated() {
    if ((int)$this->getValue('lease_aid',True)>0)   myPear_logs()->add('book',$this->ID);
  }

  /*
   * Return human-readable lease period
   */
  function _periodToString($text='&nbsp;&nbsp;&nbsp;&nbsp;',$lease_aid_old=Null){
    if (is_object($this->parentForm)){ 
      $lease_aid = (int)$this->getValue('lease_aid',True);
      $this->parentForm->hookAvatar();
      if ($ap = myPear::getInstance_new('bForm_vm_Room',$lease_aid,array('strict','nocache'))){ 
	if ($lease_aid_old){
	  $ap_old = myPear::getInstance_new('bForm_vm_Room',$lease_aid_old,array('strict','nocache'));
	  $from = '"'.$ap_old->name().'" -> ';
	}else{
	  $from = '';
	}

	$reply = sprintf ("$text %s <em>%s \"%s\" %s</em>",
			  $this->parentForm->av->name(),
			  $from,
			  $ap->name(),
			  b_time::period($this->getValue('lease_start'),$this->getValue('lease_end')));
      }else{ 
	$reply = 'NO_BOOKING';
	$reply = '';
      }
      return $reply;
    }
  }


  /*
   * 
   */
  public static function _select_who_pays($name,$recP,$anchor='',$onlyFromObject=True){

    if (is_object($recP)){
      $rec = $recP->formDB;
      $name_for_select = $name;
      $recP->forceROmode();
      $MODE = $recP->MODE;
    }else{
      // This selection is NOT supposed to be shown in the bHolder context.
      // It works, it is posible, but looks too messy..
      if ($onlyFromObject) return '';
      $rec = $recP;
      $name_for_select = $name . $rec['lease_id'];
      $MODE = 'RO';
    }
    if (VM_self_accommodated($rec)) return '';
    
    // Sanity
    if (empty($rec['lease_aid']) || (@$rec['v_status'] !== STATUS_YES)){
      b_debug::_debug('v_status='.@$rec['v_status'],cnf_dev);
      return '';
    }
    
    $get_pay = self::get_lease_options($rec,'get_pay');
    $options = array();
    if (is_object($recP)){
      VM::$formDummy->MODE = $MODE;
      VM::$formDummy->show_gv=True;
      VM::$formDummy->defineVariable(array($name_for_select=>' '),True);
      VM::$formDummy->getValue_reset($name_for_select,(is_array($get_pay) ? join(',',array_keys($get_pay)):$get_pay));
      b_debug::_debug("getValue(name_for_select) = ".var_export(VM::$formDummy->getValue($name_for_select),True),cnf_dev); 

      $options = self::get_lease_options($rec,'pay_menu',$MODE,cnf_dev);
      b_debug::_debug($options,cnf_dev);
      $hidden_values = join("\n",array("<input type='hidden' name='lease_id' value='".$rec['lease_id']."'/>",
				       "<input type='hidden' name='v_id'     value='".$rec['v_id']."'/>",
				       "<input type='hidden' name='a_id'     value='".$rec['a_id']."'/>"));
      $reply = is_array($options)
	? x("span class='only_online'",
	    x("form action='".b_url::same('?resetcache_once=1',$anchor)."' method='post' enctype='multipart/form-data' name='PAY$rec[lease_id]'",
	      VM::$formDummy->selectMenu_exec($name_for_select,
					      $options,
					      "onchange='submit()'",
					      ($MODE=='RW')).
	      $hidden_values.
	      (is_object($recP)?"<input type='hidden' name='_post_bForm_ID_once' value='".$recP->ID."'/>":"").
	      "<noscript><input type='submit' value='go'/></noscript>"),
	    'POST')
	: (empty($options) 
	   ? '' 
	   : $options . $hidden_values);
      //	   : array($name_for_select =>array('textField'))); // .$hidden_values);
      b_debug::_debug("reply=".var_export($options,True));
    }else{
      $reply = '';
    }
    return $reply;
  }

  /*
   *
   */
  public static function _select_cotenant($name,$recP,$RW,$anchor=''){
   locateAndInclude('bForm_vm_Room');
    
    $rec = (is_object($recP)
	    ? $recP->formDB
	    : $recP);
    if (isset($rec['v_status']) && ($rec['v_status'] !== STATUS_YES))  return '';
    if (VM_self_accommodated($rec)) return '';
    if (!bForm_vm_Room::_mightBeSharable($rec) || self::_room_is_full($rec)) return '';
    
    $cotenants = VM_cotenants()->get_cotenants($rec);
    if (is_object($recP)){
      $recP->forceROmode();
      VM::$formDummy->MODE = $recP->MODE;
      $RW = ($recP->MODE == 'RW');
    }
    if (VM_tooLateForAccommodation($recP,$cotenants)){
      $RW = False;
    }elseif (!is_object($recP) && (!empty($cotenants) || self::get_lease_options($rec,VM_no_cotenants))){
      // The cotenant can't be changed in the "applicants list", this would be too messy
      $RW = False;
    }elseif ($RW === 'if_empty' && empty($cotenants)){
      $RW = True;
    }
    b_debug::_debug("RW = ".var_export($RW,True),cnf_dev);

    $lease_id = $rec['lease_id'];
    $name_for_select = $name . $lease_id;
	    
    if ($RW){ //======================================================================================
      
      // Look for the candidates for co-tenants
      $candidates = VM_cotenants()->get_candidate($rec);

      // Set the current value (shown in RO mode and as selected in RW)
      if (self::get_lease_options($rec,VM_no_cotenants)){
	VM::$formDummy->getValue_reset($name_for_select,VM_no_cotenants);
      }else{
	foreach($cotenants as $lm_id=>$leases){
	  foreach($leases as $l_id){
	    if ($l_id == $lease_id) continue;
	    $r = self::_leaseInfo($l_id);
	    $candidates[$l_id] = $r['lease_name'];
	    VM::$formDummy->getValue_reset($name_for_select,$l_id);
	  }
	}
      }

      if (!empty($candidates)) $candidates['*select_name*'] = 'cotenant';
      $reply = empty($candidates)
	? ''
	: x("span class='only_online'",
	    x("form action='".b_url::same('?resetcache_once=1',$anchor)."' method='post' enctype='multipart/form-data' name='TENANT$lease_id'",
	      VM::$formDummy->selectMenu_exec($name_for_select,
					      $candidates,
					      "onchange='submit()'",
					      $onlyRW=False). // it was "True"
	      (($share_with = bForm_vm_Visit::_getGSF('share',$rec)) 
	       ? str_replace('?',' ','<br>'.x('span class=smaller-text',join('<br>',$share_with))) 
	       : '').
	      (!is_object($recP)&&($n=@$rec['v_accompaning']) ? "<br><span class=smaller-text>Accompaning persons - me + $n</span>" : "").
	      "<input type='hidden' name='lease_id' value='".$lease_id."'/>".
	      "<input type='hidden' name='v_id'     value='".$rec['v_id']."'/>".
	      "<input type='hidden' name='a_id'     value='".$rec['a_id']."'/>".
	      (is_object($recP)?"<input type='hidden' name='_post_bForm_ID_once' value='".$recP->ID."'/>":"").
	      "<noscript><input type='submit' value='go'/></noscript>"),
	    'POST');
      if (!empty($reply)) b_debug::_debug("reply=array(".join(', ',$candidates).")");

    }else{    //======================================================================================	
      
      // In RO mode just print the cotenant names
      $names = VM_cotenants()->get_cotenants($rec,False,'names');
      if (empty($names) && 
	  !VM_tooLateForAccommodation($recP,$names) && 
	  !self::get_lease_options($rec,VM_no_cotenants)) $names = array(b_fmt::redText('to be selected'));
      $reply = join(',',$names);
      if (!empty($reply)) b_debug::_debug($reply);
    }
    return $reply;
  }
}
