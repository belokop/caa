<?php
/*
 * "Abstract room" class, represents a bookable accomodation object (Hotel room, rented apartment, etc).
 *
 * Has the followins properties:
 * - Capacity (number of beds)
 * - First/Last dates when is available
 * - Price
 * - etc.
 * The roomss are grouped in a Building (Hut) 
 */

define('VMaps_day1',b_time::txt2unix('2008-03-14'));

locateAndInclude('bForm_vm');
class bForm_vm_Room extends bForm_vm {

  public static $defaultPrice = 1000; // Set to 100 SEK if the price is unknown

  var $TABLE  = 'abs_rooms';
  var $TABLEID= 'a_id';
  public static $receiptFormula = '';
  public $hut = Null; // The parent Hut class (if any)
  public $e   = Null; // The event class 
  
  function defineVariables() {
    $this->defineVariable(array('a_name'           => ($this->MODE=='RW'?'Nickname':' '),
				'a_phone'          => 'Phone',
				'a_status'         => 'Status',
				'a_comment'        => 'Comment',
				'a_hutid'          => 'Building',
				'a_area'           => 'Area (kvm)',
				'a_end'            => 'Available to',
				'a_price'          => 'Monthly rent',
				'a_price_cleaning' => 'Cleaning fee',
				'a_start'          => 'Available from',
				'a_capacity'       => 'Capacity (beds)',
				),True);
    
    foreach(array('a_name',
		  'a_price',
		  'a_capacity')  as $item)  $this->isVITAL[$item] = True;
    foreach(array('a_hutid',
		  'a_phone',
		  'a_capacity',
		  'a_area',
		  'a_comment',
		  'a_price_cleaning',
		  'a_start',
		  'a_end')    as $item)  $this->isMBE[$item] = True; // MBE - Might Be Empty
  }

  function __construct($id, $mode='RO') {
    if ($a_hutid = @$_GET['a_hutid']) $this->getValue_reset('a_hutid',$a_hutid);
    parent::__construct($id, $mode);

    if (!empty($_GET['e_id_once'])){
      // Catch the rent price update for a particular event
      $this->e = myPear::getInstance_new('bForm_vm_Event',$_GET['e_id_once']);
      $this->show_gv = True;
      if ($this->myPost_bForm()){
	$a_price_event = $_POST['a_price'];
	if ($a_price_event != $_SESSION['a_price_myOrg'][$this->ID]){
	  b_debug::xxx(sprintf("a_price %s --> $a_price_event",$_SESSION['a_price_myOrg'][$this->ID]));
	  // Put back the myOrg rent price since they are already auto-updated
	  $this->updateDB(array('a_price'=>$_SESSION['a_price_myOrg'][$this->ID]));
	  $this->getValue_reset('a_price', $_SESSION['a_price_myOrg'][$this->ID]);
	  // Update the rent price in accommodationOptions for this event
	  VM_accommodationOptions($this->e)->set_a_price($a_price_event,$this->ID,$this->getValue('a_hutid'));
	}
      }
      $this->show_gv = False;
    }
  }

  /*
   *
   */
  function readDefRecord(){
    $q = myPear_db()->query(sprintf("SELECT * FROM %s ".
				    " LEFT JOIN abs_huts ON a_hutid = hut_id ".
				    " WHERE %s = '%d'",
				    $this->TABLE,$this->TABLEID,$this->ID));
    while ($r=myPear_db()->next_record($q)) {
      $this->formDB = $r; 
      if ((int)$this->getValue('a_start',1,1) < VMaps_day1) $this->formDB['a_start'] = myPear_day0;
      if ((int)$this->getValue('a_end'  ,1,1) < VMaps_day1) $this->formDB['a_end']   = myPear_day9;
      if (!empty($r['a_hutid'])) $this->hut = myPear::getInstance_new('bForm_vm_Hut',$r['a_hutid'],'strict');
      // Save the default room price
      if (empty($_SESSION['a_price_myOrg'][$this->ID])){
	$_SESSION['a_price_myOrg'][$this->ID] = $this->formDB['a_price'];
	b_debug::xxx(sprintf('save a_price="%s"',$_SESSION['a_price_myOrg'][$this->ID]));
      }
    }
  }

