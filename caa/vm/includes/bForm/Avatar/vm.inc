<?php

// Force loading VM module in case of auto-login
require_once(dirname(__FILE__).'/../../../config.inc');
b_reg::load_module(EA_MODULE);

locateAndInclude('bForm_Avatar_ea','fatal');
class bForm_Avatar_vm extends bForm_Avatar_ea{

  var $v_code = null;
  var $needFullInformation = Null;

  /*
   * Helper to bForm_Avatar::delete
   * Supposed to delete all the "traces" of the avatar, but not the avatar itself
   */
  public static function delete_deps($where){
  }

  function  __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
    $this->isReady(True); 

    // Ask for residentship from the visitors with reimbursed travel expenses
    // Not nice code, the same (well, almost) piece of code in bForm_Avatar_vm & bForm_vm_Visit
    locateAndInclude('bForm_vm_Visit');
    if (bForm_vm_Visit::is_visitPolicyOn($this->ID,VM_V_payTrip)){
      $this->need_av_residentship();
    }
  }

  /*
   * Interface... TBD better
   */  
  function need_av_residentship(){
    $this->defineTypes(array('isMBE'=>array('av_residentship' =>False,
					    //'av_address'      =>False,
					    '_av_address'     =>False)));
  }    
  
  /*
   * Reject the candidate for merging if he is unknown to VM
   */
  public static function findDuplicates_reject($r){
    $reply = True;
    foreach($r as $key=>$value) $where[] = "$key REGEXP '$value'";
    $q = myPear_db()->qquery("SELECT v_avid FROM zzz_avatars ".
			       " LEFT JOIN abs_visits ON v_avid = av_id ".
			       " WHERE ".join(' AND ',$where),cnf_show_mrg);
    while($r = myPear_db()->next_record($q)){
      if (!empty($r['v_avid'])){
        $reply = False;
        break;
      }
    }
    return $reply;
  }

  /*
   *
   */
  protected function formBlock_isVisible($block_ID,$fields=array()) {
    switch($block_ID){
    case 'av_birth_block':
      $reply = $this->isWritable() && (empty($_GET['block_once']) || (@$_GET['block_once'] === 'av_birth_block'));
      break;
      
    default:
      $reply = parent::formBlock_isVisible($block_ID,$fields);
    }
    return $reply;
  }

  /*
   * Collect the Avatar duties
   */
  function role($asIcons=False){
    $role = $roleIcon = array();
    if    ($this->isSuperUser())   $role[] = RANK__superuser;
    if    (VM_managers()->isMember($this->ID)) $role[] = RANK_vm_manager;
    if    ($this->isBooker())      $role[] = RANK_vm_booker;
    if    ($this->isObserver())    $role[] = RANK_vm_observer;
    if    ($this->isEndorser())    $role[] = RANK_vm_endorser;
    if    ($this->isOrganizer())   $role[] = RANK_vm_organizer;
    if    ($this->isVisitor())     $role[] = RANK_vm_visitor;
    elseif($this->isRegistrant())  $role[] = RANK_vm_registrant;
    sort($role);
    if ($asIcons){
      foreach($role as $r) $roleIcon[] = bIcons()->get($r);
      return $roleIcon;
    }else{
      return $role;
    }
  }

  protected function askResidentship(){ return True; }

  /*
   * We need full information about the persons who get reimbursed
   */
  protected function needFullInformation(){
    if (!$this->isReady())       return Null;
    if (is_null($this->needFullInformation)){
      $this->needFullInformation = (bool)parent::needFullInformation() || 
	(bool)$this->isVisitor(Null,array(VM_V_payPerdiem,VM_V_payTrip));
    }
    return $this->needFullInformation;
  }
    
  function askPhD(){
    return False; 
    if ($this->isReady()) return parent::askPhD() || $this->isOwner(); 
    else                  return Null;
  }
  
  function askMS(){
    return False; 
    if ($this->isReady()) return parent::askMS()  || $this->isOwner(); 
    else                  return Null;
  }
  
  protected function askSex(){
    return False;
    return $this->needFullInformation(); 
  }

  protected function askHomeAddress(){
    return $this->needFullInformation(); 
}

  /*
   * Check if the Avatar is invited by someone.
   * @param $av_host_id - the host to be checked, 
   *                      default - the currently logged-in user
   */
  function isHostedBy($v_host_avid) {
    $reply = False;
    if(is_object($v_host_avid))  $v_host_avid = $v_host_avid->ID;
    if   ( empty($v_host_avid) && is_object(bAuth::$av)) $v_host_avid = bAuth::$av->ID;
    if   (!empty($v_host_avid) && (int)$this->ID){
      $q = myPear_db()->query("SELECT * FROM abs_visits LEFT JOIN zzz_avatars ON av_id = v_avid WHERE v_host_avid=$v_host_avid AND av_id = ".$this->ID);
      $reply = (myPear_db()->num_rows($q) > 0);
    }
    return $reply;
  }

  /*
   *
   */
  function isWritable(){
    $reply = $this->isReady() && (parent::isWritable() || bAuth::authenticated() && 
				  ((bAuth::$av->ID == $this->ID) ||
				   (bAuth::$av->isBooker()) ||
				   $this->isHostedBy(bAuth::$av)));
    $this->dbg($reply);
    return $reply;
  }
  
  private function unitMember($av_id,$rank,$um_status=Null){
    if (!$av_id)              $av_id = $this->ID;
    if ($um_status) $reply = (VM_unit($rank)->getUnitMemberStatus($av_id) === $um_status);
    else            $reply =  VM_unit($rank)->isMember($av_id);
    b_debug::xxx($reply,array('AA',3,$this));
    return $reply;
  }

  function isEndorser($what=Null,$av_id=Null){
    if (!$av_id)   $av_id = $this->ID;
    $reply = in_array($av_id,VM_endorsers()->getEndorsers($what));
    return $reply;
  }

  function isOrganizer($e_id=Null){
    return VM::isOrganizer($this->ID,$e_id);
  }
  
  function isBooker($av_id=Null){
    return $this->unitMember($av_id,RANK_vm_booker);
  }

  function isObserver($av_id=Null){
    return $this->unitMember($av_id,RANK_vm_observer);
  }

  /*
   * Check that the avatar is a known visitor, i.e.
   * was registered for any (particular) event OR was invited as a guest.
   * Note, the _RENT is not considered as a visit
   *
   * @param $av_id - (int) the avatar to check. 
   *                 If empty, $this is assumed
   * @param $visit_policies (optional array) 
   *               - if not empty, consider only those visits where this policy(ies) is set 
   */
  function isVisitor($av_id=Null,$visit_policies=array()){
    if (empty($av_id)){
      $visits = $this->getVisits(Null,$visit_policies);
    }elseif ($av = myPear::getInstance_new(get_class($this),$av_id,'strict')){
      $visits = $av->getVisits(Null,$visit_policies);
    }
    if (!empty($visits)) $this->dbg(!empty($visits),(bool)($this->debug || cnf_show_a));
    return !empty($visits);
  }

  /*
   * Check that the avatar is registered for any (particular) event.
   * 
   * @param $e_id - the event to check for. 
   *                Check all events if $e_id is empty.
   * @return - (array) list of events where the avatar has applied for.
   */
  function isRegistrant($e_id=Null){
    $reply = array();
    $where = array("v_avid = '$this->ID'", "v_code > 0");
    if ($e_id) $where[] = "v_eid = '$e_id'";
    $q = myPear_db()->query("SELECT * FROM abs_visits WHERE ".join(' AND ',$where));
    while($r = myPear_db()->next_record($q)) $reply[] = $r['v_eid'];
    if (!empty($reply)) $this->dbg($reply,(bool)($this->debug || cnf_show_a));
    return $reply;
  }

  /*
   * Get the list of events to be endorsed, 
   * i.e. the events which are not yet started but the application deadline is reached
   */
  function eventsToEndorse($only_ifApplicantsExist=True){

    locateAndInclude('bForm_vm_Event');

    $reply = array();
    $q = myPear_db()->query("SELECT * FROM abs_events WHERE ".
			      " e_start   > ".time() ." AND ".
			      " e_reg_end < ".time().
			      " ORDER BY e_start ASC");
    while($r = myPear_db()->next_record($q)){
      // b_debug::xxx(date('Y-m-d , ',$r['e_reg_end']).bForm_vm_Event::_name($r));
      if ($only_ifApplicantsExist && !bForm_vm_Event::_attenders($r['e_id'])) continue;
      $reply[$r['e_id']] = bForm_vm_Event::_name($r);
    }
    //    b_debug::xxx('<br>'.join('<br>',array_values($reply)));
    return $reply;
  }

  /*
   * Get the list of visits
   * Optionally check also visit policy, i.e. should the visit be reimbursed, etc.
   * @param v_type
   */
  function getVisits($v_type = 'all',$check_policy=array()){

    switch ($v_type){
    case VISIT_TYPE_COLLABORATION:
    case VISIT_TYPE_PROGRAM:
    case VISIT_TYPE_RENT:
    case VISIT_TYPE_OTHER:
      $where[] = "v_type = '$v_type'";
      break;
    default:
      $where[] = "v_type != '".VISIT_TYPE_RENT."'";
    }

    $where[] = "v_avid = '$this->ID'";
    $reply = array();

    $q = myPear_db()->query("SELECT * FROM abs_visits LEFT JOIN abs_events ON v_eid=e_id WHERE ".
			      join(' AND ',$where)." GROUP BY v_id ORDER BY v_start DESC");
    while($r = myPear_db()->next_record($q)){
      if (!($include_it = empty($check_policy))){
	foreach($check_policy as $p){
	  if (b_mask::get($r['v_policy'],$p))  $include_it = True;
	}
      }
      if ($include_it){
	if (empty($r['e_name'])) $r['e_name'] = "Visit ".b_time::period($r['v_start'],$r['v_end']);
	else                     $r['e_name'] = date('M Y - ',$r['v_start']).$r['e_name'];
	$reply[$r['v_id']] = $r['e_name'];
      }
    }
    return $reply;
  }
}
