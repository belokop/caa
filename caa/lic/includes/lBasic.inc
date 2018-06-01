<?php
b_debug::xxx(__FILE__,'L');
class lBasic {

  public $debug = False;

  var $w      = array(); // WHERE clause in the sql query
  var $ID     = Null;
  var $TABLE  = Null;
  var $TABLEID= Null;

  function __construct() {
    $id = loader::$sql_identities[get_class($this)];
    $this->TABLE  = $id['t'];
    $this->TABLEID= $id['i'];
  }

  function data2id($data) {
    if (empty($this->w)) foreach ($data as $k=>$v) $this->w[] = "$k='$v'";
    $q = myPear_db()->qquery("SELECT * FROM $this->TABLE WHERE ".join(' AND ',$this->w),$this->debug);
    while ($r = myPear_db()->next_record($q)) $this->ID = $r[$this->TABLEID];
  }

  function updateDB($data) {
    if ($this->ID) {
      foreach ($data as $k=>$v) $w[] = "$k='$v'";
      $q = myPear_db()->qquery("UPDATE $this->TABLE SET ".join(',',$w). " WHERE $this->TABLEID = $this->ID",$this->debug);
    }else{
      $this->data2id($data);
      if (!$this->ID) {
	$this->convert($data);
	$q = myPear_db()->qquery("INSERT INTO $this->TABLE SET ".join(',',$this->w),$this->debug);
	$this->data2id($data);
	print get_class($this)." - new record ".join(' ',$this->w)."\n";
      }
    }
  }

  function convert(&$data) {}

  function __toString() { return get_class($this); }
}

class lEvent extends lBasic{
  function __construct($id=Null,$db=Null) {
    parent::__construct();
  }
}


class lHost extends lBasic{

  function __construct($id=Null,$db=Null) {
    parent::__construct();
  }

  function convert(&$data) {
    if (!isset($data['h_ip']))    $data['h_ip'] = gethostbyname($data['h_name']);
    if ( isset($data['h_ip'])){
      unset ($this->w);
      $this->data2id($data);
    }
  }
}


class lServer extends lBasic{
  function __construct($id=Null,$db=Null) {
    parent::__construct();
  }
}


class lSoft extends lBasic{
  function __construct($id=Null,$db=Null) {
    parent::__construct();
  }
}


class lUser extends lBasic{
  function __construct($id=Null,$db=Null) {
    parent::__construct();
  }
}
