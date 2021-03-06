<?php
/*
 * "Abstract room" class, represents a book-able accommodation object (Hotel room, rented apartment, etc).
 *
 * Has the following properties:
 * - Capacity (number of beds)
 * - First/Last dates when is available
 * - Price
 * - etc.
 * The rooms are grouped in a Building (Hut) 
 */

define('VMaps_day1',b_time::txt2unix('2008-03-14'));

locateAndInclude('bForm_vm');
locateAndInclude('bList_vm_accommodationOptions');
class bForm_vm_Room extends bForm_vm {

  public static $defaultPrice = 1000; // Set to 100 SEK if the price is unknown

  var $TABLE  = 'abs_rooms';
  var $TABLEID= 'a_id';
  public static $receiptFormula = '';
  public $hut = Null; // The parent Hut class (if any)
  public $e   = Null; // The event class 
  
  
  function __construct($id, $mode='RO') {

    parent::__construct($id, $mode);

    // Sanity
    if ((int)$this->getValue('a_start',1,1) < VMaps_day1) $this->getValue_reset_forced('a_start',myPear_day0);
    if ((int)$this->getValue('a_end'  ,1,1) < VMaps_day1) $this->getValue_reset_forced('a_end',  myPear_day9);

    // Catch the arguments...
    if ($a_hutid = @$_GET['a_hutid']){
      $this->getValue_reset('a_hutid',$a_hutid);
    }
    
    // Save the default room price
    foreach(bList_vm_accommodationOptions::$whatIsSaved as $item){
      if (empty($_SESSION[$item.'_myOrg'][$this->ID])){
	if (empty($this->formDB[$item])) continue;
	$_SESSION[$item.'_myOrg'][$this->ID] = $this->formDB[$item];
	$this->dbg(sprintf('save cache %s_myOrg="%s"',$item,$_SESSION[$item.'_myOrg'][$this->ID]),cnf_dev);
      }
    }
    
    // Hook the hut
    $this->hookHut();
    
    if ((bool)VM::_MENU()){
      if (VM::_MENU()->context() == 'org'){
	foreach(bList_vm_accommodationOptions::$whatIsSaved as $item){
	  $value = parent::getValue($item,1,1);
	  if (empty($value)) continue;
	  if ($_SESSION[$item.'_myOrg'][$this->ID] != $value){
	    b_debug::xxx(sprintf("update cache %s: %s --> %s",$item,$_SESSION[$item.'_myOrg'][$this->ID],$value),True);
	    $_SESSION[$item.'_myOrg'][$this->ID] = $value;
	  }
	}
      }elseif (VM::_MENU()->context() == 'event'){
	VM::e_ID();
	$this->e = VM::$e; 
	if (empty($this->e)) b_debug::internalError("empty event, but \"event\" context");
	// Catch the updates for the parent event
	if ($this->myPost_bForm()){
	  foreach(bList_vm_accommodationOptions::$whatIsSaved as $item){
	    $item_event = $_POST[$item];
	    $item_org = parent::getValue($item,1,1);
	    if ($item_org != $_SESSION[$item.'_myOrg'][$this->ID]){
	      b_debug::xxx(sprintf("Unfortunately was %s reset %s --> $item_org",$item,$_SESSION[$item.'_myOrg'][$this->ID]));
	      // Put back the myOrg rent price since they are already auto-updated
	      $this->updateDB(array($item=>$_SESSION[$item.'_myOrg'][$this->ID]));
	      $this->getValue_reset($item, $_SESSION[$item.'_myOrg'][$this->ID]);
	    }
	    // Update the rent price in accommodationOptions for this event
	    VM_accommodationOptions($this->e)->set_a_value($item_event,$this->ID,$this->hut->ID,$item);
	  }
	}
      }
    }
    $this->dbg('done');
  }

