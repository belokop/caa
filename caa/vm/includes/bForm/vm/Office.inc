<?php
/**
 *                          2010-05-30
 */
locateAndInclude('bForm_vm');
class bForm_vm_Office extends bForm_vm {

  var $TABLE  = 'abs_offices';
  var $TABLEID= 'o_id';

  /*
   * get list of the offices available during the given period
   */
  public static function available($start,$end){
    $q = myPear_db()->query("SELECT * FROM abs_offices WHERE o_status='".OFFICE_AUTO_ALLOCATABLE."'");
    // etc...
  }
  
  function defineVariables() {
    $this->defineVariable(array('Capacity (desks)'=> 'o_capacity',
				'Name'  => 'o_name',
				'Phone' => 'o_phone',
				'Status'=> 'o_status',
				'From'  => 'o_start',
				'To'    => 'o_end'));

    $this->defineTypes(array('isMBE'=>array('o_phone',
					    'o_start',
					    'o_end')));
  }

  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }

  function readDefRecord(){
    $q = myPear_db()->query("SELECT * FROM $this->TABLE WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q)) {
      $this->formDB = $r; 
      $day1 = b_time::txt2unix('2008-03-14');
      if ((int)$this->getValue('o_start',1) < $day1) $this->formDB['o_start'] = rent0;
      if ((int)$this->getValue('o_end'  ,1) < $day1) $this->formDB['o_end']   = rent9;
    }
  }

  /*
   * True if the office is shared
   */
  public function _isShared(){
    return False;
  }

  public static function oo(){
    $q = myPear_db()->query("SELECT o_id FROM abs_offices WHERE o_name = '".VM_TEXT_oo."'");
    while($r=myPear_db()->next_record($q))  return $r['o_id'];
  }
  
  function getForm(){
    
    myPear::H1($this->name());

    $this->getFormOpen();    
    $this->formBlock("",'Office',
		     array('o_name',
			   'o_phone',
			   'o_capacity',
			   'o_status'=>array('selectMenu',array('staff'=>'staff',OFFICE_AUTO_ALLOCATABLE=>OFFICE_AUTO_ALLOCATABLE))),
		     array('l'=>$this->sameURL_but('RW'),
			   'd'=>'update office',
			   'i'=>'i-alarm_clock'));
	
    $submit= array();
    if ($this->MODE=='RO' && $this->isWritable())   $submit['update office data'] = $this->sameURL_but('RW');
    $this->closeForm('submit',$submit);
  }
  
  function isReadable(){
    return True;
  }

  function isWritable() { 
    if ($this->getValue('o_name') == VM_TEXT_oo) return False;
    return (VM::hasRightTo('book') || parent::isWritable() || VM_access()->mayDoAccounting() || VM::hasRightTo('book'));
  }

  function name($includeCapacity=True){
    if ($includeCapacity) $n = $this->getValue('o_capacity',True);
    else                  $n = 0;
    if (!(int)$this->ID)       return 'Adding new office';
    elseif ($n > 0 && $n < 99) return sprintf("%s  [%s]",$this->getValue('o_name',True),$n);
    else                       return                    $this->getValue('o_name',True);
  }

  function formOKspecific() {}
}
