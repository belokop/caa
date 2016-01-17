<?php
locateAndInclude('APIaccess');
class APIaccess_vm extends APIaccess{
  
  /*
   * Construct the privileges
   */
  public function __construct(){

    // Ensure the EA access is loaded
    b_reg::load_module(EA_MODULE);
    EA_access();
    
    parent::__construct();
    
    bForm_Avatar::set_context(bAuth::$av);
    $authenticated = bAuth::authenticated();
    $this->who_is_here(array('VM_manager_here'      => array('rank'=> 'RANK_vm_manager',
							     'def' => 'superUser_here'),
			     'VM_booker_here'       => array('rank'=> 'RANK_vm_booker',
							     'def' => 'VM_manager_here'),
			     'VM_observer_here'     => array('rank'=> 'RANK_vm_observer',
							     'def' => 'VM_booker_here'),
			     'VM_visitor_here'      => array('rank'=> 'RANK_vm_visitor'),
			     'VM_registrant_here'   => array('rank'=> 'RANK_vm_registrant'),
			     'VM_authenticated_here'=> array('rank'=> 'RANK__authenticated'),
			     'VM_organizer_here'    => array('IF'  => ((VM::e_ID() && VM::$e->isOrganizer()) || VM::isOrganizer())),
			     'VM_endorser_here'     => array('IF'  => ($authenticated && method_exists(bAuth::$av,'isEndorser') && bAuth::$av->isEndorser())),
			     'VM_employee_here'     => array('IF'  => ($authenticated && method_exists(bAuth::$av,'isE')        && bAuth::$av->isE())),
			     'VM_lt_visitor_here'   => array('IF'  => ($authenticated && method_exists(bAuth::$av,'isV')        && bAuth::$av->isV())),
			     ));

    $this->who_is_here(array('VM_guests_handlare_here'    =>array('IF'=> ($authenticated && VM_bookers()->isHandlingDuty(DUTY_guests))),
			     'VM_rooms_updater_here'      =>array('IF'=> ($authenticated && VM_bookers()->isHandlingDuty(DUTY_update_rooms))),
			     'VM_reimberser_here'         =>array('IF'=> ($authenticated && VM_bookers()->isHandlingDuty(DUTY_reimbursement))),
			     'VM_unlocker_here'           =>array('IF'=> ($authenticated &&(VM_bookers()->isHandlingDuty(DUTY_unlock_events)||VM_manager_here))),
			     'VM_external_registrant_here'=>array('IF'=>VM_registrant_here && !VM_lt_visitor_here && !VM_employee_here),
			     'VM_administrator_here'      =>array('IF'=>VM_observer_here || VM_endorser_here || VM_organizer_here),
			     ));

    $this->who_is_here(array('VM_member_here'             =>array('IF'=>EA_member_here || VM_external_registrant_here || VM_administrator_here || VM_organizer_here),
			     ));
  }
  
  /*
   *
   */
  public function isManager(){
    return VM_manager_here;
  }

  /*
   * Do not authorize registrants to scan the database
   */
  public function authorized(){
    if ($reply = parent::authorized()){
      $reply = ($this->getRank() > RANK_vm_registrant) || VM::hasRightTo('request_visits');
    }
    $this->getTitle();
    return $reply;
  }
  
  /*
   *
   */
  function getRank(){
    static $reply = Null;
    if ($reply === Null){
      $reply = parent::getRank();
      if ($reply == RANK__authenticated){
	foreach ($this->getRanks_matrix() as $e_id=>$r) $reply = max($r,$reply);
      }
    }
    return $reply;
  }

  /*
   *
   */
  public function mayDoAccounting($av_id=''){
    $reply = False;
    if (is_object(bAuth::$av)){
      if (empty($av_id)) $av = bAuth::$av;
      else               $av = myPear::getInstance('bForm_Avatar_vm',$av_id);
      $reply = superUser_here || $av->isObserver();
    }
    return $reply;
  }
  
  /*
   *
   */
  public function mayCheckAccounting($av_id=''){ return VM_observer_here; }
  
  /*
   * Show "the access matrix" (who may do what)
   */
  public function show(){
    locateAndInclude('bForm_Avatar_vm');
    parent::show();
  }

  /*
   * Get "the access matrix" (who may do what)
   */
  private function getRanks_matrix($av_id=0){

    if (empty($av_id)) $av_id = @bAuth::$av->ID;
    if (empty($av_id)) return array();
    
    bForm_Avatar::set_context(bAuth::$av,VM_MODULE);
    
    // $_SESSION[myPear_cache][__METHOD__][$av_id] = Null;
    if (@$_SESSION[myPear_cache][__METHOD__][$av_id] === Null){
      
      // Check the organization_role
      $organizational_duty = myPear::org_duty($av_id,array(RANK__superuser,
							   RANK_vm_manager,
							   RANK_vm_endorser,
							   RANK_vm_booker,
							   RANK_vm_observer,
							   ));
      
      // Walk thru all the recent events 
      locateAndInclude('bForm_vm_Event');
      foreach(bForm_vm_Event::getEvents(array("e_start > ".(time() - AUTO_EXPIRATION * 86400))) as $e_id){
	$rank = $organizational_duty;	  
	if (!$rank){
	  
	  // RANK_vm_registrant - Is he a registrant for any event, including those in the past?
	  if (VM::isRegistrant($av_id,Null))  $rank = max($rank,RANK_vm_registrant);
	  
	  // RANK_vm_organizer, Is the user an organizer for this event?
	  if (VM::isOrganizer($av_id,$e_id))  $rank = max($rank,RANK_vm_organizer);
	}
	
	// Fill list of positions which might be browsed
	if (!empty($rank)){                         
	  $_SESSION[myPear_cache][__METHOD__][$av_id][$e_id] = $rank;
	} 
      }
    }
    if (empty($_SESSION[myPear_cache][__METHOD__][$av_id]))$_SESSION[myPear_cache][__METHOD__][$av_id] = array();
    return    $_SESSION[myPear_cache][__METHOD__][$av_id];
  }
}
