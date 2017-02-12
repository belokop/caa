<?php

class myPear_base{

  public $debug        = False;
  public static $_debug= False;
  protected $dbg_color = 'blackText';

  function __construct() {}

  //                  $id,     $text,   $force,      $addLevel
  protected function dbg($arg0='',$arg1='',$arg2=False, $arg3=0) {
    
    if (!class_exists('b_debug',False)) return;

    // Monitor the change of a variable 
    // public static function watchDog($now,$class='all',$condition=True,$verbose=False);

    //if (stripos(get_class($this),'bList') !== False)    b_debug::watchDog(@$this->TABLEID,$this);
    //b_debug::watchDog(@$this->isMBE['av_citizenship'],$this,True);
    //b_debug::watchDog(@$this->isRO['av_birthdate'],$this,True);
    //b_debug::watchDog($this->MODE,$this,(stripos(get_class($this),'Lease')));
    //b_debug::watchDog("$this->t",get_class($this),True);
    //b_debug::watchDog($this->debug,$this,(stripos(get_class($this),'Avatar') !== False));
    //$x=defined('superUser_here');  if (class_exists('b_debug',0)) b_debug::watchDog($x);
    //if (stripos(get_class($this),'Visit') !== False)    b_debug::watchDog(@$this->isRO['v_status'],$this);
    //    b_debug::watchDog($_REQUEST['vm_eventSelector']);

    switch(func_num_args()){
    case 0: 
      return b_debug::xxx(' ',  array(3,$this, (bool)($this->debug||self::$_debug), $this->dbg_color));
    case 1:
      return b_debug::xxx($arg0,array(3,$this, (bool)($this->debug||self::$_debug), $this->dbg_color));
    case 2:  
      if    (is_bool($arg1))  return b_debug::xxx($arg0,array($arg1,3,'redText',$this));
      elseif(is_array($arg1)) return b_debug::xxx($arg0,$arg1);
    default: 
      return b_debug::xxx($arg1,array(3+$arg3,$this, (bool)($this->debug || self::$_debug || $arg2), $this->dbg_color));
    }
  }
    
}