    // Hook the hut
  private function hookHut(){
    if (empty($this->hut) && $a_hutid=parent::getValue('a_hutid',1,1)){
      $this->hut = loader::getInstance_new('bForm_vm_Hut',$a_hutid,array('strict','fatal'));
      $this->dbg('hook '.$this->hut->__toString());
    }
    return $this->hut;
  }

  /*
   *
   */
  function defineVariables() {
    $this->defineVariable(array('a_name'           => ($this->MODE=='RW'?'Nickname':' '),
				'a_phone'          => 'Phone',
				'a_status'         => 'Status',
				'a_comment'        => 'Comment',
				'a_hutid'          => 'Building',
				'a_area'           => 'Area (kvm)',
				'a_end'            => 'Available to',
				'a_price'          => 'Monthly rent',
				'a_start'          => 'Available from',
				'a_capacity'       => 'Capacity (beds)',
				'a_quantity'       => 'N rooms available',
				),True);
    
    $this->defineTypes(array('isVITAL'  =>array('a_name',
						'a_price',
						'a_capacity'),
                             'isMBE'    =>array('a_hutid',
						'a_phone',
						'a_area',
						'a_comment',
						'a_start',
						'a_end'),
			     'isRO'     =>array('a_name'    => (bool)$this->getValue('a_name') && (bool)$this->getValue('a_capacity'),
						'a_capacity'=> (bool)$this->getValue('a_name') && (bool)$this->getValue('a_capacity')
						)));
  }
  
  /*
   *
   */
  function readDefRecord(){
    $q = myPear_db()->query($sql=sprintf("SELECT * FROM %s ".
					 " LEFT JOIN abs_huts ON a_hutid = hut_id ".
					 " WHERE %s = '%d'",
					 $this->TABLE,$this->TABLEID,$this->ID));
    while ($r=myPear_db()->next_record($q)) $this->formDB = $r; 
    $this->dbg($sql);
  }

  /*
   * True if the room is sharable
   */
  public function isSharable($lease=Null){
    return self::_isSharable(array('a_name'    =>$this->getValue('a_name',1,1),
				   'a_capacity'=>$this->getValue('a_capacity',1,1),
				   'hut_name'  =>$this->hut->getValue('hut_name',1,1)), 
			     $lease
			     );
  }
  
  /*
   *
   */
  public static function _mightBeSharable($rec){
    // Sanity...
    if (!empty($rec['lease_aid'])){
      if (empty($rec['hut_code']))   b_debug::internalError('??? hut_code',$rec);
      if (empty($rec['a_capacity'])) b_debug::internalError('??? a_capacity',$rec);
    }
    $reply = (empty($rec['lease_aid'])
	      ? False
	      : (($rec['a_capacity'] > 1) && ($rec['hut_code'] !== HUT_BI)));
    return $reply;
  }
  
  /*
   *
   */
  public static function _isSharable($rec,$lease=Null,$depth=0){
    
    if (!self::_mightBeSharable($rec)){
      $reply = False;
    }else{
      if ((empty($rec['lease_start']) || empty($rec['lease_end']) || empty($rec['lease_aid'])) && !empty($lease)){
	if (is_numeric($lease)) $lease = loader::getInstance_new('bForm_vm_Lease',$lease);
	foreach(array('lease_start','lease_end','lease_aid') as $f) $rec[$f] = $lease->getValue($f,1,1);
	if (empty($rec['lease_id'])) $rec['lease_id'] = $lease->ID;
      }
      
      if (empty($rec['lease_id'])) b_debug::traceBack("???".__method__."() - empty lease_id !!!");
      if (empty($rec['lease_id'])) b_debug::internalError("???".__method__."() - empty lease_id !!!",$rec);

      if (!empty($rec['lease_id']) && 
	  bForm_vm_Lease::_room_is_full($rec['lease_id'])){
	$reply = False;
      }else{
	$VM_cotenants = VM_cotenants()->get_cotenants($rec['lease_id'],True);
	foreach($VM_cotenants as $lm_id=>$leases){
	  if (count(array_values($leases)) >= $rec['a_capacity']){
	    bForm_vm_Lease::set_lease_options($rec['lease_id'],array(VM_room_is_full=>1));
	    $reply = False;
	  }
	}
	if (!isset($reply)){
	  // Must be sharable for all the cotenants...
	  $reply = True;		 
	  foreach($VM_cotenants as $lm_id=>$leases){
	    foreach($leases as $l){
	      if ($l == $rec['lease_id']) continue;
	      if (!empty($depth))         continue;
	      $rec['lease_id'] = $l;
	      if (!self::_isSharable($rec,Null,1)){
		$reply = False;
		break;
	      }
	    } 
	  }
	}
      }
    }
    if (empty($depth)){
      $id = '';
      if (!empty($rec['lease_id']))    $id = $rec['lease_id'];
      if (!empty($rec['av_lastname'])) $id = $rec['av_lastname'];
      if (!empty($rec['a_name'])) $id .= " $rec[a_name]"; 
      if (empty($id)) $id = '???';
      b_debug::_debug(var_export($reply,True),cnf_dev);
    }
    return $reply;
  }

