<?php
/*
 * Unit of the visit endorses.
 * There is 1 such unit per Organization, it keeps list of administrators
 * who have rights to approve/reject visits requests
 *
 * Packed data:
 * - 'reminder' last reminder timestamp
 * - 'v_types' visit type to endorse (comma separated list of visit types)
 * 
 */
locateAndInclude('bUnit_vm');
class bUnit_vm_endorses extends bUnit_vm{

  // when the reminder was last sent
  protected $packed_items = array('reminder','v_types');

  function  __construct($parent_ID=myOrg_ID,$rank=RANK_vm_endorser){
    
    // Apply to the parent bUnit
    parent::__construct($parent_ID,RANK_vm_endorser);
  }
  
  /*
   */
  protected function isMemberEditable(){
    return $this->isWritable();
  }

  /*
   * The member which does the work can't be deleted, dismiss him first
   */
  protected function isUnitMemberDeletable(){
    return empty($this->rec['v_types']);
  }

  /*
   * Update event types to endorse if we are in the interactive dialog
   */
  protected function edit_bList(){
    // Complete the endorsements list
    foreach(VM::$endorsements as $v_type=>$v){
      if ($v === 'bList::orgGroups') VM::$endorsements[$v_type] = myOrg()->orgGroups()->get_groups();
    }
    
    // Check for updates
    if ($this->myPost_bUnit()){
      $v_types = array();
      foreach(VM::$endorsements as $v_type=>$v){
	foreach($v as $code=>$name) { 
	  $this->code2name($code,$name,$v_type);
	  if (bForm::_checkBoxIsOn($code,array($code=>$this->getValue($code)))) $v_types[] = $code;
	}
      }
      sort($v_types);
      $this->updateMember(array('v_types'=>join(',',$v_types)),$_POST['um_id']);
    }
  }
  
  /*
   *
   */
  protected function edit_unitMember($av_id){
    $rec = $this->getUnitMember($av_id);
    $rec['v_types'] = $this->arg_explode(@$rec['v_types']);

    myPear::setPageHeader('Endorser '.bForm_Avatar::_fmtName('fl',$rec,False));

    $this->bForm(array());
    foreach(VM::$endorsements as $v_type=>$groups){
      foreach($groups as $code=>$name){
	$this->code2name($code,$name,$v_type);
	$this->f->defineVariable($name,$code);
	$this->f->isMBE[$code] = True;
	$this->f->formDB[$code] = (in_array($code,$rec['v_types']) ? 1 : 0);
	$this->f->checkbox($code);
      }
    }
    $this->f->textHidden('um_id',$rec['um_id']);
    $this->f->closeForm();
    return 'busy...';
  }

  /*
   * code==0 for the event types common for the whole organization
   */
  private function code2name(&$code,&$name,$v_type){
    if ((string)$code === '0'){
      $code = $v_type;
    }else{
      $name .= ' - '.VM::$description[$v_type]['d']; 
    }
  }


  /*
   * Add specific items
   */
  protected function membersInfo_toSelect($extended=False){
    return array_merge(parent::membersInfo_toSelect($extended),
                       array('v_types'));
  }
  
  protected function header($arg=Null){
    return array('name'=>'', 'v_types'=>'Endorser for','virt_group'=>'');
  }

  /*
   *
   */
  function getEndorsers($subject=Null,$verbose=False){

    // Get the list of endorces with the names, email, etc.
    $endorsers = $this->getDutyHolders('v_types',$subject);

    // Return either a simple array(IDs) or an array useable by selectMenu 
    $reply = array();
    if ($return_name = ($verbose === 'full')){
      $verbose = False;
      foreach($endorsers as $av_id=>$d) if (!empty($av_id) && !empty($d['av_name'])) $reply[$av_id] = $d['av_name'];
    }else{
      foreach(array_keys($endorsers) as $av_id) if (!empty($av_id)) $reply[] = $av_id;
    }
    
    // Complain if the unit is not yet populated
    if ($verbose && empty($reply) && !empty($subject)){
      myPear::WARNING('No endorser appointed for visits of type "'.VM::$description[$subject]['d'].'"'); 
    }
    return $reply;
  }

  /*
   * Remind the endorses about their duties.
   * The reminder is sent once every "VM_reminding_period" days 
   */
  function remind(){
    if (@$_SESSION[__METHOD__]++) return;
    
    // Tell the encoder which URL should be at the login time
    $_GET['q_once'] = join('/',array(VM_MODULE,
				     APImenu::tab_code(tab_vm_guests),
				     APImenu::tab_code(tab_vm_guests_to_approve)));
    
    foreach($this->getUnitMemberIDs() as $av_id){
      $rec = $this->getMember(array('um_avid'=>$av_id));
      if (empty($rec['um_id'])) {
	b_debug::var_dump($rec,"??? empty 'um_id' for av_id='$av_id'");
	b_debug::traceBack("??? empty 'um_id' for av_id='$av_id'");
	continue;
      }
      if (@$rec['reminder']+VM_reminding_period*24*3600  < time()){
	foreach($this->toEndorse($av_id) as $r)  VM_mailer()->remind_endorser($av_id,$r);
	$this->updateMember(array('reminder'=>time()),$rec['um_id']);
      }
    }
  }
  
  /*
   * Return array of visits to be endorsed by the Avatar
   */
  function toEndorse($av_id){
    
    $check_since = time() - 2*VM_reminding_period*24*3600;
    $rec = $this->getMember(array('um_avid'=>$av_id));
    
    // Do not bother about the programs yet
    $v_types = array_diff($this->arg_explode(@$rec['v_types']),
			  array(VISIT_TYPE_PROGRAM));
    
    // Walk thru the requested visits and see what is not yet endorsed
    $reply = array();
    foreach($v_types as $type){
      $q = VM_query(array("v_end > ".$check_since,
			  "v_status != '".STATUS_YES."'",
			  "v_status != '".STATUS_NO."'",
			  "(v_group = '$type' OR v_type = '$type')",
			  ),' ');
      while($r = myPear_db()->next_record($q))      $reply[b_time::period($r['v_start'],$r['v_end']).": ".bForm_Avatar::_fmtName('Lf',$r)] = $r;
    }
    $this->dbg(join(',&nbsp;',array_keys($reply)),$this->debug||cnf_dev);
    return array_values($reply);
  }

  /*
   *
   */
  protected function format(){
    $virt_group = $v_types = array();
    $this->rec['v_types'] = $this->arg_explode(@$this->rec['v_types']);

    foreach(VM::$endorsements as $v_type=>$groups){
      foreach($groups as $code=>$name) { 
	$nameZ = $name;
	$this->code2name($code,$nameZ,$v_type);
	if (in_array($code,$this->rec['v_types'])){
	  if(count($groups) > 1){
	    $virt_group[] = $name;
	    $v_types[$code] = VM::$description[$v_type]['d']; 
	  }else{
	    $v_types[$code] = $name;
	  }
	}
      }
    }
    if (empty($v_types)) $v_types = array(b_fmt::redText('No role assigned yet'));
    $this->rec['v_types']    = x('i',join('<br/>',$v_types));
    $this->rec['virt_group'] = x('i',join('<br/>',$virt_group));
  }
}
