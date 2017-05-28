<?php
/*
 * Event class
 */
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

    // Follow changes in the policy definition
    if ($this->getValue('e_end') < time()) $this->set_e_v_policy(VM::$description[VISIT_TYPE_PROGRAM]['p']);
  }

  /*
   *
   */
  function get_duplicate_attenders(){
    $this->dbg(starting);
    $q = myPear_db()->qquery("SELECT COUNT(*) AS n, av_firstname, av_lastname, v_avid FROM abs_visits ".
			     " LEFT JOIN zzz_avatars ON av_id = v_avid ".
			     " WHERE v_eid=".$this->ID.
			     " GROUP BY av_id ".
			     " HAVING n>1",cnf_dev);
    while($r = myPear_db()->next_record($q)){
      $list["$r[n] applications:"][] = bForm_Avatar::_fmtName('Lf',$r);
    }
    if (!empty($list)){
      $msg[] = x('strong','Found duplicate applications for the event');
      foreach($list as $n=>$names){
	$msg[] = $n;
	sort($names);
	foreach($names as $a) $msg[] = x('em',$a);
      }
      myPear::ERROR(join('<br>',$msg));
    }
    $this->dbg(exiting);
  }

  /*
   *
   */  
  public function get_e_v_policy(){
    // Initialise policies for the event if not yet done
    if (!$this->getValue('e_v_policy')) $this->set_e_v_policy(VM::$description[VISIT_TYPE_PROGRAM]['p']);
    // the policy is a serialized array
    $reply = b_fmt::safe_decode($this->getValue('e_v_policy'));
    sort($reply,SORT_NUMERIC);
    $this->dbg($reply);
    return $reply;
  }

  public function set_e_v_policy($e_v_policy){
    if (is_array($e_v_policy) && !empty($e_v_policy)){
      $this->updateDB(array('e_v_policy' => b_fmt::safe_encode($e_v_policy)));
    }
  }

  /*
   * Dialog to set the event policies 
   */
  public function show_policies(){
    print x('h2','Event policy');

    $this->attenders(0,1);

    // Togle policy if requested
    if ($p = @$_GET['toggle_once']) $this->setPolicy($p,(int)!$this->getPolicy($p),True);
    
    // Print policies
    $t = new b_table_zebra(array('_i'=>' ','_d'=>' '));
    $t->showLineCounter = False;
    $t->class['_i'] = 'b_transparent';
    foreach(VM::$e_policies as $policy=>$p){
      if (@$p['t'] === auto_maintained) continue;
      if ($policy  == VM_E_archived){
	if (time() < $this->getValue('e_end')+3600*24) continue;	
      }elseif ($policy == VM_E_endorsed){
	if (!$this->mightBeEndorced(False)) continue;
	$t->tr(); $t->tr();
	$i_on  = 'i-lock_on';
	$i_off = 'i-lock_off';
      }else{
	$i_on  = 'i-finger_up';
	$i_off = 'i-finger_down';
      }
      if (($policy !== VM_E_archived) && $this->isArchived()){
	myPear::MESSAGE("This event is arhived");
	continue;
      }

      $on_off = $this->getPolicy($policy);
      $t->extraTD[] = ($this->show_policies_isWritable($policy)
		       /*
		       ? b_btn::submit(($on_off ? 'YES' : 'NO').", click to change",
				       $url=b_url::same("?confirm_yes&resetcache_once=1&toggle_once=$policy"), 
				       $confirm=True, 
				       $class='page-load-progress',
				       $img=bIcons()->get($on_off ? $i_on : $i_off))
		       */
		       ? bIcons()->getButton(array('class'=>'page-load-progress',
						   'i'=>($on_off ? $i_on : $i_off),
						   //'c'=>($on_off ? '' : 'opacity_on'),
						   'd'=>($on_off ? 'YES' : 'NO').", click to change",
						   'l'=>b_url::same("?resetcache_once=1&toggle_once=$policy")))
		       : bIcons()->get(array('i'=>($on_off ? $i_on : $i_off),
					     //'c'=>($on_off ? '' : 'opacity_on'),
					     'd'=>($on_off ? 'YES' : 'NO'))));
      $t->prt(array(//'_i'=>(($policy == VM_E_endorsed) ? '' : bIcons()->get($p['i'])),
		    '_d'=>x('h4',$p['d'])));
    }
    $t->close();
  }

  /*
   * Helper for show_policies
   */
  private function show_policies_isWritable($policy){
    if ($this->getValue('e_start') < time()) return VM_manager_here;
    switch($policy){
    case VM_E_archived:          return superUser_here; 
    case VM_E_endorsed:          return $this->mightBeEndorced(True);
    case VM_E_letOrgSetAccPrices:return VM_administrator_here;
    case VM_E_payTrip:
    case VM_E_provideOffice:   
    case VM_E_provideLunches:
    case VM_E_conferenceDinner:  return !$this->getPolicy(VM_E_endorsed) && VM_administrator_here;
    default:
      b_debug::internalError("Unknown option '$policy'");
    }
    return False;
  }

  /*
   *
   */
  public function mightBeEndorced($checkRights=True){
    $this->budgetSource()->get_budgetSource($default_only=False,$verbose=False);
    $mightBeEndorced = (!$this->isArchived() &&
			($this->getValue('e_reg_end') < time()) && 
			($this->getValue('e_end') > time()) &&
			($this->budgetSource()->total > 0) &&
			($checkRights
			 ? (VM_organizer_here || VM_manager_here || VM::hasRightTo('endorse_event'))
			 : True));
    $this->dbg($mightBeEndorced);
    return $mightBeEndorced;
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
  private $attenders = array();
  function attenders($only_approved=False,$warning_if_no=False){
    if (!isset($this->attenders[$only_approved])){
      $this->attenders[$only_approved] = self::_attenders($this->ID,$only_approved);
    }
    $attenders = $this->attenders[$only_approved];
    if (empty($attenders) && $warning_if_no) myPear::WARNING(x('strong',"Note, there is no applicants for this event yet..."));
    $this->dbg($attenders,True);
    return $attenders; 
  }
  
  /*
   * return list visits if $only_approved === 'v_id'
   * otherwise return True/False
   */
  static function _attenders($e_id,$only_approved=False){
    $where = array(myPear_db()->quote($e_id,1,'v_eid'));
    if ($only_approved) $where[] = myPear_db()->quote(STATUS_YES,1,'v_status');
    $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE ".join(' AND ',$where));
    if ($only_approved === 'v_id'){
      $reply = array();
      while ($r = myPear_db()->next_record($q)) $reply[] = $r['v_id'];
    }else{
      $reply = myPear_db()->num_rows($q);
    }
    b_debug::xxx($reply);
    return $reply;
  }

  /*
   *
   */
  function getVisit_ID($av_id){
    $reply = Null;
    $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE v_avid = '$av_id' AND v_eid = ".$this->ID);
    while ($r = myPear_db()->next_record($q)) $reply = $r['v_id'];
    return $reply;
   }

  /*
   * Get list of attenders for the given date, ignoring the time
   */
  private $attendersToday = array();
  function attendersToday($date=Null){
    if (empty($date)) $date = b_time::noon();
    $date = date('Ymd',$date);
    if (empty($this->attendersToday[$date])){
      $q = "SELECT v_avid FROM abs_visits LEFT JOIN abs_events ON e_id = v_eid WHERE ".
	join(' AND ',array("e_id = $this->ID",
			   "DATE_FORMAT(FROM_UNIXTIME(v_start),'%Y%m%d') <= '$date'",
			   "DATE_FORMAT(FROM_UNIXTIME(v_end)  ,'%Y%m%d') >= '$date'",
			   "v_status = '".STATUS_YES."'"));
      $q = myPear_db()->qquery($q,cnf_dev);
      $this->attendersToday[$date] = array();
      while ($r = myPear_db()->next_record($q)) $this->attendersToday[$date][] = $r['v_avid'];
    }
    return $this->attendersToday[$date];
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
  function getPolicy($policy){
    return (bool)self::_getPolicy($policy,
				  array('e_policy'=>$this->getValue('e_policy'),
					'e_end'   =>$this->getValue('e_end')),
				  (bool)$this->debug);
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
   *   VM_E_endorsed       Flag 'event is endorsed and locked, no changes allowed'
   *   VM_E_archived       Flag 'event is archived'. Has precedence over other locks
   *   VM_E_endorsmentSent Flag 'mail to organizers "event is endorsed" is sent'
   *   VM_E_provideOffice  Flag 'assign offices for the event'
   */
  function setPolicy($policy,$value,$verbose=True){
    switch($policy){
    case VM_E_archived:
      if ($value) $this->setPolicy(VM_E_endorsed,1,False);
      if ($value) $this->setPolicy(VM_E_endorsmentSent,1,False);
      break;

    case VM_E_provideOffice:
      if ($this->isEventEndorsed()){
	myPear::WARNING(x('i',$this->name()."'s record is LOCKED, \"".VM::$e_policies[$policy]['d']."\" was not changed"));
	return;
      }
    }
    
    $isChanged = $this->setBitMask($policy,'e_policy',$value);

    if ($isChanged){
      if ($verbose) myPear::MESSAGE(x('i',$this->name().": ").VM::$e_policies[$policy]['d']."  ".($value?'YES':'NO'));
      
      // Change the office policy for all visits
      if ($policy == VM_E_provideOffice){
	foreach(self::_attenders($this->ID,'v_id') as $v_id){
	  $v = myPear::getInstance_new('bForm_vm_Visit',$v_id,'fatal');
	  $v->setPolicy(VM_V_provideOffice,$value,$verbose);
	}
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

  /*
   * Return the event start/end dates
   */
  function period($formatted=False,$margin=0){
    return ($formatted
	    ? array(date('d M Y',$this->formDB['e_start']-$margin*86400),
		    date('d M Y',$this->formDB['e_end']  +$margin*86400))
	    : array($this->formDB['e_start']-$margin*86400,
		    $this->formDB['e_end']  +$margin*86400));
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