  /*
   * Compalsary fee per lease, i.e. cleaning
   */
    public function fee(){
    return 0;
  }

  /*
   * Get the room price for "myOrg" and/or "event"
   */
  public function getValue($name, $asIs=True, $d_MBE=True){
    $reply = parent::getValue($name, $asIs, $d_MBE);
    if ((bool)VM::_MENU()){
      // Get the value specific for the event, if any
      if (VM::_MENU()->context() == 'event'){
	$this->hookHut();
	if (($this->hut->getValue('hut_code') != HUT_BI) &&
	    in_array($name,bList_vm_accommodationOptions::$whatIsSaved)){ // a_quantity & a_price
	  if (empty($this->e)) $this->e = VM::$e;
	  $value = VM_accommodationOptions($this->e)->get_a_value($this->ID,$this->hut->ID,$name);
	  if ($reply != $value) $this->dbg(sprintf("myOrg $name='%s', Event $name='%s'",var_export($reply,True),var_export($value,True)),cnf_dev);
	  $reply = $value;
	}
      }
    }
    return $reply;
  }

  public static function _getReceipt($formDB=array(),$nDays){
    $a_id = @$formDB['a_id'];
    $extraApFee = 0;
    b_debug::_debug("nDays = $nDays",cnf_dev);
    $e = (empty($formDB['v_eid'])
	  ? Null
	  : loader::getInstance_new('bForm_vm_Event',$formDB['v_eid'],array('strict','fatal')));
    if ($a_id == VM::$OA->ID){
      self::$receiptFormula  = '('.LODGING_OA_TXT.')';
      return 0;
    }elseif($a_id == VM::$BR->ID){
      self::$receiptFormula  = '('.LODGING_BR_TXT.')';
      return 0;
    }else{
      $p = self::_daily_price($formDB,$e); 
      self::$receiptFormula = (($ip=(int)$p)?"($ip":'(???')."*$nDays".($extraApFee?"+$extraApFee)":')');
      return (int)($p * $nDays + $extraApFee);
    }
  }


