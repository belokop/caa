<?php
/*
 * Member format:
 *  lm_key    - unique accommodation option (say, 'hotel', 'AppHotel', etc.),
 *              Those options are used to assign bForm_vm_Hut, see column "hut_code"
 *  lm_value  - accommodation description
 */
locateAndInclude('bList_vm');
class bList_vm_hutCodes extends bList_vm{
  
  var $debug = False;
  var $list_quering = array('lm_key'   =>'Unique code',
                            'lm_value' =>'Accommodation option',
			    );
  var $list_showing = array(
			    'lm_value' => 'Accommodation option',
			    'number'   => 'N objects',
			    );
  protected $isReady  = True;

  /*
   *
   */
  function __construct(){
    parent::__construct(myOrg_ID);
    if (!$this->getMembers()){
      // Populate the list with the default codes
      foreach(VM::$known_hut_codes as $code=>$descr){
	$this->addMember(array('lm_key'   => $code,
			       'lm_value' => $descr['prefix'].'s'));
      }
    }
  }
    
  /*
   *
   */
  protected function message_addMember($r){
    myPear::MESSAGE('Adding "'.$r['lm_value'].'" to the accommodation options');
  }
  
  private $getFormAdd_OK = '?';  
  function getFormAdd_OK(){
    $lm_id = $this->getValue('lm_id');
    if ($this->getFormAdd_OK === '?'){
      if ($this->getFormAdd_OK = parent::getFormAdd_OK()){
	$lm_key = $this->getMemberValue($lm_id,'lm_key');
	$members = $this->getMembers(array('lm_key'=>$lm_key));
	unset($members[$this->member_ID]);
	$this->getFormAdd_OK = empty($members);
	if (!$this->getFormAdd_OK) myPear::ERROR("Accommodation option \"$lm_key\" is already used");
      }
      $this->dbg($this->getFormAdd_OK);
    }
    return $this->getFormAdd_OK;
  }
  
  function isWritable(){
    return VM_rooms_updater_here || parent::isWritable();
  }

  /*
   * Deletable if there is there is no aps of this type
   */
  protected function isMemberDeletable(){
    locateAndInclude('bForm_vm_Hut');
    $reply= (($this->isWritable()) && 
	     !bForm_vm_Hut::getHutsByCode($this->r['lm_key']));
    $this->dbg($reply);
    return $reply;
  }
  
  protected function isMemberEditable(){
    $reply = $this->isWritable();
    $this->dbg($reply);
    return $reply;
  }

  // The hut codes (hence, the accommodation options) are not addable,
  // I do see a reason for this.
  // May be I am wrong...
  function isAddable(){
    $reply = False;
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   *
   */
  protected function getFormAdd(){

    $this->bForm($this->list_quering,True);
    $this->f->tips = array('lm_key'    => 'Unique code',
			   'lm_value'  => 'Human readable accommodation option',
			   );
    
    $lm_id = $this->getValue('lm_id');
    $args_ready = (!myPear::is_empty($this->getMemberValue($lm_id,'lm_key')) && 
		   !myPear::is_empty($this->getMemberValue($lm_id,'lm_value')));
    
    $this->f->isRO['lm_key'] = $args_ready; //  && $this->getFormAdd_OK();

    $this->f->textField('lm_key');
    $this->f->textField('lm_value');
    $this->f->closeForm();
  }

  /*
   *
   */
  function getName($lm_key){
    $r = $this->getMember(array('lm_key'=>$lm_key));
    return $r['lm_value'];
  }

  /*
   *
   */
  function getCodes($accommodationOptions=Null,$from_to=array()){
    locateAndInclude('bForm_vm_Lease');
    foreach ($this->getMembers() as $r){
      $reply[$r['lm_key']] = $r['lm_value'];
      if (!empty($from_to)){
	// Check that the hut is available for the given period 
	$_freeRooms = bForm_vm_Lease::_freeRooms($accommodationOptions,
						 $from_to[0],
						 $from_to[1],
						 False,Null,
						 $r['lm_key']);
	if (empty($_freeRooms)) unset($reply[$r['lm_key']]);
      }
    }
    if (empty($reply)) $reply = array();
    return $reply;
  }

  /*
   *
   */
  protected function format($secondCall=False){
    locateAndInclude('bForm_vm_Hut');
    $this->r['number'] = ' '.count(bForm_vm_Hut::getHutsByCode($this->r['lm_key']));
  }
}
