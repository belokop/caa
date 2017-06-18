<?php

class myPear_base{

  public $debug        = False;
  public static $_debug= False;
  protected $dbg_color = 'blackText';
  protected static $_dbg_color = 'blackText';

  public $errorsInTheForm = array();

  /*
   *
   */
  function __construct() {}

  /*
   *
   */
  public function dbg($arg0='',$arg1=array(),$arg2=False, $arg3=0) {
  //                     $id,     $text,   $force,      $addLevel
    
    if (!class_exists('b_debug',False)) return;

    // Monitor the change of a variable 
    // public static function watchDog($now,$class='all',$condition=True,$verbose=False);

    //if (stripos(get_class($this),'bList') !== False)    b_debug::watchDog(@$this->TABLEID,$this);
    //b_debug::watchDog(@$this->isMBE['av_citizenship'],$this,True);
    //b_debug::watchDog(@$this->isRO['av_birthdate'],$this,True);
    //b_debug::watchDog($this->MODE,$this,(stripos(get_class($this),'Lease')));
    //b_debug::watchDog("$this->t",get_class($this),True);
    //b_debug::watchDog($this->debug,$this,(stripos(get_class($this),'Avatar') !== False));
    //if (stripos(get_class($this),'Visit') !== False)    b_debug::watchDog(@$this->isRO['v_status'],$this);
    //    b_debug::watchDog($_REQUEST['vm_eventSelector']);
    //    b_debug::watchDog($this->errorsInTheForm, $this);
    //    $x=b_cnf::get('from_bForm');  if (class_exists('b_debug',0)) b_debug::watchDog($x,'from_bForm');
    //    $x=@$this->rec['lease_option'];  if (class_exists('b_debug',0)) b_debug::watchDog($x,'lease_option');

    // Will it work?
    self::$_dbg_color = $this->dbg_color;

    if(True){
      switch(func_num_args()){
      case 0: 	$arg0 = ' ';
      case 1:	$arg1 = False;
      case 2:  
	if (!is_array($arg1)) $arg1 = array($arg1);
	foreach($arg1 as $k=>$v){
	  if (is_bool($v))   {$boolOK=True; $arg1[$k] = (bool)($v || $this->debug || self::$_debug || $arg2);}
	  if (is_object($v))  $objectOK=True; 
	  if (is_string($v))  $stringOK=True; 
	  if (is_numeric($v)) $numericOK=True; 
	}
	if (empty($boolOK))    $arg1[] = ($this->debug || self::$_debug || $arg2);
	if (empty($stringOK))  $arg1[] = $this->dbg_color;
	if (empty($numericOK)) $arg1[] = 2+$arg3;
	if (empty($objectOK))  $arg1[] = $this;
	return b_debug::_debug($arg0,$arg1);
	return b_debug::_debug($arg1,array(2+$arg3, 
					   $this, 
					   (bool)($arg2 || $this->debug || self::$_debug), 
					   (string)$this->dbg_color));
      default: 
	b_debug::internalError("unexpected arguments...");
      }
    }else{
      switch(func_num_args()){
      case 0: 
	return b_debug::xxx(' ', array(3, $this, (bool)($this->debug||self::$_debug), $this->dbg_color));
      case 1:
	$_arg1 = False;
      case 2:  
	$_arg1 = (is_array($arg1)
		  ? $arg1
		  : array(3, $this, (bool)$arg1, (string)$this->dbg_color));
	foreach($_arg1 as $k=>$v) if (is_bool($v)) $_arg1[$k] = (bool)($v || $this->debug || self::$_debug);
	return b_debug::xxx($arg0,$_arg1);
      default: 
	  return b_debug::xxx($arg1,array(3+$arg3, $this, (bool)($arg2 || $this->debug || self::$_debug), $this->dbg_color));
      }
    }
  }

  /*
   *
   */
  function __toString() { return get_class($this); }
}