  /*
   * Get the daily rent to be paid by the Organization for acconmodating an Avatar 
   * (the members of family to be paid by the tenant)
   */
  public static function _daily_price($rec,$e_class=Null){
    
    if (VM_self_accommodated($rec)){

      $reply = 0;

    }else{

      // Workaround
      if (empty($rec['lease_aid'])) return 0;

      // Who pays?
      if (empty($rec['hut_id'])) b_debug::traceBack("??? empty hut_id");
      if (empty($rec['hut_id'])) b_debug::internalError("??? empty hut_id",$rec);
      if (empty($rec['a_price']))b_debug::internalError("??? empty a_price",$rec);

      locateAndInclude('bForm_vm_Lease');
      $get_pay = bForm_vm_Lease::get_lease_options($rec,'get_pay',array(),True);
      $cotenants = VM_cotenants()->get_cotenants($rec['lease_id'],True,'info');
      b_debug::_debug(array('cotenants'=>$cotenants,
			    'get_pay'  =>$get_pay),cnf_dev);
      if ($get_pay === VM_pay_everything){
	$reply = self::_parse_room_price($rec,$e_class,$cotenants);
      }elseif ($get_pay === VM_pay_nothing){ // nothing
	$reply = 0;
      }else{
	// If "nordita pays for 1/2/3/..." find the price of the studio of this size
	$pay_for = (empty($get_pay)
		    ? count($cotenants)
		    : count($get_pay));
	if (empty($pay_for)) {
	  //	  b_debug::internalError("No cotenants,but pay_for=".var_export($get_pay));
	  MSG::WARNING_T("No cotenants,but get_pay=".var_export($get_pay,True));
	}
	b_debug::_debug("We pay for $pay_for, look for the appropriate room",cnf_dev);
	$q = myPear_db()->qquery("SELECT a_id,a_name,a_price,a_capacity FROM abs_rooms WHERE a_hutid=$rec[hut_id] AND a_capacity = $pay_for",cnf_dev);
	while ($r=myPear_db()->next_record($q)) {
	  $rec['a_id'] = $rec['lease_aid'] = $r['a_id'];
	  $rec['a_price'] = $r['a_price'];
	  $r['hut_code'] = $rec['hut_code'];
	  b_debug::_debug("Reference room - ".bForm_vm_Room::_a_name($r),cnf_dev);
	}
	$reply = self::_parse_room_price($rec,$e_class,$cotenants);
      }
    }
    if (!empty($cotenants)) $reply = $reply / count($cotenants);
    b_debug::_debug($reply,cnf_dev);
    return $reply;
  }

  /*
   *
   */
  public static function _parse_room_price($rec,$e_class,$cotenants=array()){
    if (!$rec['a_id'] || !$rec['hut_id']) return Null;

    $price_basic = (empty($e_class)
		    ? $rec['a_price']
		    : VM_accommodationOptions($e_class)->get_a_value($rec['a_id'],$rec['hut_id'],'a_price'));

    $ndays = b_time::diff($rec['lease_end'],$rec['lease_start']);
    if (empty($ndays)){
      MSG::ERROR('Accommodation requested for a visit for ZERO days...');
      return 0;
    }
    
    if (strpos($price_basic,'=') === False){
      $reply = (int)($price_basic * 12 / 365); // convert monthly -> daily price
    }else{
      $extra_charge_for_second_tenant = 0;
      if (substr_count($price_basic,'=') !== substr_count($price_basic,',')+1){
	// Check syntax
	MSG::ERROR("Wrong rent price \"$price_basic\"");
	$reply = (int)($price_basic * 12 / 365); // convert monthly -> daily price
      }else{
	$price_matrix = array();
	foreach(explode(',',str_replace(' ','',$price_basic)) as $item){
	  list($limit,$value) = explode('=',$item);
	  if (is_numeric($limit)) {
	    if ($ndays <= $limit) $price_matrix[sprintf("%03d",$limit)] = $value;
	  }elseif ($limit == 'cot'){
	    $extra_charge_for_second_tenant = $value;
	  }
	}
	ksort($price_matrix);
	$reply = array_values($price_matrix);
	$reply = $reply[0];
      }
      if (count($cotenants) >= 2)  $reply += $extra_charge_for_second_tenant;
    }
    b_debug::_debug("ndays=$ndays, room daily price: $price_basic --> $reply",True);
    return $reply;
  }
  
  function period(){
    $reply = '';
    if (($d=(int)$this->getValue('a_start')) > myPear_day0) $reply .= b_time::strftime('%e %h %Y ->',$d);
    if (($d=(int)$this->getValue('a_end'))   < myPear_day9) $reply .= b_time::strftime('-> %e %h %Y',$d);
    return str_replace('->->','->',$reply);
  }

