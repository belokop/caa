<?php

locateAndInclude('bList');
locateAndinclude('bForm_lists');

class bList_wiw_tripTypes extends bList{
  
  protected $isReady = True;
  var $list_quering = array('lm_key'  =>'Trip type',
                            'lm_value'=>'TripColor');
  var $list_showing = array('_lm_key'  =>'Trip type',
                            '_lm_value'=>'TripColor',
			    '_virt_n' =>'Usage');
  protected $orderBy = 'lm_key';

  /*
   *
   */  
  function __construct($org_id=Null){
    if (empty($org_id)) $org_id = myOrg_ID;
    parent::__construct($org_id);
    if (empty($this->f)) $this->f = new bForm_lists_wiw($this);
    $this->dbg($org_id,True);
  }

  /*
   *
   */
  protected function isMemberDeletable(){
    locateAndInclude('bForm_wiw_trip');
    $nMembers = count(bForm_wiw_trip::getTrips($this->r['lm_id']));
    $reply = b_cnf::check_conditions(array(False=>array('!empty'=>($nMembers > 0)),
					   'default' => parent::isMemberDeletable()),
				     $this,
				     cnf_dev||$this->debug);
    return $reply;
  }

  /*
   *
   */
  protected function getFormAdd(){
    $this->dbg('',cnf_dev);
    $this->f->show_gv = cnf_dev;
    $this->f->getValue_reset('lm_key',  $this->getMemberValue($_GET['lm_id'],'lm_key'));
    $this->f->getValue_reset('lm_value',$this->getMemberValue($_GET['lm_id'],'lm_value'));
    $this->bForm(array());
    $this->f->defineVariable(array('Trip color'         =>'lm_value',
				   $this->memberTitle() =>'lm_key'));
    $this->f->textField('lm_key');
    $this->f->colorPickerField('lm_value');
    $this->f->closeForm();
  }

  /*
   *
   */
  protected function format(){
    locateAndInclude('bForm_wiw_trip');
    $this->r['_lm_key']  = $this->r['lm_key'];
    $this->r['_lm_value']= strToLower($this->r['lm_value']);
    $this->r['_virt_n'] = count(bForm_wiw_trip::getTrips($this->r['lm_id'])).' times';
    foreach(array_keys($this->list_showing) as $k) $this->t->css[$k]['style'] = "background:".$this->r['lm_value'];
  }

  protected function say_emptyListing(){}
}

class bForm_lists_wiw extends bForm_lists{

  function __construct($object){
    return parent::__construct($object);
  }

  protected function ZZZfData($data,$name=Null){
    $data = preg_replace("/value='([a-zA-Z0-9#]*)'/","value='\\1' style='background-color:\\1'",$data);
    b_debug::var_dump($data);
    return parent::fData($data,$name);
  }
}
