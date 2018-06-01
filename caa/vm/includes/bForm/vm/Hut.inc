<?php
/**
 *                          2011-02-15
 */
locateAndInclude('bForm_vm');
class bForm_vm_Hut extends bForm_vm {

  var $TABLE  = 'abs_huts';
  var $TABLEID= 'hut_id';

  public static $columns = array('hut_name'            => 'Name',
				 'hut_street'          => 'Street address',
				 'hut_city'            => 'City',
				 'hut_country'         => 'Country',
				 'hut_entrance_code'   => 'Entrance lock code <WR/>(if any)',
				 'hut_code'            => 'Accommodation option',
				 'hut_url'             => 'Home page',
				 'hut_status'          => 'Default accommodation?',
				 // 'hut_capacity'        => 'Capacity (N rooms)',
				 );
  public static $compalsary = array('hut_name',
				    'hut_code',
				    'hut_url');
  
  /*
   * Get all huts of the given code and (optionally) for a given time period
   */
  public static $nAps;
  static function getHutsByCode($hut_code=HUT_A,$e_start=Null,$e_end=Null,$exclude=array()){

    $excludes = array(1);
    foreach($exclude as $hut_id) $excludes[] = "hut_id != $hut_id";
    
    // Get list of all available huts

    $q = myPear_db()->query("SELECT hut_id,hut_name ".
			    " FROM abs_huts ".
			    " WHERE hut_code = '$hut_code' AND ".join( ' AND ',$excludes));
    $reply = array();
    while ($r = myPear_db()->next_record($q)){
      if (empty($r['hut_id']))   continue;
      if (empty($r['hut_name'])) continue;
      $reply[$r['hut_id']] = $r['hut_name'];
    }
    
    // Restrict selection by the time period
    if (($e_start !== Null) && ($e_end !== Null)){
      self::$nAps = array();
      foreach($reply as $hut_id=>$x){
	$reply_nAps = 0;
	$q = myPear_db()->query("SELECT a_name,a_start,a_end ".
				" FROM abs_rooms ".
				" LEFT JOIN abs_huts ON hut_id = a_hutid ".
				" WHERE a_hutid = $hut_id ".
				" AND a_end >= $e_end ".
				" AND ( a_start <= $e_start OR a_start = 0 ) AND ".join( ' AND ',$excludes));
	while ($r = myPear_db()->next_record($q)){
	  // if (cnf_dev) b_debug::xxx(sprintf("<pre> %10s %s -> %s </pre>\n",$r['a_name'],date('Y-m-d',$r['a_start']),date('Y-m-d',$r['a_end'])));
	  $reply_nAps++;
	}
	if ($reply_nAps) self::$nAps[$hut_id] = $reply_nAps;
	else            unset($reply[$hut_id]);
      }
    }
    b_debug::xxx(b_fmt::joinX(',',$reply));
    return $reply;
  }

  /*
   *
   */
  function defineVariables() {
    
    foreach (self::$columns as $f=>$d){
      if ($f == 'hut_name'){
	if     ($this->getValue('hut_code') == HUT_H) $d = 'Hotel name';
	elseif ($this->getValue('hut_code') == HUT_HS)$d = 'Hostel name';
	elseif ($this->getValue('hut_code') == HUT_A) $d = 'Name';
	elseif ($this->getValue('hut_code') == HUT_AH)$d = 'Name';
	else                                          $d = 'Name'; 
      }
      if ($this->MODE == 'RO') $d = preg_replace(';<WR/>.*;','',$d);
      $this->defineVariable($d,$f);
    }
    
    foreach(array_keys(self::$columns) as $c) $this->isMBE[$c] = True;
    foreach(self::$compalsary as $c)          $this->isMBE[$c] = False;

    $this->isRO['hut_code'] = ($this->getValue('hut_code') == HUT_BI);
  }