  protected function bForm_action($myGet=array(),$drop=array()){
    if (!empty($this->e)) $myGet['e_id_once'] = $this->e->ID;
    $reply = parent::bForm_action($myGet,$drop);
    $this->dbg($reply,True);
    return $reply;
  }

  /*
   *
   */
  function getForm(){
    
    bIcons()->toggleExplainIcons(False);
    if (strpos($this->getValue('a_price'),'=') !== False) $this->defineVariable('Daily rent formula','a_price');

    MSG::H1($this->name());
    if ((bool)$this->e)      print x('h3',$this->e->name());

    $this->getFormOpen();    
    if ((($visit=$this->parentForm) instanceof bForm_vm_Visit) && !$visit->exp_arePaid() && VM::hasRightTo('book')){
      $url = array('l'=>b_url::same("?function=vm_edit_accommodation&resetcache_once=1&v_id=$visit->ID"),
		   'd'=>btn_edit_accommodation,
		   );
    }else{
      $url = '';
    }

    $this->isRO['a_hutid'] = True;
    $this->formBlock(' ', // Block name is "space" !!!! 
		     '',
		     array('a_hutid'=>$this->selectMenu('a_hutid',array($this->getValue('a_hutid')=>$this->hut->name()))),
		     $url);
    
    $this->formBlock('Accommodation',
		     ($this->embedded?'Accommodation':'Room'),
		     array('a_name',
			   'a_area',
			   'a_capacity',
			   'a_comment'),
		     $url);

    $this->formBlock('Price',
		     'Price',
		     array('a_price'),
		     $url);

    $this->show_gv = True;
    if (($this->MODE == 'RO') && 
	($this->getValue('a_quantity')==VM_quantity2request)){
      $this->getValue_reset('a_quantity',VM_quantity2request_text.' before setting a value');
    }
    $this->formBlock('Availability',
		     'Availability',
		     array('a_quantity'),
		     $url);
    
    if (!$this->embedded){
      $dialog = array();
      if ($this->MODE == 'RW' || $this->getValue('a_start') > myPear_day0) $dialog['a_start'] = 'datePickerField';
      if ($this->MODE == 'RW' || $this->getValue('a_end')   < myPear_day9) $dialog['a_end']   = 'datePickerField';
      $this->formBlock('Renting conditions',
		       'Renting conditions',
		       $dialog,
		       (empty($url)
			? ''
			: array('d'=>btn_edit_ap,
				'l'=>b_url::same("?mode=RW&form=vm_Room&id=".$this->ID))));
    }
    
    // Now the "submit" buttons
    $submit= array();
    if ($this->MODE=='RO'){
      // $submit['continue'] = b_url::getReferrer();
      $submit['see all rooms in '.$this->hut->name()] = b_url::same('?function=bHolder_vm_Rooms::list_hut&arg1_once='.$this->hut->ID);
    }
    // test... if (!empty($this->e))  $submit['reset to default'] = b_url::same('?reset_once=y');
    $this->closeForm('submit',$submit);
    if (!$this->embedded) bIcons()->explain();
  }
  
  /*
   * Check that the room is accessible for the given time period
   */
  static function _apIsAccessible($formDB, $start, $end){
    if (empty($formDB['a_start'])) $formDB['a_start'] = myPear_day0;
    if (empty($formDB['a_end']))   $formDB['a_end']   = myPear_day9;
    $available = 
      (b_time::_($start) >= b_time::_($formDB['a_start'])) &&
      (b_time::_($end)   <= b_time::_($formDB['a_end']));
    return $available;
  }

  /*
   * Room identification
   */
  public static function room_id($r){
    $id = $r['a_id'] .'#'. self::_a_name($r,True);
    return $id;
  }

