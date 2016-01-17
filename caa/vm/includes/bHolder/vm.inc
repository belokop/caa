<?php
locateAndInclude('bHolder');
class bHolder_vm extends bHolder{

  var $sortBy_default = 'e_start';
  var $onlyAgenda   = False;
  var $highlight    = '';
  var $autoOpenHeader = True;

  function __construct(){
    parent::__construct();
  }

  function getEntries(){
    return $this->getAllEvents();
  }
  
  function listEntries(){
    $nLine = 0;
    $header = $this->header();  // sets also title
    $this->_open();
    if (empty($_GET['sBy']) || empty($header[$_GET['sBy']]))  $_GET['sBy'] = $this->sortBy_default;
    if ($q = $this->query()){
      while ($this->rec = myPear_db()->next_record($q)){ 
	if (!$this->rejectRecord()){
	  if ($this->autoOpenHeader) $this->drawHeader();
	  $this->format();
	  if (isset($this->t) && ($this->t instanceof b_table)) $this->t->prt($this->rec,$this->highlight);    
	  if (++$nLine > $this->maxLines){
	    print b_fmt::redText($this->maxLines." lines printed... Output stops");
	    break;
	  }
	}
      }
    }
    
    $this->listEntries_summary();
    if (isset($this->t) && ($this->t instanceof b_table)){
      $this->t->close();
      $this->_close();
      bIcons()->explain();
    }else{
      if (myPear_db()->num_rows($q)) $this->noSuccess();
      $this->_close();
    }
  }

  function listEntries_summary() {}

  function getAllEvents(){
    $q = myPear_db()->query("SELECT * FROM abs_events WHERE ".
			      ($this->onlyAgenda?'e_code>0':'1').
			      " ORDER BY e_start");
    $list = array();
    while ($r=myPear_db()->next_record($q)) $list[]=$r['e_id'];
    return $list;
  }

  function _emptySelection() {}
}
