<?php

locateAndInclude('bList');
locateAndinclude('bForm_lists');

class bList_wiw_tripTypes extends bList{
  
  protected $isReady = True;
  var $list_quering = array('lm_key'  =>'Trip type',
                            'lm_value'=>'TripColor');
  var $list_showing = array('lm_key'  =>'Trip type',
                            'lm_value'=>'TripColor',
			    '_virt_n' =>'N entries');
  protected $orderBy = 'lm_key';

  var $isColor       = True;
  
  function __construct($org_id=Null){
    if (empty($org_id)) $org_id = myOrg_ID;
    parent::__construct($org_id);
    $this->dbg($org_id,True);
  }

  /*
   *
   */
  function getColor($memmmbbbbrrrr){
    foreach ($this->getMembers() as $id=>$r){ 
      if ($r['lm_status'] == $ev_id) return $r['lm_id'];
    }
  }
  
  /*
   * Deletable if not used
   */
  protected function isMemberDeletable(){
    locateAndInclude('bForm_wiw_trip');
    $nMembers = count($t=bForm_wiw_trip::getTrips($this->r['lm_id']));
    if ($nMembers) myPear::MESSAGE("<em>\"".$this->r['lm_key']."\"</em> - $nMembers entries");
    return b_cnf::check_conditions(array(False=>array('!empty'=>($nMembers > 0)),
                                         'default' => parent::isMemberDeletable()),
                                   $this,
				   True);
  }

  /*
   *
   */
  private $ff = Null;
  protected function show_open(){
    if (empty($this->ff)){
      $this->ff = new bForm_lists_wiw($this);
      $this->ff->getFormOpen();
    }
  }

  protected function show_close(){
    if (!empty($this->ff)){
      $this->ff->closeForm();
    }
  }
  
  protected function format(){
    $name = 'lm_'.$this->r['lm_id'];
    // Apply the updates if any
    if ($new_color = @$_POST[$name.'_color']){
      $this->updateMember(array('lm_value' => $new_color),$this->r['lm_id']);
      $this->r['lm_value'] = $new_color;
    }
    $this->ff->defineVariable($this->r['lm_key'],$name);
    $this->ff->formDB[$name] = $this->r['lm_value'];
    $this->ff->isRO[$name]   =!$this->isMemberEditable();
    $this->ff->isMBE[$name]  =! True;
    $this->ff->colorPickerField($name,array($this->button_deleteListMember()));
    $this->r = array();
  }

  protected function say_emptyListing(){}
}

class bForm_lists_wiw extends bForm_lists{

  function __construct($object){
    return parent::__construct($object);
  }

  protected function fData($data,$name=Null){
    $data = preg_replace("/value='([a-zA-Z0-9#]*)'/","value='\\1' style='background-color:\\1'",$data);
    b_debug::var_dump($data);
    return parent::fData($data,$name);
  }
}