  public function name($includeHut=False){
    $this->hookHut();
    return (b_posix::is_int($this->ID)
	    ? self::_a_name(array('a_name'    =>$this->getValue('a_name'),
				  'a_capacity'=>$this->getValue('a_capacity'),
				  'hut_code'  =>$this->hut->getValue('hut_code'),
				  'hut_name'  =>$this->hut->getValue('hut_name'),
				  ),
			    $includeHut)
	    : 'Adding new room');
  }

  public static function _a_name($rec,$includeHut=False,$includePay=False,$includeCapacity=True,$includeAname=True){
    
    $for_applicant = ($includeHut === 'for_applicant');
    
    if (($for_applicant && ($rec['hut_code'] === HUT_BI)) ||
	(@$rec['lease_id'] && empty($rec['lease_aid'])) || 
	(@$rec['v_status'] && ($rec['v_status'] != STATUS_YES))){
      $reply = '';
    }else{
      if (empty($rec['hut_code'])) b_debug::internalError("no hut_code",$rec);
      if ($rec['hut_code'] === HUT_BI){
	$includeCapacity = False;
	$includeAname = False;
	$includeHut = True;
	$rec['hut_name'] = $rec['a_name'];
      }elseif ($for_applicant){
	$includeHut = False;
      }elseif (empty($rec['hut_name'])){
	$includeHut = False;
      }
      $reply = sprintf('%s%s%s',
		       ($includeCapacity 
			? ($for_applicant 
			   ? 'Room with '.$rec['a_capacity'].' bed'.($rec['a_capacity']>1?'s':'')
			   : '&#91;'.$rec['a_capacity'].'&#93; ')
			: ''),
		       ($includeHut ? $rec['hut_name'].($includeAname?'.':'') : ''), 
		       ($includeAname&&!$includePay  
			? ($for_applicant 
			   ? ((strToLower($rec['a_name'])=='bed' ? '' : ' '.x('(',$rec['a_name'])).
			      ', '.b_time::period($rec['lease_start'],$rec['lease_end']))
			   : $rec['a_name'])
			: '')
		       );
      if ($includePay || $for_applicant){
	if (empty($rec['lease_id'])) b_debug::internalError("includePay requested, but no lease_id",$rec);
	$pay = bForm_vm_Lease::get_lease_options($rec,
						 ($for_applicant
						  ? 'pay_room_for_applicant'
						  : 'pay_room'),
						 array(),
						 True);
	if (!empty($pay)) $reply .= ', '.x('em',$pay);
      }
    }

    $reply = ($for_applicant
	      ? b_fmt::unescape(strip_tags($reply))
	      : $reply);
    if ($for_applicant) b_debug::_debug($reply,True);
    return $reply;
  }
  
  /*
   * get the hut for the apart
   */
  public static function a_hutid($a_id){
    $reply = Null;
    $q  =  myPear_db()->query("SELECT a_hutid FROM abs_rooms WHERE a_id = $a_id");
    while ($r=myPear_db()->next_record($q)) {
      $reply = $r['a_hutid'];
    }
    return $reply;
  }
  
  function isUsed(){
    $q  =  myPear_db()->query("SELECT * FROM abs_leases WHERE lease_aid = '$this->ID'");
    return myPear_db()->num_rows($q) > 0;
  }
  
  function isReadable(){
    return True;
  }

  private $isWritable = Null;
  function isWritable() { 
    if ($this->isWritable === Null){
      VM_access();
      locateAndInclude('bForm_vm_Lease');
      $this->isWritable = ((VM_rooms_updater_here || parent::isWritable()) && 
			   VM_hutCodes()->isWritable($this->hut->getValue('hut_code')) &&
			   !in_array(bForm_vm_Lease::_hut_code($this->formDB),array(LODGING_OA)));
    }
    $this->dbg($this->isWritable,cnf_dev);
    return $this->isWritable;
  }
  
  function formOKspecific() {
    if ($this->getValue('a_area',True) < 2) $this->errorsInTheForm['a_area'] = 'Wrong value'; 
    $this->formDB_modified['a_area']  = (int)$this->getValue('a_area',True);
  }
}
