<?php
/*
 * "Abstract apartmemnt" class, represents a bookable accomodation object (Hotel room, rented apartment, etc).
 *
 * Has the followins properties:
 * - Capacity (number of beds)
 * - First/Last dates when is available
 * - Price
 * - etc.
 * The apartments are grouped in a Building (Hut) 
 */

define('VMaps_day1',b_time::txt2unix('2008-03-14'));

locateAndInclude('bForm_vm');
class bForm_vm_Apartment extends bForm_vm {

  var $TABLE  = 'abs_apartments';
  var $TABLEID= 'a_id';
  public static $receiptFormula = '';
  public $hut = Null; // The parent Hut class (if any)
  
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
		  'a_capacity',
		  'a_area',)  as $item)  $this->isVITAL[$item] = True;
    foreach(array('a_hutid',
		  'a_phone',
		  'a_capacity',
		  'a_comment',
		  'a_price_cleaning',
		  'a_start',
		  'a_end')    as $item)  $this->isMBE[$item] = True;
    $this->isMBE['a_capacity'] = True;
  }

  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }

  /*
   *
   */
  function readDefRecord(){
    $q = myPear_db()->query("SELECT * FROM $this->TABLE ".
			      " LEFT JOIN abs_huts ON a_hutid = hut_id ".
			      " WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q)) {
      $this->formDB = $r; 
      if ((int)$this->getValue('a_start',1,1) < VMaps_day1) $this->formDB['a_start'] = rent0;
      if ((int)$this->getValue('a_end'  ,1,1) < VMaps_day1) $this->formDB['a_end']   = rent9;
      if (!empty($r['a_hutid'])) $this->hut = myPear::getInstance_new('bForm_vm_Hut',$r['a_hutid'],'strict');
    }
  }

  /*
   * True if the apartment is shared
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

  public static function getReceipt($formDB=array(),$nDays){
    $a_id = @$formDB['a_id'];
    $extraApFee = 0;
    if ($a_id == VM::$OA->ID){
      self::$receiptFormula  = '('.LODGING_OA_TXT.')';
      return self::_dailyRent($formDB);
    }else{
      //if ($ap = myPear::getInstance_new('bForm_vm_Apartment',$a_id,'strict')) $extraApFee = $ap->fee();
      $p =  self::_dailyRent($formDB); 
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
  public static function _dailyRent($formDB=array()){
    
    $price = @$formDB['a_price'];
    
    locateAndInclude('bForm_vm_Lease');
    switch(bForm_vm_Lease::_hut_code($formDB)){
    case LODGING_OA:  
      $reply = 0;
      break;
      
    default:
      if (strpos($price,'=') === False){
	if (empty($price)){
	  $price = @$defaultPrice;
	  myPear::WARNING("The monthly rent price for '$formDB[a_name]' is not set, defaulted to ".b_fmt::money($price));
	}
	$reply = (int)($price * 12 / 365); // convert mantly -> daily price
      }else{
	$ndays = b_time::diff($formDB['lease_end'],$formDB['lease_start']);
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
    if (($d=(int)$this->getValue('a_start',1)) > rent0) $reply .= b_time::strftime('%e %h %Y ->',$d);
    if (($d=(int)$this->getValue('a_end',1))   < rent9) $reply .= b_time::strftime('-> %e %h %Y',$d);
    return str_replace('->->','->',$reply);
  }

  /*
   *
   */
  function getForm(){
    
    bIcons()->toggleExplainIcons(False);
    if (strpos($this->getValue('a_price',1,1),'=') !== False) $this->defineVariable('Daily rent formula','a_price');
    myPear::H1($this->name());

    $this->getFormOpen();    
    if ((($visit=$this->parentForm) instanceof bForm_vm_Visit) && !$visit->exp_arePayed() && VM::hasRightTo('book')){
      $url = array('l'=>b_url::same("?function=vm_edit_accommodation&resetcache_once=1&v_id=$visit->ID"),
		   'd'=>btn_edit_accommodation,
		   // 'i'=>'i-hotel'
		   );
    }else{
      $url = '';
    }
    $this->formBlock('Accommodation',
		     ($this->embedded?'Accommodation':' '),
		     array('a_name',
			   'a_area',
			   'a_capacity',
			   'a_comment'),
		     $url);

    $this->formBlock('Price',
		     'Price',
		     (True ? array('a_price') : array('a_price','a_price_cleaning')),
		     $url);
    
    if (!$this->embedded){
      $dialog = array();
      if ($this->MODE == 'RW' || $this->getValue('a_start',1) > rent0) $dialog['a_start'] = 'datePickerField';
      if ($this->MODE == 'RW' || $this->getValue('a_end',1)   < rent9) $dialog['a_end']   = 'datePickerField';
      $this->formBlock('Renting conditions',
		       'Renting conditions',
		       $dialog,
		       array('d'=>btn_edit_ap,
			     'l'=>b_url::same("?mode=RW&form=vm_Apartment&id=".$this->ID)));
      if (stripos($this->getValue('a_name',1),'shared') !== False) $this->textHidden('a_status',AP_SHARABLE);
    }
    
    // Now the "submit" buttons
    $submit= array();
    if ($this->MODE=='RO'){
      // if ($this->isWritable())   $submit['modify'] = b_url::same("?form=vm_Apartment&mode=RW&id=".$this->ID);
      $submit['continue'] = b_url::getReferrer();
    }
    $this->closeForm('submit',$submit);
    if (!$this->embedded) bIcons()->explain();
  }
  
  /*
   * Check that the apartment is accesible for the given time period
   */
  static function _apIsAccessible($formDB, $start, $end){
    if (empty($formDB['a_start'])) $formDB['a_start'] = rent0;
    if (empty($formDB['a_end']))   $formDB['a_end']   = rent9;
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
    $q  =  myPear_db()->query("SELECT a_hutid FROM abs_apartments WHERE a_id = $a_id");
    while ($r=myPear_db()->next_record($q)) {
      $reply = $r['a_hutid'];
    }
    return $reply;
  }

  function name($includeArea=False){
    if (!(int)$this->ID) return 'Adding new apartment';
    else                 return self::a_name($this->formDB,$includeArea);
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
    return $this->isWritable;
  }
  
  function formOKspecific() {
    if ($this->getValue('a_area',True) < 2) $this->errorsInTheForm['a_area'] = 'Wrong value'; 
    $this->formDB_modified['a_area']  = (int)$this->getValue('a_area',True);
  }
}
