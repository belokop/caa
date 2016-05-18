<?php
locateAndInclude('bUnit_vm');
/*
 * Unit of the visit bookers.
 */
class bUnit_vm_bookers extends bUnit_vm{

  protected $packed_items = array('duties');

  // Human readable duty names
  public $duties = array(DUTY_guests       => array('t'  =>'Handles guests'),
			 DUTY_programs     => array('t'  =>'Handles programs'),
			 DUTY_reimbursement=> array('t'  =>'Handles reimbursement'),
			 DUTY_unlock_events=> array('t'  =>'Unlocks events',
						    'def'=>'manager'),
			 DUTY_update_rooms => array('t'  =>'Update rooms',
						    'def'=>'manager'),
			 );
  
  function __construct($parent_ID=myOrg_ID,$rank=RANK_vm_program_coordinator){

    parent::__construct($parent_ID,$rank);

    // Add managers to the bookers unit if there are duties for them
    foreach(VM_managers()->getMembers() as $x=>$rec){
      $um_id = $this->add_unitMember($rec['av_id'],'if_new');
      $manager_duties = $this->arg_explode($this->get_memberInfo($rec['av_id'],'duties'));
      foreach($this->duties as $duty=>$d) if (@$d['def']==='manager') $manager_duties[] = $duty;
      $manager_duties = array_unique($manager_duties);
      sort($manager_duties);
      $this->updateMember(array('duties'=>join(',',$manager_duties)),$um_id);
    }
  }

  /*
   * Get the bookers on duty
   */
  function membersOnDuty($duty){
    $reply = array();
    foreach($this->getDutyHolders('duties',$duty) as $av_id=>$d){
      $reply[$d['av_name']] = $d['av_email'];
    }
    return $reply;
  }

  /*
   * Check that the currently logged in user handles this duty
   */
  function isHandlingDuty($duty){
    $reply = False;
    foreach($this->membersOnDuty($duty) as $name=>$email){
      if (is_object(bAuth::$av) && ($email == bAuth::$av->getEmail())) $reply = True;
    }
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
  function isWritable(){
    $reply = VM_manager_here;
    $this->dbg($reply);
    return $reply;
  }
   */
  
  /*
   * The member which does the work can't be deleted, dismiss him first
   */
  protected function isUnitMemberDeletable(){
    $reply = VM_manager_here && empty($this->rec['duties']);
    $this->dbg($reply);
    return $reply;
  }

  protected function isMemberEditable(){
    $reply = $this->isWritable();
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * Update duties
   */
  protected function edit_bList(){
    // Check for updates
    if ($this->myPost_bUnit()){
      $duties = array();
      foreach(array_keys($this->duties) as $duty_code){
	if (bForm::_checkBoxIsOn($duty_code,array($duty_code=>$this->getValue($duty_code)))) $duties[] = $duty_code;
      }
      sort($duties);
      $this->updateMember(array('duties'=>join(',',$duties)),$_POST['um_id']);
    }
  }

  /*
   * Edit dialog
   */
  protected function edit_unitMember($av_id){
    $rec = $this->getUnitMember($av_id);
    $rec['duties'] = $this->arg_explode(@$rec['duties']);

    myPear::setPageHeader('Booker '.bForm_Avatar::_fmtName('fl',$rec,False));
    
    $this->bForm(array());
    foreach($this->duties as $duty_code=>$descr){
      $this->f->isRO[$duty_code] = (@$descr['def'] === 'manager') && VM_managers()->isMember($av_id);
      $this->f->defineVariable($descr['t'].'?',$duty_code);
      $this->f->isMBE[$duty_code] = True;
      $this->f->formDB[$duty_code] = (in_array($duty_code,$rec['duties']) ? 1 : 0);
      $this->f->checkbox($duty_code);
    }
    $this->f->textHidden('um_id',$rec['um_id']);
    $this->f->closeForm();
    return 'busy...';
  }

  /*
   * Add specific items
   */
  protected function membersInfo_toSelect($extended=False){
    $reply = parent::membersInfo_toSelect($extended);
    $reply[] = 'duties';
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   *
   */
  function header(){
    $reply = array('name'=>'');
    foreach($this->duties as $code=>$descr) $reply[$code] = str_replace(' ','<br/>',$descr['t'].'?');
    return $reply;
  }
  
  /*
   *
   */
  protected function format(){
    $duties = $this->arg_explode(@$this->rec['duties']);
    
    foreach(array_keys($this->duties) as $duty_code){
      if (in_array($duty_code,$duties)) $this->rec[$duty_code] = bIcons()->get('i-ok');
    }
  }
}
