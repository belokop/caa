<?php
/**
 *                          2010-05-30
 */
locateAndInclude('bForm');
class bForm_ea_Office extends bForm{

  var $TABLE  = 'abs_offices';
  var $TABLEID= 'o_id';

  function defineVariables() {
    $this->defineVariable(array('Capacity (desks)'=> 'o_capacity',
				'Name'  => 'o_name',
				'Phone' => 'o_phone',
				'For whom'=> 'o_status',
				'From'  => 'o_start',
				'To'    => 'o_end'));

    $this->defineTypes(array('isMBE'=>array('o_phone',
					    'o_start',
					    'o_end',
					    )));
  }

  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);

    $this->defineVariables();

    $day1 = b_time::txt2unix('2008-03-14');
    if ((int)$this->getValue('o_start',1) < $day1) $this->getValue_reset('o_start',myPear_day0);
    if ((int)$this->getValue('o_end'  ,1) < $day1) $this->getValue_reset('o_end',  myPear_day9);
    $this->dbg('------------------------');
  }

  /*
   *
   */
  function readDefRecord(){
    $q = myPear_db()->query("SELECT * FROM $this->TABLE WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q)) $this->formDB = $r; 
  }

  /*
   * True if the office is shared
   */
  public function _isShared(){
    return False;
  }

  function getForm(){
    
    MSG::H1($this->name());
    
    $this->getFormOpen();    
    $this->formBlock('Office',
		     'Office',
		     array('o_name',
			   'o_phone',
			   'o_capacity',
			   'o_status'=>array('selectMenu',array('staff'=>'staff',OFFICE_AUTO_ALLOCATABLE=>OFFICE_AUTO_ALLOCATABLE))),
		     array('l'=>$this->sameURL_but('RW'),
			   'd'=>'update office',
			   //'i'=>'i-alarm_clock',
			   ));
    
    $submit= array();
    if ($this->MODE=='RO' && $this->isWritable())   $submit['update office data'] = $this->sameURL_but('RW');
    $this->closeForm('submit',$submit);
  }
  
  function isReadable(){
    return True;
  }

  function isWritable() {
    $reply = (defined('EA_administrator_here') && EA_administrator_here);
    return $reply;
  }

  function name($includeCapacity=True){
    if ($includeCapacity) $n = $this->getValue('o_capacity',True);
    else                  $n = 0;
    if (!(int)$this->ID)       return 'Adding new office';
    elseif ($n > 0 && $n < 99) return sprintf("%s  [%s]",$this->getValue('o_name',True),$n);
    else                       return                    $this->getValue('o_name',True);
  }

  function formOKspecific() {
    if (!is_numeric($this->getValue('o_capacity')) || ((int)$this->getValue('o_capacity') <=0))
      $this->errorsInTheForm['o_capacity'] = "Capacity must integer";
  }
}
