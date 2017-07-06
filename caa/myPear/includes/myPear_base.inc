<?php

class myPear_base{

  public static $_debug= False;
  public static $_dbg_color = 'blackText';

  public $errorsInTheForm = array();

  /*
   *
   */
  function __construct() {}

  /*
   *
   */
  public function dbg($arg0='',$arg1=array(),$arg2=False, $arg3=0) {
    //                $text,   $flags,       $force,      $addLevel
    
    if (!class_exists('b_debug',False)) return;
    
    // Monitor the change of a variable 
    // public static function watchDog($now,$class='all',$condition=True,$verbose=False);

    //if (stripos(get_class($this),'bList') !== False)    b_debug::watchDog(@$this->TABLEID,$this);
    //b_debug::watchDog(@$this->isMBE['av_citizenship'],$this,True);
    //b_debug::watchDog(@$this->isRO['av_birthdate'],$this,True);
    //b_debug::watchDog($this->MODE,$this,(stripos(get_class($this),'Lease')));
    //b_debug::watchDog("$this->t",get_class($this),True);
    //if (stripos(get_class($this),'Visit') !== False)    b_debug::watchDog(@$this->isRO['v_status'],$this);
    //    b_debug::watchDog($_REQUEST['vm_eventSelector']);
    //    b_debug::watchDog($this->errorsInTheForm, $this);
    //    $x=b_cnf::get('from_bForm');  if (class_exists('b_debug',0)) b_debug::watchDog($x,'from_bForm');
    //    $x=@$this->rec['lease_option'];  if (class_exists('b_debug',0)) b_debug::watchDog($x,'lease_option');

    switch(func_num_args()){
    case 0: 
      $arg0 = ' ';
    case 1:
      $arg1 = False;
    case 2: 
      $arg2 = False;
      $arg3 = 0;
      
      if (!is_array($arg1)) $arg1 = array($arg1);
      foreach($arg1 as $k=>$v){
	if (is_bool($v))   {$boolOK=True; $arg1[$k] = (bool)($v || static::$_debug || $arg2);}
	if (is_object($v))  $objectOK=True; 
	if (is_string($v))  $stringOK=True; 
	if (is_numeric($v)) $numericOK=True; 
      }
      if (empty($boolOK))    $arg1[] = (bool)(static::$_debug || $arg2);
      if (empty($stringOK))  $arg1[] = static::$_dbg_color;
      if (empty($numericOK)) $arg1[] = 2+$arg3;
      if (empty($objectOK))  $arg1[] = $this;
      //      if (is_string($arg0))if (trim(strip_tags($arg0)) === 'event 267906') var_dump(self::$_dbg_color,static::$_dbg_color);
      return b_debug::_debug($arg0,$arg1);
    default: 
      b_debug::internalError("unexpected arguments...");
    }
  }
  
  /*
   *
   */
  function __toString() { return get_class($this); }
}
