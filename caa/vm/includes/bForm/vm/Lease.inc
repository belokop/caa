<?php
locateAndInclude('bForm_vm');
class bForm_vm_Lease extends bForm_vm{

  private static $isReady = False;
  
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
   * Remove LODGING_REQ_ONLY requests, it was a bad idea
   */
  public static function cancel_LODGING_REQ_ONLY(){
    $q = myPear_db()->qquery(sprintf("SELECT lease_id FROM abs_leases WHERE lease_aid = '%s'",VM::$REQ_ONLY->ID),cnf_dev);
    while($r = myPear_db()->next_record($q)){
      $l = new bForm_vm_Lease($r['lease_id']);
      b_debug::xxx($l->__toString());
      $l->delete();
    }
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
    $q = myPear_db()->query("SELECT lease_id,hut_code,a_id,a_code FROM abs_leases ".
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

    $w = array("a_id = $a_id",
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
    $reply = array();
    while($r = myPear_db()->next_record($q)) $reply[$r['lease_id']] = bForm_Avatar::_fmtName('Lf',$r);
    asort($reply);
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
    if ((@$_POST['_post_bForm_ID_once'] == $this->ID) && isset($_POST['lease_aid'])){
      
      // assign apartment to the lease
      $lease_aid = $_POST['lease_aid'];
      $this->hookRoom($lease_aid,$verbose=True);
      
      // assign cotenant(s) to the lease
      if(($a = myPear::getInstance_new('bForm_vm_Room',$lease_aid,'strict')) && $a->isShared()){
	$cotenants = array();
	foreach(preg_grep('/cotenant\d+$/',array_keys($_POST)) as $k){
	  if (empty($cotenants))	  $cotenants = array($this->ID);
	  $cotenants[] = $_POST[$k];
	  unset($_POST[$k]);
	  unset($_REQUEST[$k]);
	}
	VM_cotenants()->set_cotenants($cotenants);
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
			    " LEFT JOIN abs_rooms ON a_id=lease_aid ".
			    " LEFT JOIN abs_huts   ON a_hutid=hut_id ".
			    " WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q))   $this->formDB = $r;
  }

  /*
   * Get the apartment type
   */
  function hut_code(){
    return self::_hut_code(array('a_id'    => $this->getValue('a_id',True,True),
				 'a_code'  => $this->getValue('a_code',True,True),
				 'hut_code'=> $this->getValue('hut_code',True,True),
				 ));
  }
  
  /*
   * Hut code for the "internal accommodation types" is 
   * defined by the apartment "a_code" field (own accommodation, break in the visit, etc.)
   */
  static function _hut_code($buffer){
    $reply = Null;
    if (@$buffer['a_id']){
      $hut_code = @$buffer['hut_code'];
      if ($hut_code === HUT_BI)     $reply = $buffer['a_code'];
      elseif(!empty($hut_code))     $reply = $hut_code;
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
      ? bForm_vm_Room::getReceipt($this->formDB,$this->nDays())
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
					     b_url::same("?action_once=".btn_splitLease."&function=vm_edit_accommodation&resetcache_once=1&lease_id=$this->ID&v_id=".
							 $this->parentForm->ID),
					     $confirm=True,
					     '');
	
      }
      if ($nLeases > 1){
	$block_header[] = b_btn::submit_icon('i-drop',
					     btn_deleteLease,
					     b_url::same("?action_once=".btn_deleteLease."&function=vm_edit_accommodation&resetcache_once=1&lease_id=$this->ID&v_id=".$this->parentForm->ID),
					     $confirm=True,
					     '');
      }
    }
    
    // Cancel the accommodation menu if the event is in the past 
    $dialog = array('lease_id' => array('textHidden'));
    if ((time() - AUTO_EXPIRATION*86400) > $this->parentForm->getValue('v_end',1)) $this->isRO['lease_aid'] = !superUser_here;

    if ($this->isRO('lease_aid') && (!$this->hasAccommodation() || ($this->getValue('lease_aid') == 0))){ 
      $this->getValue_reset('lease_aid',b_fmt::redText('No accommodation'));
      $dialog['lease_aid'] = array('textField');
    }else{
      $dialog['lease_aid'] = array('selectMenu',
				   $this->freeRooms(),
				   "onchange='submit()'");
    }
    
    if (bForm_vm_Room::_isShared($this->formDB)){
      if ($this->MODE == 'RO' && !VM_cotenants()->get_cotenants($this->ID)) $this->MODE = 'RW';
      $cotenant = self::select_cotenant('_cotenant',$this->formDB,($this->MODE == 'RW'));
      if (!empty($cotenant)) $dialog[b_rawData] = array('Cotenant',$cotenant);
    }

    if ($this->isRO('lease_start') && $this->isRO('lease_end')){
      if (($lease_start=$this->getValue('lease_start')) && ($lease_end=$this->getValue('lease_end'))){
	$this->defineVariable(b_time::period($lease_start,$lease_end),'lease_aid');
      }
      
      if ($this->parentForm->isVisitType_program() && ($_virt_wish=$this->parentForm->getValue('v_comment',True))){
	$this->defineVariable(array('_virt_wish'=>' '),True);
	$this->isRO['_virt_wish'] = True;
	$this->formDB['_virt_wish'] = b_fmt::redText($_virt_wish,'greyText');;
	$dialog['_virt_wish'] = array('textField');
      }
    }else{
      $minmax = array($this->parentForm->getValue('v_start'),
		      $this->parentForm->getValue('v_end'));
      $dialog['lease_start'] = array('datePickerField',$minmax);
      $dialog['lease_end']   = array('datePickerField',$minmax);
    }
    
    // print the block
    if ($this->parentForm->isApproved()) $this->formBlock(VM_visit_lease.$this->ID,
							  $block_header,
							  $dialog,
							  array('l'=>b_url::same("?function=vm_edit_accommodation&resetcache_once=1&lease_id=$this->ID&v_id=".
										 $this->parentForm->ID),
								'i'=>'i-hotel',
								'd'=>btn_edit_accommodation));
    $this->dbg('------------end');
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
    if (in_array($hut_code,array_merge(array(HUT_BI),array_values($this->parentForm->e->accommodationOptions()->get_BI())))){
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
			($lease_aid == VM::$REQ_ONLY->ID))) $reply = False;
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
      VM_cotenants()->deleteMember($lm_id);
    }
  }