  /*
   * True if the room is shared
   */
  public function isShared(){
    return self::_isShared($this->formDB);
  }

  public static function _isShared($rec=array()){
    if (!is_array($rec)) $rec = array('a_status' => $rec);
    $reply = (@$rec['a_status'] === AP_SHARABLE);
    return $reply;
  }

  /*
   * Compalsary fee per lease, i.e. cleaning
   */
  public function fee(){
    return $this->getValue('a_price_cleaning',True);
  }

  /*
   * Get the room price for "myOrg" and/or "event"
   */
  public function getValue($name, $asIs=True, $d_MBE=False){
    $reply = parent::getValue($name, $asIs, $d_MBE);
    switch($name){
    case 'a_price':
      if (!empty($this->e) && ($this->hut->getValue('hut_code') != HUT_BI)){
	if (cnf_dev) parent::getValue('a_name',1,1);
	$a_price = VM_accommodationOptions($this->e)->get_a_price($this->ID,parent::getValue('a_hutid',True));
	$this->dbg("myOrg rent='$reply', Event rent='$a_price'",cnf_dev);
	$reply = $a_price;
      }
    default:
    }
    return $reply;
  }

  public static function getReceipt($formDB=array(),$nDays){
    $a_id = @$formDB['a_id'];
    $extraApFee = 0;
    $e = (empty($formDB['e_id'])
	  ? Null
	  : myPear::getInstance_new('bForm_vm_Event',$formDB['e_id'],'strict'));
    if ($a_id == VM::$OA->ID){
      self::$receiptFormula  = '('.LODGING_OA_TXT.')';
      return self::_daily_a_price($formDB,$e);
    }else{
      //if ($ap = myPear::getInstance_new('bForm_vm_Room',$a_id,'strict')) $extraApFee = $ap->fee();
      $p = self::_daily_a_price($formDB,$e); 
      self::$receiptFormula = (($ip=(int)$p)?"($ip":'(???')."*$nDays".($extraApFee?"+$extraApFee)":')');
      return (int)($p * $nDays + $extraApFee);
    }
  }


  /*
   *    $p = array(24 => '3=1003,29=620,999=601',
   *               30 => '3=1110,29=730,999=619',
   *	           41 => '3=1503,29=1084,999=1003',
   *	           50 => '3=1503,29=1084,999=1003');
   */
  public static function _daily_a_price($formDB,$e_class){
    locateAndInclude('bForm_vm_Lease');
    
    $price = (empty($e_class)
	      ? @$formDB['a_price']
	      : VM_accommodationOptions($e_class)->get_a_price($formDB['a_id'],$formDB['hut_id']));
    
    switch(bForm_vm_Lease::_hut_code($formDB)){
    case LODGING_OA:  
      $reply = 0;
      break;
      
    default:
      if (strpos($price,'=') === False){
	if (empty($price)){
	  $price = self::$defaultPrice;
	  myPear::WARNING(sprintf("The monthly rent price for '%s' is not set, defaulted to %s",$formDB['a_name'],b_fmt::money($price)));
	}
	$reply = (int)($price * 12 / 365); // convert montly -> daily price
      }else{
	$ndays = b_time::diff($formDB['lease_end'],$formDB['lease_start']);
	if (empty($ndays)) b_debug::print_r($formDB);
	if (empty($ndays)) b_debug::traceBack('???');
	foreach(explode(',',$formDB['a_price']) as $item){
	  list($limit,$reply) = explode('=',$item);
	  if ($ndays <= $limit) break;
	}
      }
    }
    b_debug::xxx($price,array(2));
    return $reply;
  }