  /*
   *
   */
  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
    $this->dbg('completed');
    //    $this->getValue('hut_capacity');
  }

  /*
   *
   */
  function getForm(){
    
    MSG::H1($this->name());

    $this->getFormOpen();    
    
    // Insert the status field in dialog if not empty 
    $fields = array('hut_name'    => array('textField'),
		    // 'hut_capacity'=> array('textField'),
		    'hut_code'    => array('selectMenu',VM_hutCodes()->getCodes()),
		    'hut_url'     => array('textField'));
    if ($this->isDefault() || ($this->MODE == 'RW')){
      array_unshift_assoc($fields, 'hut_status', array('checkBox'));
    }
    
    $this->formBlock('Hut',
		     'Description',
		     $fields,
		     array('l'=>$this->sameURL_but('RW'),
			   'd'=>'update information',
			   ));
    $this->formBlock('Address',
		     'Address',
		     array('hut_street'        => 'textField',
			   'hut_city'          => 'textField',
			   'hut_country'       => 'textFieldCountry',
			   'hut_entrance_code' => 'textField'),
		     array('l'=>$this->sameURL_but('RW'),
			   'd'=>'update information',
			   ));
    $submit= array();
    $this->closeForm('submit',$submit);
  }
  
  function isReadable(){
    return True;
  }

  function isWritable() { 
    return (parent::isWritable() || VM_rooms_updater_here) && VM_hutCodes()->isWritable($this->getValue('hut_code')); 
  }

  /*
   * Hut name, include optionally the hut code
   */
  function name(){
    if (!(int)$this->ID){
      $reply = 'Adding new hut';
    }else{
      $reply = $this->getValue('hut_name');
      $codes = @VM::$known_hut_codes[$this->getValue('hut_code')];
      if (!stripos($reply,$codes['prefix'])){
	switch(@$codes['position']){
	case 'a': $reply = "$reply ".$codes['prefix']; break;
	case 'b': $reply = $codes['prefix']." $reply"; break;
	default:
	}
      }
    }
    return trim($reply);
  }
  
  /*
   * Set this Hut as "the default one", whatever it means
   */
  function setAsDefault($status='on',$verbose=True){
    if ($status != $this->isDefault()){
      $this->updateDB(array('hut_status' => $status));
      if ($verbose)	MSG::MESSAGE("Set ".$this->name()." as a default accommodation");
    }
  }
  
  /*
   * Get the status
   */
  function isDefault(){
    return $this->checkBoxIsOn('hut_status');
  } 

  /*
   *
   */
  public static function _get_allHuts(){
    static $reply = Null;
    //    static $call = 0; b_debug::_debug("call #".(int)$call++,True);                                                                                                           
    if ($reply === Null){
      $reply = array();
      $q = myPear_db()->qquery("SELECT hut_id,hut_name FROM abs_huts WHERE hut_code != '".HUT_BI."' ORDER BY hut_code",cnf_dev);
      while($r = myPear_db()->next_record($q)) $reply[$r['hut_id']] = $r['hut_name'];
    }
    return $reply;
  }


  /*
   *
   */
  public function get_total_rooms($accommodationOptions=Null,$detailed=False){
    $q = myPear_db()->query(sprintf("SELECT a_id,a_quantity,a_capacity FROM abs_rooms WHERE a_hutid = %s AND a_quantity REGEXP '%s' ORDER BY a_quantity ASC",
				    $this->ID,VM_quantity_total),cnf_dev);
    while ($r = myPear_db()->next_record($q)){
      $a_capacities[] = $r['a_capacity'];
      $a_quantity = (is_object($accommodationOptions)
		     ? $accommodationOptions->get_a_value($r['a_id'],$this->ID,'a_quantity')
		     : $r['a_quantity']);
      $get_total_rooms = trim(str_replace('rooms','',
					  str_replace('=','',
						      str_replace(VM_quantity_total,'',$a_quantity))));
      $a_qs[] = $get_total_rooms;
    }
    if (!empty($get_total_rooms)){
      $a_qs = array_unique($a_qs);
      if (count($a_qs) != 1){
	sort($a_qs);
	MSG::ERROR("Configuration error: ambigious \"total rooms\" (".join(',',$a_qs).") for ".$this->name().
		      "<br><strong><em>In total $get_total_rooms room</em> is used</strong>");
      }
      sort($a_capacities);	
      $this->get_total_rooms = ($detailed
				? array($get_total_rooms => $a_capacities)
				: $get_total_rooms);
      $this->dbg($this->get_total_rooms,cnf_dev);
    }else{
      $this->get_total_rooms = 0;
    }
    return $this->get_total_rooms;
  }

  /*
   *
   */
  public function name_total_rooms($accommodationOptions=Null){
    $reply = '';
    $TR = $this->get_total_rooms($accommodationOptions,True);
    if (!empty($TR)){
      foreach($TR as $n=>$a_capacities){
	$n_rooms = $n;
	foreach($a_capacities as $c) $reply .= '&#91;'.$c.'&#93; ';
      }
      $reply .= VM_quantity_total." $n_rooms rooms";
    }
    if (!empty($reply)) $this->dbg($reply,cnf_dev);
    return $reply;
  }

  //  public static function _getCapacity($hut_id){
  //    $hut = loader::getInstance_new('bForm_vm_Hut',$hut_id,'fatal');
  //    $reply = $hut->getCapacity();
  //    return $reply; 
  //  }

  //  public function getCapacity(){
  //    $reply = $this->getValue('hut_capacity');
  //    return $reply;
  //  }

  /*
   * We do not care here about missing description
   */
  public function getValue($name, $asIs=True, $d_MBE=True){
    $v = parent::getValue($name, True, True);
    return $v;
  }
  
  function formOKspecific() {}
}

/*
 * Should go to the utilities
 */  
function array_unshift_assoc(&$arr, $key, $val){
  $a = array_reverse($arr, true);
  $a[$key] = $val;
  $arr = array_reverse($a, true);
}
