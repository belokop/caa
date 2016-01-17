<?php
class bIcal {

  public $debug = False;

  function __construct() {
    if (($d=b_cnf::get('formdebug','?')) && (stripos(get_class($this),$d) !== False)) $this->debug=True;
  }

  function dbg($text,$forced=False){
    if (is_null($this->debug))  $this->debug = (bool)b_cnf::get('qdebug');
    if ($this->debug || $forced) b_debug::xxx($text,array($this,3,'orangeText',True));
  }

  static public function dateColor($date,$default=array(),$alwaysHave=array()) {
    if (b_time::isRedDay($date)){
      $classes = array_merge(array('bIcal_weekend'),$alwaysHave);
    }else{
      if (!is_array($default)) $default = array($default);
      $classes = array_merge($alwaysHave,$default);
    }
    if ($classes) $reply = 'class="'.join(' ',$classes).'"';
    else          $reply = '';
    return $reply;
  }

  function id($id) {
    return mb_strToLower(preg_replace("/[#| -\/,.]/","",$id));
  }
  
  function __toString() { return get_class($this); }
}