  function period(){
    $reply = '';
    if (($d=(int)$this->getValue('a_start',1)) > myPear_day0) $reply .= b_time::strftime('%e %h %Y ->',$d);
    if (($d=(int)$this->getValue('a_end',1))   < myPear_day9) $reply .= b_time::strftime('-> %e %h %Y',$d);
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

    myPear::H1($this->name());
    if ($this->e) print x('h3',$this->e->name());

    $this->getFormOpen();    
    if ((($visit=$this->parentForm) instanceof bForm_vm_Visit) && !$visit->exp_arePayed() && VM::hasRightTo('book')){
      $url = array('l'=>b_url::same("?function=vm_edit_accommodation&resetcache_once=1&v_id=$visit->ID"),
		   'd'=>btn_edit_accommodation,
		   );
    }else{
      $url = '';
    }

    if ($a_hutid = $this->getValue('a_hutid')) {
      $hut = new bForm_vm_hut($a_hutid);
      $this->isRO['a_hutid'] = True;
      $this->formBlock(' ', // Block name is "space" !!!! 
		       '',
		       array('a_hutid'=>$this->selectMenu('a_hutid',array($a_hutid=>$hut->name()))),
		       $url);
    }
    
    $this->formBlock('Accommodation',
		     ($this->embedded?'Accommodation':'Room'),
		     array('a_name',
			   'a_area',
			   'a_capacity',
			   'a_comment'),
		     $url);

    $this->formBlock('Price',
		     'Price',
		     array('a_price'), // array('a_price','a_price_cleaning'),
		     $url);
    
    if (!$this->embedded){
      $dialog = array();
      if ($this->MODE == 'RW' || $this->getValue('a_start',1) > myPear_day0) $dialog['a_start'] = 'datePickerField';
      if ($this->MODE == 'RW' || $this->getValue('a_end',1)   < myPear_day9) $dialog['a_end']   = 'datePickerField';
      $this->formBlock('Renting conditions',
		       'Renting conditions',
		       $dialog,
		       (empty($url)
			? ''
			: array('d'=>btn_edit_ap,
				'l'=>b_url::same("?mode=RW&form=vm_Room&id=".$this->ID))));
      if (stripos($this->getValue('a_name',1),'shared') !== False) $this->textHidden('a_status',AP_SHARABLE);
    }
    
    // Now the "submit" buttons
    $submit= array();
    if ($this->MODE=='RO') $submit['continue'] = b_url::getReferrer();
  // test... if (!empty($this->e))  $submit['reset to default'] = b_url::same('?reset_once=y');
    $this->closeForm('submit',$submit);
    if (!$this->embedded) bIcons()->explain();
  }
  
  /*
   * Check that the room is accesible for the given time period
   */
  static function _apIsAccessible($formDB, $start, $end){
    if (empty($formDB['a_start'])) $formDB['a_start'] = myPear_day0;
    if (empty($formDB['a_end']))   $formDB['a_end']   = myPear_day9;
    $available = 
      (b_time::_($start) >= b_time::_($formDB['a_start'])) &&
      (b_time::_($end)   <= b_time::_($formDB['a_end']));
    // myPear::DEBUG(($available ? '' : 'NOT ').'available for '.b_time::period($start,$end).': '.x('em',b_time::period($formDB['a_start'],$formDB['a_end']).' '.self::a_name($formDB)));
    return $available;
  }

  /*
   *
   */
  public static function a_name($formDB,$includeArea=False){
    $area = ($formDB['a_area']>9 && $formDB['a_area']<777) ? " ($formDB[a_area] sq.m.)" : '';
    $reply = (($hut_name=@$formDB['hut_name']) && ($formDB['hut_code']!=HUT_BI) ? "$hut_name, " : '') . $formDB['a_name'];
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

  function name($includeArea=False){
    return (myPear::is_int($this->ID)
	    ? self::a_name($this->formDB,$includeArea)
	    : 'Adding new room');
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
      locateAndInclude('bForm_vm_Lease');
      $this->isWritable = (bool)(VM_rooms_updater_here || parent::isWritable()) && (bForm_vm_Lease::_hut_code($this->formDB) !== LODGING_OA);
      $this->dbg($this->isWritable);
    }
    $this->dbg($this->isWritable,$this->debug||cnf_dev);
    return $this->isWritable;
  }
  
  function formOKspecific() {
    if ($this->getValue('a_area',True) < 2) $this->errorsInTheForm['a_area'] = 'Wrong value'; 
    $this->formDB_modified['a_area']  = (int)$this->getValue('a_area',True);
  }
}
