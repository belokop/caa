<?php
locateAndInclude('bForm_vm');
class bForm_vm_Lease extends bForm_vm{

  private static $isReady = False;
  
  /*
   * Create menu "Nordita pays for N tenants only"
   * 'lease_option' is a serialized array
   */
  public static function get_lease_options($lease_id,$mode='menu',$data=array(),$verbose=False){

    // Parse input arguments
    if (is_array($lease_id)){
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
	case 'lease_option':
	  $lease_option = unserialize($v);
	}
      }
    }else{
      $column   = 'lease_id'; 
      $args = array($column => $lease_id);
    }    

    // Complete the arguments for the recursive invocations
    if (!in_array($mode,array('set','reset','menu'))){
      $no_data = (is_string($data) && empty($args[$data]) && myPear_db()->columnExists($data,'abs_leases'));
      if ($no_data || empty($lease_option)){
	if (empty($lease_option)){
	  $lease_option = array();
	  $select[] = 'lease_option';
	}
	if ($no_data){
	  $select[] = $data;
	}
	//      var_dump(is_string($data),is_string($data) && empty($args[$data]),$select);
	$q = myPear_db()->qquery("SELECT ".join(',',$select)." FROM abs_leases WHERE $column = '$lease_id'",cnf_dev);
	while($r = myPear_db()->next_record($q)){
	  if ($no_data){
	    $args[$data] = $r[$data]; 
	  }
	  if (empty($lease_option)){
	    $args['lease_option'] = $r['lease_option']; 
	    $lease_option = unserialize($r['lease_option']);
	  }
	}
      }
    }

    
    // Do the work
    switch($mode){
      
    case 'get':
      // Sic, the reply is an array !!
      $reply = $lease_option;
      if (is_string($data)) $reply[$data] = @$args[$data];
      break;
      
    case 'reset':
      if (empty($data)){
	$lease_option = array('pay'=>VM_default_lease_pay);
      }else{
	$lease_option = self::get_lease_options($args,'get');
	unset($lease_option[$data]);
	ksort($lease_option);
      }
      myPear_db()->qquery("UPDATE abs_leases SET lease_option='".serialize($lease_option)."' WHERE $column='$lease_id'",cnf_dev);
      $reply = True;
      break;
      
    case 'set':
      // Get the current values
      $lease_option = self::get_lease_options($args,'get');
      foreach($data as $k=>$v) $lease_option[$k] = $v;
      // Update
      ksort($lease_option);
      myPear_db()->qquery("UPDATE abs_leases SET lease_option='".serialize($lease_option)."' WHERE $column='$lease_id'",cnf_dev);
      if (@$data['pay'] === 'one'){
	if (empty($args['lease_id'])){
	  $q = myPear_db()->qquery("SELECT lease_id FROM abs_leases WHERE $column='$lease_id'",True);
	  $leases = array();
	  while($r = myPear_db()->next_record($q)) $leases[] = $r['lease_id'];
	}else{
	  $leases = array($args['lease_id']);
	}
	foreach($leases as $lease_id){
	  $l = bForm_vm_Lease::_leaseInfo($lease_id);
	  bForm_vm_Lease::get_lease_options($l,'set',array(VM_no_cotenants=>VM_no_cotenants),True);
	}
      }
      $reply = True;
      break;
      
    case 'get_pay':
    case 'get_pay_HR':
      $r = self::get_lease_options($args,'get');
      $reply = ((@$r['pay'] === VM_default_lease_pay) && ($data === 'RO')
		? cnf_emptyString
		: ($mode == 'get_pay'
		   ? @$r['pay']
		   : self::get_lease_options($args,'excel_int')));
      break;
      
    case 'excel_int':
      // Generate an "internal" (i.e. for organizers,admins,etc) human readable comment how to pay for the accommodation
    case 'excel_ext':
      // Generate human readable comment for the hotel personnel how to pay for the accommodation
      
      $r = self::get_lease_options($args,'get','lease_aid');
      if (empty($r['pay'])){
	myPear::DEBUG("empty r[pay], return the default ".b_fmt::joinX(', ',$r));
	//	b_debug::traceBack("empty r[pay], return the default");
	$r['pay'] = VM_default_lease_pay;
      }

      if ($r['lease_aid'] == VM::$OA->ID){
	$reply = '';
      }elseif ($r['pay'] == VM_default_lease_pay){
	$reply = '';
      }elseif ($r['pay'] == 'nothing'){
	$reply = ($mode=='excel_ext'
		  ? strToUpper('Payed by the tenant')
		  : myOrg_nameS." pays nothing");
      }elseif(!empty($r['pay'])){
	$reply = ($mode=='excel_ext'
		  ? strToUpper(myOrg_nameS." pays for $r[pay] tenant")
		  : myOrg_nameS." pays for $r[pay]");
      }else{
	b_debug::internalError('empty lease_option');
      }
      break;

    case 'menu':
      // Menu selector
      $q = myPear_db()->query("SELECT a_capacity FROM abs_rooms LEFT JOIN abs_leases ON lease_aid=a_id WHERE lease_id = '$lease_id'");
      $reply = array();
      while($r = myPear_db()->next_record($q)){
	foreach(VM::$pay_options as $k=>$o){
	  if ($k < $r['a_capacity']) $reply[$o] = myOrg_nameS." pays ".($k>0 ? 'for ' : '').$o;
	}
      }
      if (!empty($reply)) $reply[VM_default_lease_pay] = ($data === 'RO'
						       ? cnf_emptyString
						       : myOrg_nameS." pays ".VM_default_lease_pay);
      break;
      
    case VM_room_is_full:
    case VM_no_cotenants:
      $reply = isset($lease_option[$mode]);
      break;
      
    default:
      b_debug::internalError("Unexpected mode = \"$mode\"");
    }
    if ($verbose)  myPear::DEBUG(__FUNCTION__."($args[lease_id],$mode) ".x('em',($reply===cnf_emptyString ? b_fmt::escape($reply) : $reply)));
    return $reply;
  }

  /*
   *
   */
  public static function shared_room_no_cotenants_wanted($lease_id,$verbose=False){
    $reply = self::get_lease_options($lease_id,VM_no_cotenants,array(),False); 
    if ($verbose) myPear::DEBUG(sprintf("%s(%s): <em>%s</em>",__FUNCTION__,(is_array($lease_id)?$lease_id['lease_id']:$lease_id),var_export($reply,True)));
    return $reply;
  }

  /*
   *
   */
  public static function shared_room_is_full($lease_id,$verbose=False){
    $reply = (self::get_lease_options($lease_id,VM_room_is_full,array(),False) ||
	      self::shared_room_no_cotenants_wanted($lease_id,False));
    if ($verbose) myPear::DEBUG(sprintf("%s(%s): <em>%s</em>",__FUNCTION__,(is_array($lease_id)?$lease_id['lease_id']:$lease_id),var_export($reply,True)));
    return $reply;
  }

  /*
   * Check all the leases and (re)set flag VM_room_is_full
   */
  public static function update_fullRoomFlag($leases=array()){
    // Get the list of leases
    if (empty($leases)){
	$q = myPear_db()->qquery("SELECT lease_id,lease_option,a_capacity FROM abs_leases LEFT JOIN abs_rooms ON a_id=lease_aid WHERE a_capacity > 1",True);
	while($r = myPear_db()->next_record($q)) $leases[] = $r['lease_id'];
    }
    
    // Walk thru the leases, set the value
    foreach(array_unique($leases) as $lease_id){
      self::update_fullRoomFlag_exec($lease_id);
    }
  }

  private static function update_fullRoomFlag_exec($lease_id,$r=array()){
    if (empty($r)) $r = bForm_vm_Lease::_leaseInfo($lease_id);
    foreach(VM_cotenants()->get_cotenants($lease_id,True) as $lm_id=>$cotenants){
      $n_cotenants = count($cotenants);
      $VM_room_is_full = bForm_vm_Lease::get_lease_options($r,VM_room_is_full);
      if ($n_cotenants == $r['a_capacity']){
	if (!$VM_room_is_full) bForm_vm_Lease::get_lease_options($r,'set',array(VM_room_is_full=>VM_room_is_full));
      }elseif($n_cotenants < $r['a_capacity']){
	if ( $VM_room_is_full) bForm_vm_Lease::get_lease_options($r,'reset',VM_room_is_full);
      }else{
	myPear::ERROR(sprintf("a_capacity=%s but %s cotenants",$r['a_capacity'],$n_cotenants));
      }
    }
  }
  
  /*
   * Return the visit for the given lease
   */
  public static function visit($lease_id){
    $q = myPear_db()->query("SELECT * FROM abs_leases LEFT JOIN zzz_avatars  ON   av_id = v_avid WHERE lease_id = '$lease_id'");
    $reply = Null;
    while($r = myPear_db()->next_record($q)){
      $reply = new bForm_vm_Visit($r['lease_vid']);
    }
    return $reply;
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
  public static function _get_allLeases($visit){
    if (is_object($visit)) $v_id = $visit->ID;
    else                   $v_id = $visit;
    
    $leases = array();
    $q = myPear_db()->query("SELECT lease_id,hut_code,a_id,a_name FROM abs_leases ". // a_code
			    " LEFT JOIN abs_rooms ON a_id = lease_aid ".
			    " LEFT JOIN abs_huts ON hut_id = a_hutid ".
			    " WHERE lease_vid = '$v_id' ORDER BY lease_start");
    while($r = myPear_db()->next_record($q)){
      $leases[$r['lease_id']] = self::_hut_code($r);
      // Sanity. Huge performance overhead...
      //      if (cnf_dev && (($l=@myPear::getClassFromID($r['lease_id']),True,True)){
      //     	myPear::WARNING_T("Bogus lease $l($r[lease_id]), not 'bForm_vm_Lease'");
      //      }
    }

    // Create an empty lease on the fly is there is no-one
    if (empty($leases) && is_object($visit) && ($visit->getValue('v_status',True) != STATUS_NO)){
      if (is_object($visit)){
	$lease_id = self::create_firstLease(0,$visit);
      }else{
	$lease_id = self::create_firstLease(0,new bForm_vm_Visit($v_id));
      }
      if (!empty($lease_id)){
	$l = new bForm_vm_Lease($lease_id);
	$leases[$l->ID] = $l->hut_code();
      }
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
      if (cnf_debug||cnf_show_db||cnf_dev) b_debug::xxx($visit->name());
      myPear_db()->query("INSERT INTO abs_leases SET ".
			 join(',',array('lease_start='. b_time::check_in ($visit->getValue('v_start',1)),
					'lease_end='  . b_time::check_out($visit->getValue('v_end',1)),
					'lease_vid='  . $visit->ID,
					'lease_id='   . $lease_id)));
    }
    return $lease_id;
  } 

  /*
   * Return leases sharing (or possibly sharing) the given apartment
   */
  public static function getTenants($l_start,$l_end,$a_id,$include_booked=False){

    $reply = array();
    if (is_numeric($a_id)){
      $w = array("a_id = '$a_id'",
		 '('.join(' OR ',array("(lease_start BETWEEN $l_start AND $l_end)",
				       "(lease_end   BETWEEN $l_start AND $l_end)",
				       "(lease_start <= $l_start AND lease_end >= $l_start)",
				       "(lease_start <= $l_end   AND lease_end >= $l_end)",
				       )).')');
      
      $reply = array();
      $q = myPear_db()->query("SELECT lease_id,av_firstname,av_lastname FROM abs_leases ".
			      " LEFT JOIN abs_rooms ON lease_aid = a_id ". 
			      " LEFT JOIN abs_visits     ON lease_vid = v_id ". 
			      " LEFT JOIN zzz_avatars    ON v_avid = av_id ". 
			      " WHERE ".join(' AND ',$w));
      while($r = myPear_db()->next_record($q)) $reply[$r['lease_id']] = bForm_Avatar::_fmtName('Lf',$r);
      asort($reply);
    }
    return $reply;
  }


  /*
   * Return the lease name without instantiating the Lease class
   */
  public static function queryLease($lease_ids,$debug=False){
    foreach($lease_ids as $lease_id) $where[] = "lease_id = $lease_id";
    return myPear_db()->query("SELECT * FROM abs_leases ".
			      " LEFT JOIN abs_rooms ON lease_aid = a_id ". 
			      " LEFT JOIN abs_visits     ON lease_vid = v_id ". 
			      " LEFT JOIN zzz_avatars    ON v_avid = av_id ". 
			      " WHERE " . join(' OR ',$where));
  }

  public static function _leaseInfo($lease_id,$name_only=False,$message=''){
    $reply = array();
    $q = self::queryLease(array($lease_id));
    while($r = myPear_db()->next_record($q)) $reply = $r;
    if ($name_only){
      $reply =  bForm_Avatar::_fmtName('Lf',$reply);
      if (cnf_dev && !empty($message)) b_debug::xxx("$reply $message");
    }else{
      $reply['lease_name'] =  bForm_Avatar::_fmtName('Lf',$reply);
    }
    return $reply;
  }

  function __construct($id,$visit=Null){
    parent::__construct($id);
    if ($visit instanceof bForm_vm_Visit){
      $this->hookVisit($visit);
    }
    $this->isEmbedded(); // call it here to prevent redefining the MODE by the "isEmbedded" function

    //
    // apply the accommodation if requested
    if (@$_POST['_post_bForm_ID_once'] == $this->ID){

      if (isset($_POST['_who_pays'])){
	// assign "who pays for the accommodation"
	self::get_lease_options($this->ID,'set',array('pay'=>$_POST['_who_pays']));
      }

      if (isset($_POST['lease_aid'])){

	// assign apartment to the lease
	$lease_aid = $_POST['lease_aid'];
	$this->hookRoom($lease_aid,$verbose=True);
	
	// assign cotenant(s) to the lease
	if (is_numeric($lease_aid) && ($a = myPear::getInstance_new('bForm_vm_Room',$lease_aid,'strict')) 
	    // && ($a->isSharable($this) || self::shared_room_is_full($this->ID))
	    ){
	  $cotenants = array();
	  foreach(preg_grep('/cotenant\d+$/',array_keys($_POST)) as $k){
	    if ($_POST[$k] == VM_no_cotenants){
	      VM_cotenants()->set_cotenants(array(VM_no_cotenants));
	    }else{
	      if (empty($cotenants))	  $cotenants = array($this->ID);
	      $cotenants[] = $_POST[$k];
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
    $q = myPear_db()->query("SELECT * FROM $this->TABLE ".
			    " LEFT JOIN abs_visits ON v_id=lease_vid ".
			    " LEFT JOIN abs_rooms  ON a_id=lease_aid ".
			    " LEFT JOIN abs_huts   ON a_hutid=hut_id ".
			    " WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q))   $this->formDB = $r;
  }

  /*
   * Get the apartment type
   */
  function hut_code(){
    return self::_hut_code(array('a_id'    => $this->getValue('a_id',True,True),
				 'a_name'  => $this->getValue('a_name',True,True),
				 //'a_code'  => $this->getValue('a_code',True,True),
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
	// $reply = $buffer['a_code'];
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
  function getForm(){

    // Process "delete lease" which comes from the "visit dialog"
    if ($this->myPost_bForm() && @$_POST['lease_aid'] === LODGING_RM){
      $this->hookRoom(LODGING_RM);
      return;
    }

    if ($this->parentForm->isApproved()){
      $this->dbg('------------start');
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
					       btn_deleteLease,
					       b_url::same("?action_once=".btn_deleteLease.
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
	// Cotenants
	// !self::shared_room_no_cotenants_wanted($this->ID,True)
	if (( self::shared_room_is_full($this->ID,True) || bForm_vm_Room::_isSharable($this->formDB,$this)) &&
	    (!self::shared_room_no_cotenants_wanted($this->ID,True) || $this->MODE === 'RW')){
	  if ($this->MODE == 'RO' && !VM_cotenants()->get_cotenants($this->ID)) $this->MODE = 'RW';
	  $cotenant = self::select_cotenant('_cotenant',$this,($this->MODE === 'RW'));
	  if (!empty($cotenant)) $dialog[b_rawData] = array('Cotenant',$cotenant);
	}
	
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
	
	// Who pays for the accommodation?
	if ($this->getValue('lease_aid')){
	  $this->defineVariable(' ','_who_pays');
	  $this->formDB['_who_pays'] = $_who_pays = self::get_lease_options($this->ID,'get_pay',$this->MODE,cnf_dev);
	  if ($_who_pays !== cnf_emptyString) $dialog['_who_pays'] = array('selectMenu',
									   self::get_lease_options($this->ID,'menu',array(),cnf_dev),
									   "onchange='submit()'");
	}
      }
      
      // Accommodation wish
      if ($this->parentForm->isVisitType_program() && ($_virt_wish=$this->parentForm->getValue('v_acc_wish',True))){
	$this->defineVariable(array('_virt_wish'=>b_fmt::redText('Accommodation wish','greyText')),True);
	$this->isRO['_virt_wish'] = True;
	$this->formDB['_virt_wish'] = b_fmt::redText($_virt_wish,'greyText');;
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
      $this->dbg('------------end');
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
   *
   */
  private static function _mayBook($hut_code,$visit_class){
    $this->hookVisit();
    //if (in_array($hut_code,array_merge(array(HUT_BI),array_values($this->parentForm->e->accommodationOptions()->get_BI())))){
    if (in_array($hut_code,array_merge(array(HUT_BI),array_values(VM_accommodationOptions($this->parentForm->e)->get_BI())))){
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
    b_debug::xxx($reply,array(2,$this->debug,'AA'));
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
    if ($exclude_OA && (($lease_aid == VM::$OA->ID) || 
			($lease_aid == VM::$BR->ID))) $reply = False;
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
  
  function clean_cotenants(){
    foreach(VM_cotenants()->get_cotenants($this->ID,True) as $lm_id=>$leases){
      $this->dbg("lm_id=$lm_id leases=".join(', ',$leases) ,cnf_dev);
      VM_cotenants()->deleteMember($lm_id);
      foreach($leases as $lease_id) self::get_lease_options($lease_id,'reset',VM_room_is_full);
    }
  }

  /*
   * Hook room to the lease
   */
  function hookRoom($lease_aid,$verbose=True){
    if (cnf_dev) $verbose = True;
    b_debug::xxx();

    if ($lease_aid === LODGING_RM){
      b_debug::xxx('REMOVE',array(2,$this->debug,'M'));

      // clean the cotenants list (if any) since the apartment is changed  
      $this->clean_cotenants();
      if ($verbose) myPear::MESSAGE_add($this->_periodToString('Remove accommodation for'));
      $this->getValue_reset_forced('lease_aid',0);
      $this->hookVisit();
      self::get_lease_options($this->ID,$mode='reset');
    }elseif ($lease_aid && (($old_aid=@$this->formDB['lease_aid']) != $lease_aid)){
      // sanity
      if (!(myPear::getInstance_new('bForm_vm_Room',$lease_aid,'strict') instanceof bForm_vm_Room)) {
	b_debug::internalError("$lease_aid is not bForm_vm_Room",$this);
      }
      $this->hookVisit();

      // clean the cotenants list (if any) since the apartment is changed  
      $this->clean_cotenants();

      $this->updateDB(array('lease_aid'=>$lease_aid));
      if ($verbose) myPear::MESSAGE_add($this->_periodToString(($old_aid?'Change':'Set').' accommodation for',$old_aid));
    }
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
    bTiming()->cpu(__FUNCTION__);
    $this->hookVisit();
    if ($this->isWritable === Null) $this->isWritable = (bool)!$this->not_isWritable_point && $this->parentForm->isWritable() && $this->parentForm->isApproved();
    bTiming()->cpu();
    return $this->isWritable;
  }

  function split($date=Null){
    $this->hookVisit(); 
    myPear::MESSAGE($this->_periodToString('Split lease'));
    if (!$date) $date = (int)((($e=$this->getValue('lease_end'))+($s=$this->getValue('lease_start')))/2);
    $this->setDate(array('lease_end'  => $date),True);

    $lnew = new bForm_vm_Lease('empty');
    $lnew->updateDB(array('lease_start'=> b_time::_($date,CONST_TIME_CI),
			  'lease_end'  => $e,
			  'lease_aid'  => 0,
			  'lease_vid'  => $this->parentForm->ID,
			  'lease_id'   => (($id=(int)$this->ID) ? $id : bMaster()->reserve_masterCounter())));

    $this->parentForm->get_allLeases(True);

    if (0)    myPear::MESSAGE(join('<br/>',array($this->_periodToString(),
						 $lnew->_periodToString())));
			 
    // Adjust the new values
    self::adjust_all($this->parentForm,$lnew->ID);

    return $lnew->ID;
  }
  
  function delete(){
    $this->dbg("Lease ".$this->ID." ".$this->getValue('a_name'),$debug=(bool)($this->debug or cnf_dev));
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
		 'v_eid'      =>$this->getValue('lease_vid'));
    if ($a_id = $this->getValue('lease_aid')){
      $room = myPear::getInstance_new('bForm_vm_Room',$a_id,'strict');
      $rec['lease_aid'] = $a_id;
      $rec['hut_name']  = $room->hut->getValue('hut_name');
      $rec['hut_code']  = $room->hut->getValue('hut_code');
      $rec['a_name']    = $room->getValue('a_name');
      $rec['a_capacity']= $room->getValue('a_capacity');
    }
    return self::_freeRooms(VM_accommodationOptions($this->parentForm->e),
			    $rec,
			    $this->hasAccommodation(False));
  }
  
  private static function _dbg($txt,$function='_freeRooms',$args=array()){
    foreach($args as $k=>$v) if (is_array($v)) $args[$k] = '&#91;'.join(',',$v).'&#93;';
    myPear::DEBUG(sprintf("%s(%s): %s",$function,join(',',$args),$txt));
  }

  /*
   * Check the pool and get a list of available rooms in the hut for the given dates & event 
   */
  public static function _freeRooms($accommodationOptions,$rec,$hasAccommodation=False,$hut_code=Null){
    
    list($start,$end) = array(b_time::check_in($rec['lease_start']),
			      b_time::check_out($rec['lease_end']));

    self::_dbg(b_time::period($start,$end),__function__,array($accommodationOptions,$hut_code));
    
    $huts = array();
    if (!empty($hut_code)){
      $where[] = empty($accommodationOptions)
	? '1'
	: ' hut_id != '.join(' AND hut_id !=  ',$accommodationOptions); // excludes
      $where[] = (is_numeric($hut_code)
		  ? "hut_code = '$hut_code'"
		  : "1");
      $q = myPear_db()->qquery("SELECT hut_id FROM abs_huts WHERE ".join(' AND ',$where),cnf_dev);
      while ($r = myPear_db()->next_record($q)) $huts[] = $r['hut_id'];
      $event_context = False;
      $accommodationOptions = False;
    }else{
      // Chase for the associated event ID, since VM::_MENU()->context() 
      // might give a wrong answer for the search results.
      if (!is_object($accommodationOptions) && ($e_id = @$rec['v_eid'])){
	self::_dbg("Recover e_id = $e_id");
	$accommodationOptions = VM_accommodationOptions($e_id);
      }
      $event_context = is_object($accommodationOptions);
      if ($event_context){
	// Use only those "huts" which are selected by the administration, i.e. listed in vm_accommodationOptions
	foreach($accommodationOptions->getMembers() as $x=>$r) $huts[] = $r['lm_key'];
      }else{
	// Check ALL available rooms
	foreach(bList_vm_accommodationOptions::get_allHuts() as $hut_id=>$hut_name) $huts[] = $hut_id;
      }
    }

    self::_dbg(b_fmt::redText("context ".($event_context ? 'event '.VM::$e->name() : 'org')));
    self::_dbg('huts = '.join(', ',$huts));
    
    //Loop over all the huts for the event
    $rooms = $descr = $n_TR = array();
    foreach($huts as $hut_id){
      $hut = new bForm_vm_Hut($hut_id);
      $TR = $hut->get_total_rooms($accommodationOptions);
      self::_dbg($hut->getValue('hut_id').' - '.$hut->name().' '. $hut->name_total_rooms($accommodationOptions));
      
      // Loop over all the rooms available for this time period
      $q_r = myPear_db()->qquery(sprintf("SELECT a_id,a_price,a_name,a_quantity,a_capacity,a_quantity FROM abs_rooms ".
					 " LEFT JOIN abs_huts ON a_hutid=hut_id ".
					 " WHERE a_end >= %s ".
					 " AND ( a_start <= %s OR a_start = 0 OR a_start IS NULL) ".
					 " AND a_hutid = %s ".
					 " AND hut_code != '".HUT_BI."' ".
					 " ORDER BY CONCAT(hut_code,hut_name,a_capacity) ASC",
					 $start,$end,$hut->ID),cnf_dev);
      while ($r_room = myPear_db()->next_record($q_r)){
	if (!empty($hut_code)){
	  $reply[$hut->ID] = $hut->name();
	  break;
	}
	// Room ID
	$r_room['hut_name'] = $hut->name();
	$id = bForm_vm_Room::_a_name($r_room,True);
	self::_dbg($id);
	if (!empty($rooms) && in_array($id,array_keys($rooms))){
	  myPear::ERROR("Configuration error: $id is duplicate");
	}else{
	  $rooms[$id] = 0;
	}
	

	if ($TR){
	  $n_TR[$id] = $TR;
	  $tt_booked_TR[$hut->ID] = 0;
	  $my_booked_TR[$hut->ID] = 0;
	  $xx_booked_TR[$hut->ID] = array();
	}elseif($r_room['a_capacity'] == 1){
	  $tt_booked_single[$id] = 0;
	  $my_booked_single[$id] = 0;
	  $xx_booked_single[$id] = array();
	}elseif($r_room['a_capacity'] > 1){
	  $tt_booked_shared[$id] = 0;
	  $my_booked_shared[$id] = 0;
	  $xx_booked_shared[$id] = array();
	}else{ 
	  b_debug::internalError("??? room capacity = '$r_room[a_capacity]'");
	}

	// How many rooms of this type have we for the event?
	$a_quantity = ($event_context
		       ? $accommodationOptions->get_a_value($r_room['a_id'],$hut->ID,'a_quantity')
		       : $r_room['a_quantity']);
	if (empty($a_quantity)) b_debug::print_r($r,"empty a_quantity for $id");
	$descr[$id] = array($hut->ID,$r_room['a_id'],$a_quantity,$r_room['a_capacity']);

	// Loop over leases for the room
	if ($event_context){
	  $q = myPear_db()->qquery($sql=sprintf("SELECT lease_id,v_eid,av_lastname,av_firstname,e_name ".
						" FROM abs_leases ".
						" LEFT JOIN abs_visits  ON lease_vid = v_id ".
						" LEFT JOIN zzz_avatars ON v_avid = av_id ". 
						" LEFT JOIN abs_events  ON v_eid = e_id ".
						" WHERE lease_aid = %s ".
						" AND lease_start <= $end ".
						" AND lease_end   >= $start ORDER BY CONCAT (e_name,v_host_avid)",
						$r_room['a_id']),True);
	  while ($r = myPear_db()->next_record($q)){
	    $myLease = (@$r['v_eid'] == VM::$e->ID);
	    $r['a_capacity'] = $r_room['a_capacity'];
	    $tenant = bForm_Avatar::_fmtName('Lf',$r) . (empty($r['e_name']) ? '' : ', '.x('em',$r['e_name']));
	    if ($TR){
	      if($r_room['a_capacity'] > 1) self::update_fullRoomFlag_exec($r['lease_id'],$r);
	      $increment = (($r_room['a_capacity'] == 1) || bForm_vm_Lease::shared_room_is_full($r,1)
			    ? 1
			    : 0);
	      $tt_booked_TR[$hut->ID] += $increment;
	      if ($myLease) $my_booked_TR[$hut->ID] += $increment;
	      else{
		b_debug::print_r($r,$tenant);
		$xx_booked_TR[$hut->ID][] = $tenant;
	      }
	    }elseif($r_room['a_capacity'] == 1){
	      if (!empty($r['lease_id'])){
		$tt_booked_single[$id]++;
		if ($myLease) $my_booked_single[$id]++;
		else{
		  b_debug::print_r($r,$tenant);
		  $xx_booked_single[$id][] = $tenant;
		}
	      }
	    }elseif($r_room['a_capacity'] > 1){
		    self::update_fullRoomFlag_exec($r['lease_id'],$r);
	      if (bForm_vm_Lease::shared_room_is_full($r,1)){
		$tt_booked_shared[$id]++;
		if ($myLease) $my_booked_shared[$id]++;
		else{
		  b_debug::print_r($r,$tenant);
		  $xx_booked_shared[$id][] = $tenant;
		}
	      }
	    }else{
	      b_debug::internalError('????????? capacity='.$r_room['a_capacity']);
	    }
	    if (False){
	      ob_start();
	      $n_tt = ($TR ? 'TR' :
		       ($r_room['a_capacity'] == 1 
			? $tt_booked_single[$id]
			: $tt_booked_shared[$id]
			));
	      $v = new bForm_vm_Visit($r['lease_vid']);
	      $msg = "$n_tt - ".$v->period().' '.$v->av->name();
	      if ($v->e) $msg .= ' '.$v->e->name();
	      ob_get_contents();
	      ob_end_clean();
	      if (empty($v->e) || (!empty($v->e) && $v->e->ID != VM::$e->ID))  self::_dbg(b_fmt::redText($msg));
	      else                                                             self::_dbg($msg);
	    }	 
	  }
	}
      }
    }

    /*
    // debug...
    $r = $rooms;      b_debug::print_r($r,__function__.'(): available rooms','nosort');
    $r = $descr;      b_debug::print_r($r,__function__.'(): desription','nosort');
    $r = $n_TR;       b_debug::print_r($r,__function__.'(); n_TR','nosort');
    $r = $tt_booked_TR;  b_debug::print_r($r,__function__.'(): tt_booked_TR','nosort');
    $r=$tt_booked_single;b_debug::print_r($r,__function__.'(): bookings single');
    $r=$xx_booked_single;b_debug::print_r($r,__function__.'(): STRANGES bookings single');
    $r=$tt_booked_shared;b_debug::print_r($r,__function__.'(): bookings shared');
    $r=$xx_booked_shared;b_debug::print_r($r,__function__.'(): STRANGES bookings shared');
    */

    // Prepare the reply
    if (empty($reply)){
      $reply = array();
      if (empty($hut_code)){
	if ($hasAccommodation) $reply[LODGING_RM] = LODGING_RM_TXT;
	if ($event_context) foreach($accommodationOptions->get_BI() as $k=>$v) $reply[$k] = $v;
      }
      
      foreach(array_keys($rooms) as $id){
	list($hut_id,$a_id,$a_quantity,$a_capacity) = $descr[$id];
	$skip = False;
	$count = 0;
	if (empty($hut_code)){
	  if (empty($n_TR[$id])){	// Big unlimited hotels with a_quantity number of rooms of the given capacity
	    if ($a_capacity==1){
	      if (($count=$tt_booked_single[$id]) >= $a_quantity) $skip = "single booked $count/$a_quantity";
	    }else{
	      if (($count=$tt_booked_shared[$id]) >= $a_quantity) $skip = "shared booked $count/$a_quantity";
	    }
	    if ($a_quantity <= 0) $skip = 'not yet requested';
	  }else{	// TR - hotel with fixed number of rooms of variable capacity
	    foreach($tt_booked_TR as $h_id=>$x){
	      if($hut_id == $h_id) $count = $x;
	    }
	    $a_quantity = $n_TR[$id];
	    if ($count >= $a_quantity) $skip = sprintf("total number of booked rooms is reached %s/%s",$count,$a_quantity);
	  }
	  
	  // increase a_quantity if the room was booked
	  if ($count && ($count > $a_quantity)) { 	
	    if (empty($xx_booked_single[$id]) && empty($xx_booked_shared[$id]) &&
		empty($n_TR[$id])){ 
	      myPear::WARNING("Increase a_quantity $a_quantity ==> $count for $id");
	      $accommodationOptions->set_a_value(max($count,$a_quantity),$a_id,$hut_id,'a_quantity');
	    }else{
	      if (!empty($my_booked_single[$id]) || !empty($my_booked_shared[$id])){
		$stranges = (empty($xx_booked_single[$id])
			     ? $xx_booked_shared[$id]
			     : $xx_booked_single[$id]);
		myPear::WARNING(x('strong',b_time::period($start,$end)." $id is OVERBOOKED ($count rooms booked / $a_quantity available))").
				"<br/>Clashing bookings:<br/>".join('<br/>',$stranges));
	      }
	      // Update the options
	      if ($skip && $count) { 	// Update the options
		if ($event_context){
		//	  self::_dbg(b_fmt::redText("IGNORE $id - $skip, the room was booked $count/$a_quantity times (???)"));
		//	  if (empty($n_TR[$id])) $accommodationOptions->set_a_value(max($count,$a_quantity),$a_id,$hut_id,'a_quantity');
		}
		//	$skip = False;
	      }
	    }
	  }
	}
	
	if ($skip){
	  self::_dbg("Skip $id - $skip");
	}else{
	  // Save the room in the list... finally...
	  $reply[$a_id] = $id;
	  if ($event_context){
	    list($n_free,$n_total) = (empty($n_TR[$id])
				      ? array($a_quantity - ($a_capacity==1 ? $tt_booked_single[$id] : $tt_booked_shared[$id]),
					      $a_quantity)
				      : array($n_TR[$id]-$count,
					      $n_TR[$id]));
	    self::_dbg(b_fmt::redText("ACCEPT ".($a_quantity-$count)."(free)/$a_quantity(a_quantity) $id"));
	  }
	}
      }
      
      // if the reference lease has a room attached, include it in the reply
      if (($a_id = @$rec['lease_aid']) && empty($reply[$a_id])){
	$reply[$a_id] = bForm_vm_Room::_a_name($rec,True);
	self::_dbg(b_fmt::redText("FORCED ACCEPT ".$reply[$a_id]));
      }
    }
    
    self::_dbg("reply=".join(',&nbsp;',array_values($reply)),__function__,array($accommodationOptions,$hut_code));
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
  public static function select_cotenant($name,$recP,$mode='auto'){
    locateAndInclude('bForm_vm_Room');

    $rec = (is_object($recP)
	    ? $recP->formDB
	    : $recP);

    if (($rec['lease_end'] < time()+24*3600) ||
	!(bForm_vm_Room::_isSharable($rec) || self::shared_room_is_full($rec['lease_id']))) return '';
    
    $cotenants = VM_cotenants()->get_cotenants($rec['lease_id']);
    
    $RW = False;
    if ($mode === 'auto' && empty($cotenants)) $RW = True;
    elseif (is_bool($mode))                    $RW = $mode;
    
    if ($RW){ //======================================================================================
      // the name must be unique
      $counter = $rec['lease_id'];
      $name_for_select = $name . $counter;
      
      // Look for the candidates for co-tenants
      $candidates = VM_cotenants()->get_candidate($rec);
      
      // Add the current cotenant to the candidates list 
      foreach($cotenants as $lm_id=>$leases){
	foreach($leases as $lease_id){
	  $r = self::_leaseInfo($lease_id);
	  $candidates[$lease_id] = $r['lease_name'];
	  VM::$formDummy->formDB[$name_for_select] = $lease_id;
	}
      }
      
      // Remove the ansökande
      unset($candidates[$rec['lease_id']]);
      if (is_object($recP) && self::shared_room_no_cotenants_wanted($rec)) $recP->getValue_reset($name_for_select,VM_no_cotenants);
      $debug = (empty($candidates) ? $x=array('empty...') : $candidates);
      b_debug::print_r($debug,"candidates for ".bForm_Avatar::_fmtName('Lf',$rec).' '.b_time::period($rec['lease_start'],$rec['lease_end']));
      if (empty($rec['v_id'])) b_debug::var_dump($rec,"empty(rec[v_id]");	
      $reply = (empty($candidates)
		? ''
		: x("span class='only_online'",
		    x("form action='".b_url::same()."' method='post' enctype='multipart/form-data' name='TENANT$counter'",
		      VM::$formDummy->selectMenu_exec($name_for_select,
						      $candidates,
						      "onchange='submit()'",$onlyRW=True).
		      "<input type='hidden' name='lease_id' value='".$rec['lease_id']."'/>".
		      "<input type='hidden' name='v_id'     value='".$rec['v_id']."'/>".
		      "<input type='hidden' name='a_id'     value='".$rec['a_id']."'/>".
		      "<noscript><input type='submit' value='go'/></noscript>"),
		    'POST'));
    }else{    //======================================================================================	
      $names = array();
      foreach($cotenants as $lm_id=>$leases){
	foreach($leases as $lease_id){
	  $r = self::_leaseInfo($lease_id);
	  $names[] = $r['lease_name'];
	}
      }
      if (empty($names)) $names = array(b_fmt::redText('to be selected'));
      $reply = join(',',$names);
    }
    return $reply;
  }
}
