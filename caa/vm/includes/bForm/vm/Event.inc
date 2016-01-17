<?php
locateAndInclude('bForm_vm');

class bForm_vm_Event extends bForm_vm {
 
  var $TABLE   = 'abs_events';
  var $TABLEID = 'e_id';

  public static function getEvents($where=array(1)){
    $reply = array();
    $where[] = "e_orgid = ".myOrg_ID;
    $q = myPear_db()->query("SELECT e_id FROM abs_events WHERE ".join(' AND ',$where)." ORDER BY e_start DESC");
    while($r = myPear_db()->next_record($q)) $reply[] = $r['e_id'];
    return $reply;
  }
  
  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }
  
  public function get_e_v_policy(){
    // Initialise policies for the event if not yet done
    if (!$this->getValue('e_v_policy')) $this->set_e_v_policy(VM::$description[VISIT_TYPE_PROGRAM]['p']);
    // the policy is a serialized array
    $reply = b_fmt::safe_decode($this->getValue('e_v_policy'));
    $this->dbg($reply,True);
    return $reply;
  }

  public function set_e_v_policy($e_v_policy){
    if (is_array($e_v_policy) && !empty($e_v_policy)){
      $this->updateDB(array('e_v_policy' => b_fmt::safe_encode($e_v_policy)));
    }
  }

  /*
   *
   */
  private $budgetSource = Null;
  function budgetSource(){
    if ($this->budgetSource === Null){
      locateAndInclude('bList_vm_budgetSource');
      $this->budgetSource = new bList_vm_budgetSource($this);
    }
    return $this->budgetSource;
  }

  /*
   *
   */
  private $socialEvents = Null;
  function socialEvents(){
    if ($this->socialEvents === Null){
      locateAndInclude('bList_vm_socialEvents');
      $this->socialEvents = new bList_vm_socialEvents($this);
    }
    return $this->socialEvents;
  }

  /*
   *
   */
  private $socialEventRates = Null;
  function socialEventRates(){
    if ($this->socialEventRates === Null){
      locateAndInclude('bList_vm_socialEventRates');
      $this->socialEventRates = new bList_vm_socialEventRates($this);
    }
    return $this->socialEventRates;
  }

  /*
   *
   */
  private $reimbursementRates = Null;
  function reimbursementRates(){
    if ($this->reimbursementRates === Null){
      locateAndInclude('bList_vm_reimbursementRates');
      $this->reimbursementRates = new bList_vm_reimbursementRates($this);
    }
    return $this->reimbursementRates;
  }

  /*
   *
   */
  private $accommodationOptions = Null;
  function accommodationOptions(){
    if ($this->accommodationOptions === Null){
      locateAndInclude('bList_vm_accommodationOptions');
      $this->accommodationOptions = new bList_vm_accommodationOptions($this);
      $this->accommodationOptions->update();
    }
    return $this->accommodationOptions;
  }

  /*
   *
   */
  function defineVariables() {
    $this->defineVariable(array('Title'      => 'e_name',
				'Agenda code'=> 'e_code',
				'Start date' => 'e_start',
				'End date  ' => 'e_end',
				'Policy'     => 'e_policy'));
    
    $this->defineTypes(array('isMBE'=>array('e_start',
					    'e_end')));
  }
  
  /*
   * Check all the visits associated with the event and return the list of 
   * those which do not have visit formalities completed.
   * The formalities are defined by the Visits class
   */
  function formalitiesNotCompleted_OBSOLETE($nedodelka=Null){
    locateAndInclude('bForm_vm_Visit');
    $reply = array();
    $q = myPear_db()->query("SELECT * FROM $this->TABLE LEFT JOIN abs_visits ON v_eid = e_id WHERE e_id = $this->ID");
    while($r = myPear_db()->next_record($q)){
      if ($n = bForm_vm_Visit::formalitiesNotCompleted($r)){
	if (empty($nedodelka) || $n == $nedodelka) $reply[$r['v_id']] = $n;
      }
    }
    return $reply;
  }

  /*
   * Get list of attenders for the given date
   */
  function attenders($only_approved=False){
    return self::_attenders($this->ID,$only_approved);
  }

  static function _attenders($e_id,$only_approved=False){
    $where = array("e_id = $e_id");
    if ($only_approved) $where[] = "v_status = '".STATUS_YES."'";
    $q = myPear_db()->query("SELECT v_avid FROM abs_visits LEFT JOIN abs_events ON e_id = v_eid WHERE ".
			    join(' AND ',$where));
    $reply = myPear_db()->num_rows($q);
    return $reply;
  }

  function getVisit_ID($av_id){
    $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE v_avid = '$av_id' AND v_eid = ".$this->ID);
    while ($r = myPear_db()->next_record($q)) $reply = $r['v_id'];
    return $reply;
   }

  /*
   * Get list of attenders for the given date
   */
  function attendersToday($date=Null){
    if (empty($date)) $date = b_time::noon();
    $date = b_time::_($date);
    $q = myPear_db()->query("SELECT v_avid FROM abs_visits LEFT JOIN abs_events ON e_id = v_eid WHERE ".
			      join(' AND ',array("e_id = $this->ID",
						 "v_start <= $date",
						 "v_end   >= $date",
						 "v_status = '".STATUS_YES."'",
						 )));
    $reply = array();
    while ($r = myPear_db()->next_record($q)) $reply[] = $r['v_avid'];
    return $reply;
  }

  /*
   * Check that the avatar is the event organizer
   */
  function isOrganizer($av_id=NULL){
    return VM::isOrganizer($av_id,$this->ID);
  }

  /*
   * Get organizers list
   */
  function getOrganizers(){
    return VM_organizers($this->ID)->getOrganizers();
  }

  function getForm(){
    $this->dbg();
    
    $this->defineVariable(array('_virt_period'=>' ',
				'_virt_url'   =>' '),True);
    $this->formDB['_virt_period'] = b_time::period($this->getValue('e_start'),$this->getValue('e_end'));
    $this->formDB['_virt_url']    = "http://agenda.albanova.se/conferenceDisplay.py?cat=".$this->getValue('e_code');
    if ($this->getValue('e_code')>0){ // The events from AGENDA are not edit-able
      foreach(array('e_name','e_start','e_end','e_code','_virt_period','_virt_url') as $i) $this->isRO[$i] = True;
    }    
    
    myPear::H1($this->nameSimple());
    $this->getFormOpen();    
    $this->formBlock("",' ',
		     array('e_name'       => array('textField'),
			   '_virt_period' => array('datePickerField2','e_start','e_end'),
			   '_virt_url'    => array('textField'),
			   ));
    
    if (1) $this->formBlock("",'Policy',
			    array('i'=>'i-bundle','d'=>btn_edit_policy,'l'=>b_url::same("?form=vm_Event&id=$this->ID&mode=RW")));
    
    $submit= array();
    if ($this->MODE=='RO') {
      if ($this->isWritable()) $submit['modify event policy'] = b_url::same(array('form'=>'vm_Event',
										  'mode'=>'RW',
										  'id'  =>$this->ID));
      $submit['continue'] = b_url::getReferrer();
    }
    $this->closeForm('submit',$submit);
  }

  /*
   * Rounded by 1000 SEK.
   *
   * The long conferences (called "programs") get budget proportional to the conference duration,
   * while the short conferences (called sometimes "workshops") negotiate the budget.
   */
  function get_defaultEventBudget($monthly=False){
    $default_budget = ($this->nDays() > VM_ev_budget_TBD_limit 
		       ? ($monthly 
			  ? VM_conferenceBudget
			  : 1000 * (int)((VM_conferenceBudget_week * $this->nWeeks() + 500.) / 1000.))
		       : 0);
    return $default_budget;
  }

  function nDays(){
    return (int)((b_time::_($this->getValue('e_end')) - b_time::_($this->getValue('e_start')))/86400);
  }

  function nWeeks(){
    switch($nDays = $this->nDays()){
    case 0:
    case 1:
      $reply = 0.15;
      break;
    case 2:
      $reply = 0.3;
      break;
    default:
      $reply = (int)($nDays / 7. + 0.5);
    }
    return $reply;
  }

  public function isEventEndorsed(){
    $reply = $this->getPolicy(VM_E_endorsed);
    return $reply;
  }

  public function isArchived(){
    return $this->getPolicy(VM_E_archived);
  }

  /*
   * Same as the parent, but asIs is always "True"
   */
  public function getValue($name, $asIs=True, $d_MBE=True){
    return parent::getValue($name, $asIs, $d_MBE);
  }

  /*
   * Get policy
   */
  private $getPolicy = array();
  function getPolicy($policy){
    if (@$this->getPolicy[$policy] === Null) $this->getPolicy[$policy] = (bool)self::_getPolicy($policy,
												array('e_policy'=>$this->getValue('e_policy'),
												      'e_end'   =>$this->getValue('e_end')),
												(bool)$this->debug);
    return $this->getPolicy[$policy];
  }
  
  /*
   * Executor for the getPolicy
   */
  static function _getPolicy($policy,$buffer=array(),$debug=False){
    // Guess the policy if it is not set.
    // As a sanity, lock the events in the past for which the policy was not defined.
    if (empty($buffer['e_policy'])){
      switch($policy){
      case VM_E_endorsed:
	break;
	
      case VM_E_archived:
	if ($buffer['e_end'] < b_time::noon() - AUTO_EXPIRATION*86400){
	  $buffer['e_policy'] = VM_E_archived;
	}
	break;
	
      default:
	if (empty($buffer['e_policy']))  $buffer['e_policy'] = 0;
      }
    }

    $reply = b_mask::get($buffer['e_policy'], $policy);
    if ($debug||cnf_show_pe) myPear::DEBUG(__METHOD__."($policy) <em>".VM::$e_policies[$policy]['d'].($reply?' yes':' no').'</em>');
    return  (bool)$reply;
  }

  /*
   *
   */
  function setPolicy($policy,$value,$verbose=True){
    switch(in_array($policy,array(VM_E_endorsed,VM_E_endorsmentSent)) || !$this->isEventEndorsed()){
    case False:
      myPear::WARNING(x('i',$this->name()."'s record is LOCKED, \"".VM::$e_policies[$policy]['d']."\" was not changed"));
      break;
      
    case True:
      $isChanged = $this->setBitMask($policy,'e_policy',$value);
      if ($verbose && $isChanged){
	myPear::MESSAGE(x('i',$this->name().": ").VM::$e_policies[$policy]['d']."  ".($value?'YES':'NO'));
      }
    }
  }

  function nameSimple(){
    return $this->name(False,40);
  }

  /*
   *
   */
  public static function _name($formDB,$includeDate=True,$max_length=999){
    $reply = (($formDB['e_code'] > 0) && $includeDate
	      ? date('M Y - ',$formDB['e_start']) . b_fmt::escape(strip_tags($formDB['e_name']))
	      : $formDB['e_name']);
    return b_fmt::strcut($reply,$max_length,'...');
  }

  /*
   * Event name with optional start date
   */
  function name($includeDate=True,$max_length=999){
    if ($this->ID == EVT_ANY)  return "All Events";
    else return self::_name(array('e_code' => $this->getValue('e_code'),
				  'e_start'=> $this->getValue('e_start'),
				  'e_name' => $this->getValue('e_name')),
			    $includeDate,$max_length);
  }

  function period($formatted=False,$margin=0){
    if ($formatted) return array(date('d M Y',$this->formDB['e_start']-$margin*86400),date('d M Y',$this->formDB['e_end']+$margin*86400));
    else            return array($this->formDB['e_start']-$margin*86400,$this->formDB['e_end']+$margin*86400);
  }

  // the agenda events are not writable
  function isWritable() {
    return False;
  }

  function formOKspecific() {
    $e_start = b_time::txt2unix($this->getValue('e_start'));
    $e_end   = b_time::txt2unix($this->getValue('e_end'));
    foreach (array('e_start','e_end') as $f)
      if (date('Y',$$f) < 2000)
	$this->errorsInTheForm[$f] = "Date should be YYYY-MM-DD";
    if (b_time::diff($e_end, $e_start) <= 0)
      $this->errorsInTheForm['e_end'] = "<i>End date</i> must be after the <i>start date</i>";
    if (!$this->errorsInTheForm) {
      foreach (array('e_start','e_end') as $f)
	$this->formDB_modified[$f] = $$f;
    }

    // set the negative code for the manually created events
    if (!$this->getValue('e_code')) {
      $r = myPear_db()->next_record(myPear_db()->query("SELECT MIN(e_code) AS min_code FROM $this->TABLE"));
      if ((int)$r['min_code'] > 0) $r['min_code'] = 0;
      $this->formDB_modified['e_code'] = $r['min_code'] - 1;
    }
    if (!$this->errorsInTheForm)
      if (is_object(APIaccess::$cache))  APIaccess::$cache->reset();
  }
}