  function hookRoom($lease_aid,$verbose=True){
    if (cnf_dev) $verbose = True;

    if ($lease_aid === LODGING_RM){
      b_debug::xxx('REMOVE',array(2,$this->debug,'M'));

      // clean the cotenants list (if any) since the apartment is changed  
      $this->clean_cotenants();
      if ($verbose) myPear::MESSAGE_add($this->_periodToString('Remove accommodation for'));
      $this->updateDB(array('lease_aid'=>0));
      $this->hookVisit();
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
  public function freeRooms($lease_start=Null,$lease_end=Null){
    if (empty($lease_start))  $lease_start = $this->getValue('lease_start'); 
    if (empty($lease_end))    $lease_end   = $this->getValue('lease_end'); 
    return self::_freeRooms((is_object($this->parentForm->e)
			     ? $this->parentForm->e->accommodationOptions()
			     : bList::getListInstance(myOrg_ID,'bList_vm_accommodationOptions')),
			    $lease_start,
			    $lease_end,
			    $this->hasAccommodation(False),
			    $this);
  }
  
  /*
   *
   */
  public static function _freeRooms($accommodationOptions,$v_start,$v_end, $hasAccommodation=False,$lease=Null,$hut_code=Null){

    $reply = array();
    $where = array(0);
    if (empty($accommodationOptions)){
      //
      // Check ALL available rooms
      //
      locateAndInclude('bList_vm_accommodationOptions');
      foreach(bList_vm_accommodationOptions::get_allHuts() as $hut_id=>$hut_name){
	$where[] = 'hut_id = '.$hut_id;
      }
    }else{    
      //
      // Use only those "huts" which are selected by the administration, i.e. listed in vm_accommodationOptions
      //
      foreach($accommodationOptions->getMembers() as $x=>$r){
	$OK = (empty($r['nrooms'])
	       ? False
	     : (count($accommodationOptions->get_occupiedRooms($r['lm_key'],$v_start,$v_end)) < $r['nrooms']));
	if ($hasAccommodation && is_object($lease) && ($r['lm_key'] == bForm_vm_Room::a_hutid($lease->getValue('lease_aid')))) $OK = True;
	if ($OK) $where[] = 'hut_id = '.$r['lm_key'];
      }

      // array_merge sucks...
      if (empty($hut_code)){
	if ($hasAccommodation) $reply[LODGING_RM] = LODGING_RM_TXT;
	foreach($accommodationOptions->get_BI() as $k=>$v) $reply[$k] = $v;
      }
    }
    
    // Read the DB
    $q = myPear_db()->query("SELECT a_start,a_end,a_area,a_id,a_name,hut_code,hut_name FROM abs_rooms ".
			    " LEFT JOIN abs_huts ON a_hutid = hut_id ".
			    " WHERE ".join(' OR ',$where).
			    " ORDER BY hut_code,hut_name,a_name");
    while ($ra = myPear_db()->next_record($q)){
      if (!empty($hut_code) && ($ra['hut_code'] != $hut_code)) continue;
      if (bForm_vm_Room::_apIsAccessible($ra,$v_start,$v_end)) $reply[$ra['a_id']] = bForm_vm_Room::a_name($ra);
    }
    return $reply;
  }

  /*
   *
   */
  public static function LODGING_REQ_ONLY($e_id=Null){
    if (empty($e_id) && !empty(VM::$e)) $e_id = VM::$e->ID;
    $q = myPear_db()->query("SELECT v_id FROM abs_visits ".
			    " LEFT JOIN abs_leases     ON abs_leases.lease_vid = abs_visits.v_id ".
			    " LEFT JOIN abs_rooms ON abs_leases.lease_aid = abs_rooms.a_id ".
			    " WHERE v_eid = '$e_id' AND a_name = '".LODGING_REQ_ONLY_TXT."' GROUP BY v_id");
    $reply = array();
    while ($r = myPear_db()->next_record($q)) $reply[] = $r['v_id'];
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
  public static function select_cotenant($name,$rec,$mode='auto'){
    locateAndInclude('bForm_vm_Room');

    if (!bForm_vm_Room::_isShared($rec) || 
	($rec['lease_end'] < time()+24*3600)) return '';
    
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
