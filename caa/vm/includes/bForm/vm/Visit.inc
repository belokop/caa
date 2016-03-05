<?php
/* 
 * The visit type might be:
 * - VISIT_TYPE_COLLABORATION (Visit)
 * - VISIT_TYPE_PROGRAM       (Program/Conference registrant)
 * - VISIT_TYPE_RENT          (Renting out apartment, the tenant to be invoiced)
 * - VISIT_TYPE_OTHER
 * - VISIT_OUT_OF_SCOPE
 */

locateAndInclude('bForm_vm');
class bForm_vm_Visit extends bForm_vm {

  var $TABLE  = 'abs_visits';
  var $TABLEID= 'v_id';
  var $maxStay= 666;
  var $title  = 'visit request';
  var $av = Null;
  var $e  = Null;
  private $exp = Null;

  public  $need_av_residentship = Null;
  public  $av_host = Null;

  /*
   * Get the visit instance
   */
  public static function getVisit($av_id,$e_id){
    $reply = Null;
    $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE v_avid='$av_id' AND v_eid='$e_id'");
    while ($r = myPear_db()->next_record($q)){
      $reply = new bForm_vm_Visit($r['v_id']);
    }    
    return $reply;
  }


  /*
   * Get the avatar visits 
   */
  public static function getVisits($av_id,$v_types=array()){
    // Optionally select by the visit type
    foreach($v_types as $v_type)      $w[] = "v_type = '$v_type'";
    if (empty($w)) $w = array(1);

    // Send the query
    $reply = array();
    $q = myPear_db()->query("SELECT v_id,v_eid FROM abs_visits WHERE ".join(' AND ',array("v_avid = '$av_id'",
											  '('.join(' OR ',$w).')')));
    while ($r = myPear_db()->next_record($q)){
      $reply[] = (in_array(VISIT_TYPE_PROGRAM,$v_types) 
		  ? $r['v_eid']
		  : $r['v_id']);
    }
    return $reply;
  }

  /*
   * Check does the avatar have visit with the active policy,
   * say, with payed trip 
   */
  public static function is_visitPolicyOn($av_id,$policy){
    $reply = False;
    if (!is_array($policy)) $policy = array($policy);
    $q = myPear_db()->query("SELECT v_policy,av_lastname FROM abs_visits LEFT JOIN zzz_avatars ON v_avid=av_id WHERE v_avid = '$av_id' GROUP BY v_policy");
    while($r = myPear_db()->next_record($q)){
      foreach($policy as $p){
	if (self::_getPolicy($p,$r)){
	  $reply = True;
	  break 2;
	}
      }
    }
    return $reply;
  }


  /*
   * Constructor
   */
  function __construct($id, $mode='RO') {
    // Ask the parent...
    parent::__construct($id, $mode);

    // Hook the Avatar & Event which he is attending
    $this->hookDependancies();
    
    // Follow the endorser will
    if (isset($_GET['a_once'])){
      $this->show_gv = cnf_dev;
      $this->dbg("Accepted? ".$_GET['a_once'],cnf_dev);
      $this->setStatus($_GET['a_once']);
      if ($_GET['a_once'] == STATUS_PENDING) $_SESSION[get_class($this).$this->ID] = '...';
    }
  }
  
  /*
   *
   */
  function __clean_cache(){
    if (is_object($this->av))          $this->av->__clean_cache(); 
    if (is_object($this->exp))         $this->exp->__clean_cache(); 
    foreach($this->get_allLeases(True) as $l) $l->__clean_cache();
    myPear::getInstance(get_class($this),$this->ID,'clean');
  }

  /*
   * $this->exp is declared private to avoid the endless loop
   * bForm_vm_Visit <-> bForm_vm_Expenses
   */
  function getExp(){
    $this->hookDependancies();
    return $this->exp;
  }

  function setExp(&$exp=Null){
    if ( is_object($exp) && !is_object($this->exp)){
      $this->exp = $exp;
      $this->exp->defineVariables();
      $this->mergeDefinitions($this->exp);
      $this->hookDependancies();
    }
  }

  public static function getVisitorInfo($av_id,$e_id,$column){
    if (myPear_db()->columnExists($column,'abs_visits')){
      $q = myPear_db()->query("SELECT $column FROM abs_visits ".
			      " WHERE v_eid = '$e_id' AND v_avid = '$av_id'");
      while($r = myPear_db()->next_record($q)) $reply = $r[$column];
      return @$reply;
    }else{
      myPear::WARNING_T('does not exist');
      return Null;
    }
  }

  /*
   * Get the visit id for an avatar
   */
  public static function getID($av_id,$e_id){
    return self::getVisitorInfo($av_id,$e_id,'v_id');
  }

  /*
   * Get the list of the applicable policies for this visit,
   * auto-correct the bogus combinations - like "denial letter sent", but the visit is not canceled.
   * For the list of applicable policies see the config file.
   */
  function getPolicies(){
    $this->hookEvent();
    $v_type = $this->getValue('v_type',True);
    return self::_getPolicies(array('e_v_policy'=> VM_visit_policies($v_type,$this->e),
				    'v_type'    => $v_type,
				    'v_eid'     => $this->getValue('v_eid',True)));
  }

  /*
   *
   */
  static function _getPolicies($buffer){
    if (!is_array($buffer['e_v_policy'])) b_debug::internalError("'e_v_policy' is not an array");
    $policies = array();
    $v_type = $buffer['v_type'];
    foreach($buffer['e_v_policy'] as $p){
      if (empty(VM::$v_policies[$p])){
	if (cnf_dev) myPear::DEBUG(__METHOD__."() - Get unknown policy '$p', ignore...");
      }else{
	if (!cnf_dev) if ($p == VM_V_payPerdiem) continue;
	if (!cnf_dev) if ($p == VM_V_payOther)   continue;
	$policies[$p] = VM::$v_policies[$p];
      }
    }
    return $policies;
  }

  /*
   * RegExp search thru GSF
   */
  function getGSF($what='travel.*support',$li='<li>'){
    $gsf = self::_getGSF($what,array('v_gsf'=>$this->getValue('v_gsf',True)),$li);
    if (empty($gsf))$reply = '';
    else  $reply = (empty($li)
		    ? join('<br/>',$gsf)
		    : x('ul',str_replace('<br/>',' ',join(' ',$gsf)))); 
    return $reply;
  }

  public static $_getGSF = array();
  static function _getGSF($what='travel.*support',$formDB,$li=''){
    self::$_getGSF = $reply = array();
    if (!empty($formDB['v_gsf'])){
      foreach(unserialize($formDB['v_gsf']) as $k=>$v){
	if (preg_match("/$what/i",$k)){
	  self::$_getGSF[$what] = $v;
	  if (in_array($v,array('yes','on'))) $reply[] = $k;
	  else                                $reply[] = (empty($li) ? "$k? $v" : "$k<br/> <em>$v</em>");
	}
      }
    }
    if (empty($li)){
      return $reply;
    }else{
      $reply_html = array();
      foreach($reply as $i) $reply_html[] = x($li,$i);
      return $reply_html;
    } 
  } 

  /*
   * Get the value for the given policy
   * (i.e. is the letter sent, should the per-diem to be payed, etc.)
   *
   * The policy might be 'locked', 
   * which means that all the decisions about the visit are signed,
   * no more changes is allowed.
   *
   * @param $policy - (int) policy code
   * @param $onlyIfExpected (optional)
   * @return - (bool) True/Force if the policy is set/not_set
   *           Null if the $onlyIfExpected is set and the policy is not expected for the visit
   */
  function getPolicy($policy,$forced=Null){
    if (is_bool($forced)) $this->setPolicy($policy,$forced);
    $reply = ((int)$this->ID && $this->hookAvatar()
	      ? self::_getPolicy($policy,0,$onlyIfExpected=False,$this)
	      : Null);
    return $reply;
  }
  
  /*
   *
   */
  static function _getPolicy($policy,$buffer,$onlyIfExpected=False,$visit=Null){ 

    if (!in_array($policy,array_keys(VM::$v_policies)))      return False;
    
    if (is_object($visit)){
      $buffer = array('id'         => $visit->__toString().'->',
		      'v_policy'   => $visit->getValue('v_policy',True),
		      'av_lastname'=> $visit->av->getValue('av_lastname',True,True));
    }elseif(!is_array($buffer)){
      $buffer = array('id'         => '',
		      'av_lastname'=> 'anonymous',
		      'v_policy'   => $buffer);
    }
    if (empty($buffer['v_policy'])){
      $reply = False;
    }else{
      $reply = (bool)b_mask::get($buffer['v_policy'],$policy);
      if(cnf_show_pv || ($onlyIfExpected === 'verbose')){
	myPear::DEBUG(@$buffer['id'].__FUNCTION__."($buffer[av_lastname],$policy) ".x('i',VM::$v_policies[$policy]['d'].($reply?' YES ':' NO  ')));
      }
    }
    return $reply;
  }
  

  /*
   * Set the visit policy, which is a bitwise integer.
   */
  private $setPolicyRecursion = 0;
  function setPolicy($policy,$value,$verbose=True,$force=False,$recursion_ok=False){

    if (!in_array($policy,array_keys(VM::$v_policies)))      return;

    if (!$recursion_ok && $this->setPolicyRecursion++){
      if (cnf_dev) b_debug::traceBack($this->setPolicyRecursion);
      return;
    }

    // Sanity, fix eventual mistypes of the organizers
    if ($this->getPolicy(VM_V_denialMailSent)  && !$this->isDenied())   $this->setPolicy(VM_V_denialMailSent, 0,cnf_dev,False,True);
    if ($this->getPolicy(VM_V_welcomeMailSent) && !$this->isApproved()) $this->setPolicy(VM_V_welcomeMailSent,0,cnf_dev,False,True);
    
    // Now set the requested policy
    if (!(VM_booker_here||VM_organizer_here) || 
	(@VM::$v_policies[$policy]['t'] === auto_maintained) ||
	($this->getValue('v_end') < time())) $verbose = False;
    
    $isChanged = $this->setBitMask($policy,'v_policy',$value,$check_only=True);
    if ($isChanged){
      if (!$force && $this->exp_arePayed()){
	if ($verbose || cnf_show_pv) myPear::WARNING(x('i',$this->av->name()."'s record is LOCKED, <em>".VM::$v_policies[$policy]['d']."</em> is not changed"));
      }else{      
	$this->setBitMask($policy,'v_policy',$value);
	if ($verbose || cnf_show_pv){
	  myPear::MESSAGE_add(VM::$v_policies[$policy]['d']." ".x('em',($value?'YES':'NO')." for ".$this->av->fmtname()));
	}
      }
    }
    $this->setPolicyRecursion = 0;
  }
  
  /*
   * Set the status
   */
  public function setStatus($v_status=Null){
    $was=$this->getValue('v_status');
    switch($v_status){
    case STATUS_NO:
      unset($_POST['lease_id']);
      $this->deleteBooking(Null,$verbose=True);
      // cancel all the scholarships
      foreach (VM::$reimbursements as $p) $this->setPolicy($p,0,True);
      
    case STATUS_YES:
      // Send informational letter to the administrators
      foreach(VM_bookers()->membersOnDuty(VISIT_TYPE_COLLABORATION) as $s_name=>$s_email){
	VM_mailer()->inform_secretary_about_visit($this,$s_email,$v_status,bAuth::$av->fmtName('fl'));
      }
      break;
      
    case STATUS_PENDING:
      // cancel all the scholarships
      foreach (VM::$reimbursements as $p) $this->setPolicy($p,0,True);
      break;

     default:
       myPear::WARNING_T("Unexpected status '$v_status'",$this);
       return;
     }
    if ($was != $v_status){
      $this->updateDB(array('v_status' => $v_status));
      $this->getValue_reset('v_status',$v_status);
      $this->dbg("v_status $was->$v_status",(bool)($this->debug || cnf_show_pv || cnf_dev));
    }
   }

   /*
    * get the visit status
    */
   public function getStatus(){

     // Copy the formDB to an array
     $buffer = $this->formDB;
     if ($this->hookEvent()){
       foreach($this->e->formDB as $k=>$v) $buffer[$k] = $v;
     }

     // Save the old status value (see below)
     $v_status = @$buffer['v_status'];
     $buffer['v_status'] = $this->getValue('v_status',True);

     $reply = self::_getStatus($buffer);
     if ((int)$this->ID)    $this->setStatus($reply);

     switch($reply){
     case STATUS_YES:
       // Set also flag 'the approval formalities are done' if the status is auto-set to 'accepted'
       if ((int)$this->ID && empty($v_status)) $this->setPolicy(VM_V_welcomeMailSent,True,False);
       break;
     case STATUS_PENDING:
     case STATUS_NO:
     }
     $this->dbg($reply,(bool)(cnf_show_pv||$this->debug));    
     return $reply;
   }

   /*
    *
    */
   static public function _getStatus($buffer=array(),$check_clashes=False){

     // Skip the visits which are not yet in the database
     if ((int)@$buffer['v_id']){
       if ($check_clashes && self::_getClashes($buffer,$verbose=False)){
	 $v_status = STATUS_CLASH;
       }else{

	 // Guess the status value if it is not defined yet,
	 // make an auto-approval for the transition period and for the past events
	 $v_status = $buffer['v_status'];
	 $v_start  = $buffer['v_start'];

	 if (empty($v_status)){
	   switch($buffer['v_type']){
	   case VISIT_TYPE_RENT:  
	     $v_status = STATUS_YES;   
	     break;
	   case VISIT_TYPE_COLLABORATION:
	   case VISIT_TYPE_PROGRAM:  
	   case VISIT_TYPE_OTHER: 
	     $v_status = STATUS_PENDING;
	     break; 
	   }
	 }
       }
     }else{
       $v_status = STATUS_PENDING;
     }
     return $v_status;
   }

   /*
    *
    */
   private $status_data = array();
   private function is_status_data(){
     if ((int)$this->ID && empty($this->status_data)){
       $this->status_data['v_id'] = $this->ID;
       foreach(array('v_status',
		     'v_start',
		     'v_type',
		     'e_start') as $field){
	 $this->status_data[$field] = $this->getValue($field,True,True);
       }
     }
     return $this->status_data;
   }

   function isApproved(){
     if ((int)$this->ID) $reply = self::_isApproved($this->is_status_data());
     else                $reply = True;
     $this->dbg($reply);
     return $reply;
   }

   static function _isApproved($buffer){
     $reply = (self::_getStatus($buffer)== STATUS_YES);
     return $reply;
   }

   /*
    *
    */
   function isDenied(){
     $reply = self::_isDenied($this->is_status_data());
     $this->dbg($reply);
     return $reply;
   }

   static function _isDenied($buffer){
     $reply = (self::_getStatus($buffer) == STATUS_NO);
     return $reply;
   }

   /*
    *
    */
   function isPending(){
     $reply = (self::_getStatus($this->formDB)== STATUS_PENDING);
     return $reply; 
   }


   /*
    * Hook all the leases (bookings) for this visit
    */
   private $_allLeases = array(); // list of 'lease' objects
   function get_allLeases($reset=False,$mode='classes'){
     if ($reset || (@$this->_allLeases[$mode] === Null)){
       $this->_allLeases[$mode] = self::_v_get_allLeases($this,$mode);
     }
     return $this->_allLeases[$mode];
   }

   /*
    *
    */
   static function _v_get_allLeases($v_id,$mode='hut_codes'){
     $reply = array();
     if (!empty($v_id)){
       locateAndInclude('bForm_vm_Lease');
       foreach(bForm_vm_Lease::_get_allLeases($v_id) as $lease_id=>$hut_code){
	 if ($mode === 'classes'){
	   if (empty($v)) $v = (is_object($v_id) ? $v_id : new bForm_vm_Visit($v_id));
	   $l = new bForm_vm_Lease($lease_id,$v);
	   $l->hookVisit($v);
	   $reply[] = $l;
	 }else{
	   if (!empty($hut_code)) $reply[] = $hut_code;
	 }
       }
     }
     return $reply; 
   }


   /*
    * Define blocks visibility
    */
   protected function formBlock_isVisible($block_ID,$fields=array()){

     if (!bAuth::authenticated()) return False;

     $block_ID   = str_replace($this->ID,'',$block_ID);
     $block_only = @$_GET['block_once'];
     if (($block_only == VM_visit_expenses) && ($this->MODE == 'RO')) $block_only = $_GET['block_once'] = '';
     if (strpos($block_only,VM_visit_lease) !== False) $block_only = VM_visit_lease;
     if (strpos($block_ID,  VM_visit_lease) !== False) $block_ID   = VM_visit_lease;

     if (@$_GET['action_once'] === VM_visit_project) $this->isVITAL = array();

     switch($block_ID){
     case 'GSF':
     case VM_visit_expenses:
       $reply = VM_administrator_here || VM_organizer_here || $this->isHost();
       break;

     case VM_visit_lease:
       if (VM::outOfScope($this->e) || $this->visitOutOfScope()){
	 $reply = False;
       }else{
	 $reply = (int)$this->ID; //  && $this->isApproved();
	 if (empty($this->clashes) && (int)$this->ID && $this->isPending()) myPear::WARNING("The visit is NOT yet approved");
       }
       break;

     case VM_visit_project:
       if (@$_GET['action_once'] === VM_visit_project){
	 $this->MODE = 'RW';
	 $reply = True;
       }elseif($this->isDenied() || 
	       $this->visitOutOfScope() || 
	       (VM_access()->getRank()<=RANK_vm_registrant) ||
	       ($this->getValue('v_end',True) < time()) || 
	       ($this->hookEvent() && !VM_projects()->getProjectID($this->e->ID))){
	 $reply = False;
       }elseif(!$this->getValue('v_projectid',True)){
	 // Force the project selection
	 $this->MODE = 'RW';
	 $reply = True;
       }else{
	 $reply = parent::formBlock_isVisible($block_ID,$fields);
       }
       break;

     case VM_visit_policy:
       if (VM::outOfScope($this->e) || $this->visitOutOfScope() || !VM_administrator_here){
	 $reply = False;
       }else{
	 $this->hookEvent();
	 $reply = (VM::hasRightTo('edit_visit',$this) ? True : 'RO');
	 if ($block_only==VM_visit_expenses)                  $reply = False;
	 if (@$_POST['_post_bForm_ID_once']==='new')          $reply = False;	
	 if (@$_POST['_post_bForm_ID_once']==='empty')        $reply = False;	
	 if ($reply && !VM::hasRightTo('edit_visit',$this))   $reply = 'RO';     
       }
       break;

     case VM_visit_whatElse:
       if ($this->isDenied() || $this->visitOutOfScope()){
	 $reply = False;
	 break;
       }

     case VM_visit_info:      // always show VM_visit_info
       if ($block_ID == VM_visit_info){
	 if (!empty($block_only) && ($block_only != $block_ID)){
	   $reply = 'RO';
	   if ($block_only==VM_visit_expenses) $reply = False;
	   break;
	 }
       }

     case VM_visit_host:
       if (($block_ID==VM_visit_host) && $this->isDenied())     $reply = False;
       elseif($this->MODE == 'RO') $reply = True;
       elseif(!empty($block_only)) $reply = ($block_ID == $block_only); 
       else $reply = parent::formBlock_isVisible($block_ID,$fields);
       if ($reply && $this->visitOutOfScope()) $reply = 'RO';
       break;

     default:
       $reply = parent::formBlock_isVisible($block_ID,$fields);
       if (!empty($block_ID)) myPear::WARNING_T("Unexpected block '$block_ID'");
     }

     $this->dbg($reply,(bool)$this->debug || cnf_show_st);
     return $reply;
   }
   
   function hookEvent(){
     if (!($this->e instanceof bForm_vm_Event) && ($v_eid = $this->getValue('v_eid', True))) {
       $this->e = myPear::getInstance('bForm_vm_Event',$v_eid,array('strict','fatal'));
     }
     $this->dbg((is_object($this->e)
		 ? $this->e->__toString()
		 : False));
     return is_object($this->e);
   }
   
   /*
    *
    */
   function hookExp(){
     if (!is_object($this->exp)){
       locateAndInclude('bForm_vm_Expenses');
       $this->hookEvent();
       $this->getDefaultProject();
       
       if ($this->myPost_bForm())     $this->adjust_dates();
       
       //
       // locate the Expenses record
       if ((int)$this->ID){
	 if (!($this->exp instanceof bForm_vm_Expenses)) {
	   $q = myPear_db()->query("SELECT exp_id FROM abs_expenses WHERE exp_vid = '$this->ID'");
	   while ($r = myPear_db()->next_record($q)) $id = $r['exp_id'];
	   $this->exp = new bForm_vm_Expenses(is_numeric(@$id) ? $id : 'new');
	   $this->exp->isEmbedded($this);
	   $this->exp->defineVariables();
	   $this->exp->hookVisit();
	   $this->exp->createEstimates();
	 }
	 $this->mergeDefinitions($this->exp);
       }
       $this->isVisitType_program();
       $this->dbg((is_object($this->exp)
		   ? $this->exp->__toString()
		   : False));
     }
     return $this->exp;
   }

   /*
    *
    */
   function hookDependancies(){
     
     $this->hookAvatar();
     
     $this->hookExp();

     $reply = (is_object($this->av) && is_object($this->exp));
     $this->dbg($reply);
     
     // Ask for the residenship country if the travel is expected to be reimburced
     if ($reply && ($this->need_av_residentship === Null)){
       $this->need_av_residentship = (bool)($this->getPolicy(VM_V_payTrip) &&
					    !$this->av->getValue('av_residentship',True) &&
					    !$this->exp->getValue('exp_travel_est',True));
       $this->dbg("need_av_residentship? ".var_export($this->need_av_residentship,True));
       if ($this->need_av_residentship){
	 if ($this->av->isWritable()) myPear::ERROR("Missing residentship for ".$this->av->fmtName('fL').", can't estimate the travel cost.");
	 $this->av->need_av_residentship();
	 $this->mergeDefinitions($this->av);
       }
     }
     return $reply;
   }
   
   /*
    *
    */
   function reimbursementRates(){
     $this->hookEvent();
     return (is_object($this->e)
	     ? $this->e->reimbursementRates()
	     : bList::getListInstance(myOrg_ID,'bList_vm_reimbursementRates'));
   }
   
   /*
    * Returns (bool)False if there are clashing leases.
    * Normally that should not happen, so it is useful to chase bugs. 
    */
   private $adjust_dates = Null;
   function adjust_dates(){
    if (($this->adjust_dates === Null) && (int)$this->ID && ($leases = $this->get_allLeases())){
      $this->adjust_dates = True;

      // Limit the leases by the visit start/end
      $l = count($leases) - 1;
      $leases[0]->setDate(array('lease_start' => $this->getValue('v_start')));
      $leases[$l]->setDate(array('lease_end'  => $this->getValue('v_end')));
      
      // Check the leases for clashes
      for($n=0; $n<$l; $n++){
	if (b_time::inInterval($s1=$leases[$n]->getValue('lease_start'),
			       $e1=$leases[$n]->getValue('lease_end'),
			       $s2=$leases[$n+1]->getValue('lease_start'),
			       $e2=$leases[$n+1]->getValue('lease_end'))){
	$this->adjust_dates = False;
	  myPear::WARNING(sprintf("Clashing leases <br/>".
				  "lease %d: %s<br/>" . 
				  "lease %d: %s",
				  $leases[$n]->ID,
				  b_time::period($s1,$e1,'full'),
				  $leases[$n+1]->ID,
				  b_time::period($s2,$e2,'full')));
	}
      }
    }
    return $this->adjust_dates;
  }
  
  /*
   *
   */
  function defineVariables() {

    // Note, there is a second call to 'defineVariable' below, hence no check on number of calls,
    // i.e.   if ($this->defineVariable_counter) return;

    $this->defineVariable(array('e_name'              => ' ',
				'v_host_avid'         => 'Host',
				'virt_host'           => 'Host',
				'v_accompaning'       => 'Accompanying persons',
				'v_chance'            => 'Probability of visit',
				'v_comment'           => 'Details (purpose of visit, lecture title, etc.)',
				'v_created'           => 'Request date',
				'v_group'             => 'Research group',
				'v_oid'               => 'Office number',
				'v_projectid'         => 'Project to pay from',
				'v_start'             => 'Start of visit',
				'v_end'               => 'End of visit',
				'v_status'            => 'Is visit approved?',
				'v_type'              => 'Type of the visit',
				'v_eid'               => 'v_eid',       
				'v_avid'              => 'v_avid',
				'v_code'              => 'v_code',
				'v_owner_avid'        => 'v_owner_avid',
				'v_admin_avid'        => 'v_admin_avid',
				'v_policy'            => 'v_policy',  
				'v_gsf'               => ' ',
				'_virt_owner'         => 'Request owner',
				'_virt_updated_by'    => 'Updated by',
				'lease_aid'           => 'Accommodation',
				),True);

    $this->defineTypes(array(//'submitOnChange'=>array('v_status'),
			     'isMBE'         =>array('v_projectid', 
						     'v_accompaning',
						     'v_comment',
						     'v_chance',
						     'v_oid',
						     'v_code',
						     'v_policy',
						     '_virt_owner'),
			     'isRO'          =>array('v_status' => !VM::hasRightTo('approve_guest_invitation',$this),
						     'v_eid',						
						     'v_gsf',					
						     'v_owner_avid',
						     'v_created',
						     'v_admin_avid',						
						     'e_name',
						     '_virt_updated_by',
						     '_virt_owner'),
			     'isPRIVATE'     =>array('v_owner_avid',
						     'v_accompaning',
						     'v_type',
						     'v_comment',
						     '_virt_updated_by',
						     '_virt_owner'),
			     'isVITAL'       =>array('v_start',
						     'v_end',
						     'v_type'),
			     //'keepRW'        =>array('v_type'),
			     ));
    
    /*
     * Fine tuning of the variable types
     */
    $this->isMBE['v_host_avid'] = ($this->getValue('v_type',1) != VISIT_TYPE_COLLABORATION);
    if ($this->isVisitType_program()){
      foreach(array('v_start','v_end','v_type','v_comment') as $f){
	$this->isRO[$f] = $this->isMBE[$f] = True; 
      }
    }
    
    foreach (array('v_accompaning',
		   'v_oid') as $item){
      $this->isRO[$item] = !VM::hasRightTo('edit_visit',$this);
    }
    
    $this->leaveCommentUnformatted = True;
  }

  /**
   *
   */
  function readDefRecord(){
    $q = myPear_db()->query("SELECT * FROM $this->TABLE ".
			    " LEFT JOIN abs_events   ON  e_id = v_eid ".
			    " LEFT JOIN abs_expenses ON  v_id = exp_vid ".
			    " LEFT JOIN zzz_avatars  ON av_id = v_avid WHERE $this->TABLEID = '$this->ID'");
    while ($r=myPear_db()->next_record($q)) $this->formDB = $r;
    if (empty($this->formDB)) $this->dbg("??? empty formDB for ".$this->TABLEID."=".$this->ID,$this->debug||cnf_dev);
  }
  
  function readCompleteDefRecord(){
    $q = myPear_db()->query("SELECT * FROM $this->TABLE ".
			    " LEFT JOIN zzz_list_members ON v_projectid=lm_status ".
			    " LEFT JOIN abs_expenses     ON exp_vid=v_id ".
			    " LEFT JOIN abs_leases       ON lease_vid=v_id ".
			    " LEFT JOIN abs_apartments   ON lease_aid=a_id ".
			    " LEFT JOIN abs_huts         ON a_hutid=hut_id ".
			    " LEFT JOIN zzz_avatars      ON v_avid=av_id ".
			    " WHERE $this->TABLEID = '$this->ID' ".
			    " GROUP BY lease_id ");
    $reply = array();
    while ($r=myPear_db()->next_record($q)) $reply[] = $r;
    return $reply;
  }
  
  /*
   *
   */
  function cotenants(){
    locateAndInclude('bForm_vm_Lease');
    $reply = array();
    foreach ($this->get_allLeases() as $l){
      foreach(VM_cotenants()->get_cotenants($l->ID) as $lm_id=>$ct){
	foreach($ct as $lease_id){
	  $reply[] = bForm_vm_Lease::_leaseInfo($lease_id,True);
	}
      }
    }
    sort($reply);
    return array_unique($reply);
  }

  /*
   * Helper, get the apartment codes for all the leases
   */
  public function hut_codes($without_BI=False){
    $reply = self::_hut_codes(array('v_id' =>$this->ID,
				    'v_eid'=>$this->getValue('v_eid',1)),$without_BI);
    $this->dbg(join(', ',$reply));
    return $reply;
  }

  /*
   *
   */
  public static function _hut_codes($buffer,$without_BI=False){
    $reply = array_values(self::_v_get_allLeases($buffer['v_id']));
    if ($without_BI){
      if (empty($buffer['v_eid'])){
	$reply = array();
      }else{
	$e = myPear::getInstance('bForm_vm_Event',$buffer['v_eid'],array('fatal'));
	$reply = array_diff($reply,array(HUT_BI),array_values($e->accommodationOptions()->get_BI(True)));
      }
    }
    if ($reply === array(Null)) $reply = array();
    sort($reply);
    return $reply;
  }
  
  
  function host(){
    if ($this->av_host) return  $this->av_host->ID .' - '. $this->av_host->name();
    else                return '';
  }

  function name(){
    $this->hookAvatar();
    if (is_object($this->av)){
      return trim($this->av->getValue('av_firstname',True,True).' '.$this->av->getValue('av_lastname',True,True));
    }else{
      return 'Visit';
    }
  }

  function perdiem($showFormula=False,$showDefaultOnly=False){
    if ($this->hookDependancies()){
      return $this->exp->scholarshipP($showFormula,$showDefaultOnly);
    }
  }

  function living($showFormula=False,$showDefaultOnly=False){
    if ($this->hookDependancies()){
      return $this->exp->living($showFormula,$showDefaultOnly);
    }
  }

  /*
   *
   */
  public function periodBlock($mode='RW'){
    
    $this->formDB['Visit_dates'] = 'yes';
    $this->defineVariable((($this->MODE == 'RW') ? 'Visit dates' : ' '),'Visit_dates');
    
    if ($this->e instanceof bForm_vm_Event){ 
      // Visit is within an event
      $e=$this->formDB['e_name']=$this->e->nameSimple().' <br/> '.b_time::period($this->e->formDB['e_start'],$this->e->formDB['e_end']); 
      if ($v_code=$this->getValue('v_code',1,1)){
	if (cnf_inside_fb){
	  // $this->formDB['e_name'] as is...
	}elseif (VM_registrant_here){
	  $this->formDB['e_name'] = bJS()->modal_iframe(VM_agenda()->getRegistrantURL((int)($v_code/100000)),$e);
	}else{
	  $this->formDB['e_name'] = bJS()->modal_iframe(VM_agenda()->getRegistrantURL((int)($v_code/100000),$v_code%100000),$e);
	}
      }
      $dialog['e_name'] = array('textField'); 
    }else{
      // Visit is within a collaboration
      $dialog['Visit_dates'] = array('datePickerField2','v_start','v_end','i-a-arrive','i-a-depart');
    }

    // Accompanying persons
    if ($_virt_accompaning = $this->getGSF($trigger='accompan','')){
      $n = trim(str_replace(array('(',')'),' ',self::$_getGSF[$trigger]));
      if (is_int($n))                           $v_accompaning = $n;
      elseif(preg_match('/\b(three|3)\b/i',$n)) $v_accompaning = 3;
      elseif(preg_match('/\b(two|2)\b/i',  $n)) $v_accompaning = 2;
      elseif(preg_match('/\b(one|1)\b/i',  $n)) $v_accompaning = 1;
      if (isset($v_accompaning)){
	$this->getValue_reset('v_accompaning',$v_accompaning);
	$this->isRO['v_accompaning'] = True;
      }
    }
    $dialog['v_accompaning'] = array('selectMenu',array(0=>'coming alone',
							1=>'me + 1',
							2=>'me + 2',
							3=>'me + 3',
							// 4=>'me + 4',
							));
    // Show the v_accompaning wish
    if (!empty($_virt_accompaning)){
      $this->defineVariable(array('_virt_accompaning'=>' '),True);
      $this->formDB['_virt_accompaning']= $_virt_accompaning;
      $this->isRO['_virt_accompaning']  = True;
      $dialog['_virt_accompaning'] = array('textField');
    }
    
    // Show the block
    $this->formBlock(VM_visit_info,
		     'Visit information',
		     $dialog,
		     array('i'=>'i-portfolio_edit',
			   'l'=>$this->sameURL_but(array('mode'=>$mode))));
  }
  
  /*
   * Hook the Avatar to the Visit class
   */
  function hookAvatar(){
    if (!($this->av instanceof bForm_Avatar)){
      $this->defineVariable(array('v_avid'  =>'v_avid',
				  'v_type'  =>'v_type',
				  'av_email'=>'av_email',
				  ),'only_if_not_defined');
      foreach(array(array('av_id'   =>$this->getValue('v_avid',  1,1)),
		    array('av_email'=>$this->getValue('av_email',1,1))) as $arg)
	if ($this->av = bForm_Avatar::hook($arg)) break;
      if(is_object($this->av)){
	bForm_Avatar::set_context($this->av,VM_MODULE);
	$this->av->isLocked($this->getValue('v_type',1)==VISIT_TYPE_PROGRAM);
      }
    }
    $reply = ($this->av instanceof bForm_Avatar);
    if ($reply){
      $this->mergeDefinitions($this->av);
      $this->dbg($this->av->__toString());
    }
    return $reply; 
  }
  
  /*
   *
   */
  public function getValue($name, $asIs=False, $d_MBE=True){
    $value_p = parent::getValue($name, $asIs, True);
    switch($name){
    case 'v_avid':
      if (empty($value_p)) $value = b_cnf::get($name);
      else                 $value = $value_p;
      break;

    default:
      return $value_p;
    }
    return $this->getValue_return('',$this->getValue_toCache($name,$value));
  }

  /*
   * Helper function for the getValue
   */
  protected function getValue_validate_value($name,&$value=Null){
    $err_msg = '';
    if (!myPear::is_empty($value)){
      $err_msg = parent::getValue_validate_value($name,$value);
    }
    return $err_msg;
  }

  /*
   * Helper function for the getValue
   */
  protected function getValue_validate_empty($name,&$value=Null){
    $err_msg = '';
    if (myPear::is_empty($value)){
      $err_msg = parent::getValue_validate_empty($name,$value);
      switch($name){
      case 'v_host_avid':
	if (empty($this->formDB['v_group'])) $err_msg = '';
      case 'v_group':
	if (!in_array(@$this->formDB['v_type'],array(VISIT_TYPE_OTHER,
						     VISIT_TYPE_COLLABORATION)))  $err_msg = '';
	break;
	
      case 'av_email': // workaround... Should find where it comes from
      case 'v_comment':
	$err_msg = '';
	break;
      default:
      }
      if (self::_visitOutOfScope($this->formDB)) $err_msg = '';
    }
    return $err_msg;
  }

  /*
   * Dialog to see/set the policy
   */
  private $endorses = Null;
  private function get_endorses(&$dialog){

    if ($this->endorses === Null){
      $this->endorses = array();
      if (($e_id=$this->getValue('v_eid',1)) > 0){
	// Visit for a regular event, to be approved by the event organizers
	$this->endorses[] = 'Event organizers:';
	foreach(VM_organizers($e_id)->getMembers() as $um_id=>$rec){
	  $this->endorses[$rec['um_avid']] = '&nbsp;'.bForm_Avatar::_fmtName('Lf',$rec);
	}
      }else{
	// another type of a visit
	foreach(VM_endorsers()->getEndorsers($this->getValue('v_group',1)) as $av_id){
	  $av = new bForm_Avatar_vm($av_id);
	  $this->endorses[$av->ID] = '&nbsp;'.bForm_Avatar::_fmtName('Lf',$av->formDB);
	}
      }
    }
    
    // Visit is not approved, provide the dialog
    if ($this->isPending() && !empty($this->endorses)){
      // $this->tips['endorses'] = "Person who can approve or deny the visit";
      $this->defineVariable('To be approved by','endorses');
      $this->isVITAL['endorses'] = $this->isRO['endorses'] = True;
      $_POST['endorses'] = $this->formDB['endorses'] = join('-BR-',array_values($this->endorses));
      $dialog['endorses'] = array('textField');
    }
  }

  private function GSFblock(){
    if ($v_gsf = $this->getValue('v_gsf',True)){    
      $dialog = array();
      $v_gsf = unSerialize($v_gsf);
      $accommodation = ($c = $this->getValue('v_comment',True)) ? array('Accommodation' => $c) : array();
      foreach(array_merge($accommodation,$v_gsf) as $q=>$a){
	$this->defineVariable($q,$f='_virt_'.b_crypt::hash($q));
	$this->isRO[$f] = True;
	$this->formDB[$f] = $a;
	$dialog[$f] = array('textField');
      }
      $this->formBlock('GSF',
		       'Applicant\'s wishes & comments',
		       $dialog);
    }
  }

  /*
   *
   */
  function policyBlock(){

    if (!$this->formBlock_isVisible(VM_visit_policy)) return;

    $this->getStatus();
    $visit_status = VM::$visit_status;
    $visit_status[STATUS_PENDING] = x('strong',b_fmt::redText($visit_status[STATUS_PENDING]));
    if ($this->isRO('v_status')){ 
      $visit_status[STATUS_YES] = bIcons()->get('i-finger_up');
      $visit_status[STATUS_NO]  = bIcons()->get('i-finger_down');
      $dialog = array('v_status'=> array('textField',array(),$visit_status[$this->getValue('v_status')]));
    }else{
      $this->isRO['v_status'] = False;
      $dialog = array('v_status'=>array('selectMenu',$visit_status)); // ,'onchange="submit()"'));
    }

    // Add endorses if the visit is not approved
    $this->get_endorses($dialog);
    
    // If the visit was approved, check the status/policy 
    if ($this->isApproved() || VM::hasRightTo('edit_visit',$this)){
      foreach ($this->getPolicies() as $p=>$descr){
	if (in_array($p,VM::$auto_maintained)) continue;
	$k = '_virt_policy_' . $p;
	if ($this->exp_arePayed()) $this->isRO[$k] = True;
	$this->defineVariable($descr['d'],$k);
	$this->formDB[$k] = ($this->getPolicy($p) ? 'yes' : 'no');
	$dialog[$k] = array('checkBox',0,
			    'i-finger_up-small', 
			    'i-finger_down-small');
      }
	  /*
	if ($this->MODE == 'RW' || ($value==='yes' || $this->isWritable())){
	  if ($this->exp_arePayed()) $this->isRO[$k] = True;
	  $dialog[$k] = array('checkBox');
	}
	  */
    }
    
    if (!VM::hasRightTo('edit_visit',$this)){	
      foreach(array_keys($dialog) as $k) $this->isRO[$k] = True;
    }
    
    // Print the policy dialog
    $this->formBlock(VM_visit_policy,'Visit '.($this->isApproved() ? 'policy' : 'status'),
		     $dialog,
		     array('d'=>'',
			   'i'=>($this->exp_arePayed()?'i-lock':'i-user1_edit'),
			   'l'=>$this->sameURL_but(array('mode' =>'RW'))));
  }
  
  /*
   * Show accommodation ('lease' objects)
   */
  private function accommodationBlock(){
    foreach($this->get_allLeases() as $l) $l->getForm();
  }
  
  /*
   * Project to pay from
   */
  function projectBlock(){
    
    if ($this->isApproved()){
      $where = array("lm_option != 'no'");
      if ($this->e) $where[] = "(lm_status = ".$this->e->ID." OR lm_status = 'RO')";
      else          $where[] = "(lm_status = '' OR lm_status = 0 OR lm_status = 'RO')";
      
      $this->formBlock(VM_visit_project,
		       'Budget source',
		       array('v_projectid'=>array('selectMenu_bList',VM_projects(),$where,'project')),
		       array('d'=>btn_edit_project,
			     'i'=>'i-wallet',
			     'l'=>$this->sameURL_but(array('block'=>'project','mode'=>'RW'))));
    }
  }

  /*
   * Office
   */  
  private function officeBlock(){
    if ($this->getPolicy(VM_V_provideOffice)){
      if (($s = @$this->getValue('v_start',1)) && ($e = @$this->getValue('v_end',1))){
	locateAndInclude('bForm_vm_Office');
	if (!$this->getValue('v_oid',1)) $this->getValue_reset('v_oid',bForm_vm_Office::oo());
	$dialog_host['v_oid'] = array('selectMenu',$this->availableOffices($s,$e));
	$this->isVITAL_ifADMIN['v_oid'] = True;
      }
    }
  }

  /*
   * Expenses
   */
  private function expensesBlock(){
    if ($this->hookDependancies() && $this->isApproved()){
      $this->mergeDefinitions($this->exp);
      $this->exp->getForm();
      if(!$this->getValue('v_projectid',True) && ($this->getValue('v_end',True) > time()-AUTO_EXPIRATION*86400)){
	if($this->isWritable()){
	  if(is_object($this->e) && !VM_projects()->getProjectID($this->e->ID)){
	    myPear::WARNING("<br/><em>\"".$this->e->name(False)."\"</em> has NO budget code assigned. ".
			    "<br/>Payment is not possible.");
	  }
	  if (VM::hasRightTo('select_budget',$this) && 
	      (!$this->hookEvent() || VM_projects()->getProjectID($this->e->ID))){
	    myPear::WARNING("Please select the budget source for the visit (<em>\"".$this->vars['v_projectid']."\"</em> below)");
	    // Force the project selection
	    $this->MODE = 'RW';
	  }
	}
      }
    }
  }
  
  /*
   *
   */
  private function whatElseBlock(){
    $this->formBlock(VM_visit_whatElse,
		     'What else can we get from the visit',
		     array('v_comment' =>'textArea'));
    
  }

  /*
   * Visit hosting
   */
  private function hostingBlock($only_groups=False){ 
    if (!is_object(bAuth::$av)) return;
    
    // Prepare a list of valid hosts - the group members, the owner, the enforcers
    foreach(array(array(bAuth::$av->ID => bAuth::$av->fmtName()),
		  myOrg()->orgGroups()->get_groupMembers($this->getValue('v_group',1)), 
		  VM_endorsers()->getEndorsers(Null,'full'),
		  ) as $candidates){
      foreach($candidates as $av_id=>$av_name) $valid_hosts[$av_id] = $av_name;
    }
    if (($av_id = $this->getValue('v_host_avid',1)) && empty($valid_hosts[$av_id])){
      if ((int)$av_id){
	$av = myPear::getInstance('bForm_Avatar',$av_id,array('nocache','strict'));
	if (is_object($av)) $valid_hosts[$av->ID] = $av->fmtName();
      }else{
	$this->getValue_reset('v_host_avid',0);
      }
    }
    asort($valid_hosts);

    // Dialog
    $this->isVITAL['v_group'] = True;
    $this->submitOnChange['v_group'] = !myPear::is_empty($this->getValue('v_group',1));
    $dialog = array(
		    'v_group'    => array('selectMenu', myOrg()->orgGroups()->get_groups()),
		    'v_comment'  => array('textArea'),
		    'v_host_avid'=> array('selectMenu',$valid_hosts),
		    '_virt_owner'=> array('textField'),
		    );
    if ($this->MODE == 'RO'){
      if ($av_id = (int)$this->getValue('v_host_avid',1)) $this->formDB['virt_host'] = $valid_hosts[$av_id];
      else                                                $this->formDB['virt_host'] = 0;
      $dialog['v_host_avid'] = array('textHidden',$this->getValue('v_host_avid'));
      $dialog['virt_host']   = array('textField');
    }

    $this->formBlock(VM_visit_host,
		     'Visit hosting',
		     $dialog,
		     array('d'=>'auto',
			   'i'=>'i-usrshare',
			   'l'=>$this->sameURL_but(array('mode' =>'RW'))));
  }

  /*
   *
   */
  protected function displayErrors_preflight(){
    // Check what the parent thinks 
    $errors = parent::displayErrors_preflight();
    return $errors;
  }

  /*
   * Small helper
   */
  function virt_updated_by(){
    $av = myPear::getInstance('bForm_Avatar_vm',$this->getValue('v_admin_avid',1,1),array('strict'));
    if (!empty($av)){
      $this->formDB['_virt_updated_by'] = $av->fmtName('fl',0).', '.b_time::medium_date($this->getValue('v_timestamp'));
      return $this->formDB['_virt_updated_by'];
    }
    return False;
  }

  /*
   * Visit dialog
   */
  function getForm(){

    // Sanity, catch invocation with an arbitrary v_id
    if (!$this->hookAvatar()){
      $this->dbg("Can't hook Avatar for the visit",$this->debug||cnf_dev);
      return;
    }

    // Hook dependencies
    $this->getFormOpen();
    $this->hookDependancies();
    $this->av->defineVariables();

    // Start the dialog

    if (!$this->isApproved()){
      $this->tips['v_status'] = "The visit is not processed unless it is approved";
    }
    
    if (!$this->isEmbedded()) {
      myPear::H1($this->av->fmtName('sfL'),array('noTranslate','reset'));
      bForm::nameCounter();
    }


    // Do not allow any changes for the closed visits (closing is done by the finance) 
    $this->declareRO($this->exp_arePayed());
    

    /*
     * Open the form and put  the calendar block into "pending objects" for the table class.
     * The calendar will then be printed on the first table row
     */
    if ($this->MODE == 'RO')   $this->showCalendar('v_start','v_end',$seeDepartureDate=False);

    // Show the contact info first
    if ($this->av->isOwner()){
      if (is_object($this->exp) && $this->exp->getBlanketts('button')) $this->exp->getBlanketts('message');
    }else{
      $this->av->avInfoBlock();
    }
    if ($this->need_av_residentship) $this->av->residentshipBlock();

    /*
     * This dialog starts after the visit type is selected,
     * otherwise prompt for the visit type
     */
    if ($v_type = $this->getValue('v_type',True)){
      $this->periodBlock();
      $this->policyBlock();
      $this->accommodationBlock();
      switch($v_type){
      case VISIT_TYPE_OTHER:
      case VISIT_TYPE_POSSIBLE:
      case VISIT_TYPE_COLLABORATION:
	$this->officeBlock();
	$this->hostingBlock();
      case VISIT_TYPE_PROGRAM:
	$this->expensesBlock();
	$this->projectBlock();
      case VISIT_TYPE_RENT:
	break;
	
	if ($date = $this->getValue('v_created',1)){
	  if ($av = myPear::getInstance('bForm_Avatar_vm',$this->getValue('v_owner_avid',1))){
	    $this->formDB['_virt_owner'] = $av->fmtName('fl').' '.b_time::medium_date($date);
	  }
	}
      }
      
      if ($this->virt_updated_by()) $this->textField('_virt_updated_by');

    }elseif($this->MODE == 'RW'){
      /*
       * Request first the visit type,
       * since all the rest depends on it
       */
      unset($this->submitOnChange['v_type']);
      $this->isMBE['v_group'] = False;
      $this->formBlock(VM_visit_info,
		       'Visit information',
		       array('v_type' => array('selectMenu',$this->_visit_types($v_type)), //   'onchange="submit()"'), 
			     'v_group'=> array('selectMenu',myOrg()->orgGroups()->get_groups()),
			     'v_avid' => array('textHidden',$this->getValue('v_avid')),
			     'Visit_dates'=>array('datePickerField2','v_start','v_end','i-a-arrive','i-a-depart')));
    }
    
    // Show optional free format fields from the application form (need for financial support, etc);
    $this->GSFblock();

	
    // Close the dialog
    $btn = array();
    if (($this->getValue('v_type',1) != VISIT_TYPE_PROGRAM) && ($this->getStatus()==STATUS_NO)) 
      $btn[txt_deleteVisit] = b_url::same(array(b_crypt_no=>1,
						'function'=>'vm_cancel_visit',
						'v_id'    => $this->ID,
						'x_once'  => 'confirm_yes'),
					  array('id'));
    $this->closeForm('update',$btn);
    
    //
    // Easy button for endorsers
    //
    if (empty($_SESSION[get_class($this).$this->ID]) &&
	$this->isPending() &&
	(($v_group=$this->getValue('v_group',1)) && in_array(bAuth::$av->ID,VM_endorsers()->getEndorsers($v_group)) ||
	 (is_object($this->e) && $this->e->isOrganizer()))
	){
      print b_btn::UL_buttons(array(b_btn::big_button('Deny visit',      array('form'  => get_class($this),
									       'id'    =>$this->ID,
									       'a_once'=>STATUS_NO,
									       'resetcache_once'=>1),
						      'red',True),
				    b_btn::big_button('I need to think...',array('form'  => get_class($this),
									       'id'    =>$this->ID,
									       'a_once'=>STATUS_PENDING,
									       'resetcache_once'=>1),
						      'grey',True),
				    b_btn::big_button('Approve visit',   array('form'  => get_class($this),
									       'id'    =>$this->ID,
									       'a_once'=>STATUS_YES,
									       'resetcache_once'=>1),
						      'green',True)));
    }
    bIcons()->explain();
  }
  
  private function getDefaultProject(){
    if ((int)$this->ID && !$this->getValue('v_projectid',1)){
      if ($this->hookEvent()){
	if ($projectID = VM_projects()->getProjectID($this->e->ID)) $this->updateDB(array('v_projectid'=>$projectID),$forced=True);
      }
    }
  }

  protected function _recordCreated() {
    myPear_logs()->add('visit',$this->ID);
  }
                       
  /*
   *
   */
  function availableOffices($start,$end){
    $list = array();
    $considerOfficeOccupancy = False; // seem not to be necessary
    if ($considerOfficeOccupancy){
      $q = myPear_db()->query("SELECT * FROM abs_offices WHERE o_id>0 AND o_status='".OFFICE_AUTO_ALLOCATABLE."' ORDER BY o_name");
      while($r=myPear_db()->next_record($q)){
	$qq = myPear_db()->query("SELECT * FROM abs_visits WHERE ".
				    join(' AND ', array("v_oid = ".$r['o_id'],
							"v_start <= $end",
							"v_end   >= $start")));
	if ((myPear_db()->num_rows($qq) < $r['o_capacity']) ||
	    ($r['o_id'] == $this->formDB['v_oid']))$list[$r['o_id']] = $r['o_name'];
      }
    }else{
      $q = myPear_db()->query("SELECT * FROM abs_offices WHERE o_id>0 ORDER BY o_name");
      while($r=myPear_db()->next_record($q)){
	$list[$r['o_id']] = $r['o_name'];
	if ($r['o_status'] != OFFICE_AUTO_ALLOCATABLE) $list[$r['o_id']] .= ' - staff';
      }
    }
    if (($o_id=$this->formDB['v_oid']) && empty($list[$o_id])){
      if ($o = myPear::getInstance('bForm_vm_Office',$o_id)) $list[$o_id] = $o->name();
    }
    return $list;
  }
  
  /*
   * The host is the person responsible for the visit, i.e. the program organizer OR the inviting person
   */
  function isHost($av_id=Null){
    $reply = Null;
    if ( empty($av_id)) $av_id = @bAuth::$av->ID;
    if (!empty($av_id)){
      $this->hookEvent();
      $reply = (($this->getValue('v_host_avid',1) == $av_id) ||
		(is_object($this->e) && $this->e->isOrganizer($av_id)));
    }
    return $reply;
  }

  /*
   * The visitor himself in NOT owner of the visit object
   */
  function isOwner() {
    $reply = False;
    if (bAuth::authenticated()){
      switch($this->getValue('v_type',1)){
      case VISIT_TYPE_PROGRAM:
	if ($this->hookEvent())	$reply = $this->e->isOrganizer();
	break;
      default:
	$reply = ((bAuth::$av->ID == $this->getValue('v_host_avid',1)) ||
		  (bAuth::$av->ID == $this->getValue('v_owner_avid',1)));
      }
    }
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   *
   */
  function visitOutOfScope(){
    return VM::outOfScope($this->e) || self::_visitOutOfScope(array('v_type'=>$this->getValue('v_type',True,True)));
  }

  static function _visitOutOfScope($buffer){
    return (@$buffer['v_type'] === VISIT_OUT_OF_SCOPE);
  }

  /*
   *
   */
  function isVisitType_program(){
    return self::_isVisitType_program(array('v_type'=>$this->getValue('v_type',1)));
  }
  static function _isVisitType_program($buffer=Null){
    return ($buffer['v_type'] === VISIT_TYPE_PROGRAM);
  }
  
  private function _visit_types($v_type){
    $this->hookDependancies();
    if (in_array($v_type,array(VISIT_OUT_OF_SCOPE))) {
      $types = array($v_type=>VM::$description[$v_type]['d']);
      $this->isRO['v_type'] = True;
    }elseif (in_array($v_type,array(VISIT_TYPE_PROGRAM,VISIT_TYPE_RENT))) {
      $types = array($v_type=>VM::$visitTypes[$v_type]);
      $this->isRO['v_type'] = True;
    }else{
      $types = VM::$visitTypes;
      unset($types[VISIT_TYPE_PROGRAM]);
    }
    return $types;
  }

  /*
   * Canceling the visit
   */
  function delete(){
    $this->dbg($this->av->name(),$this->debug||cnf_dev);
    myPear::MESSAGE_add(b_fmt::redText("Delete visit ".$this->_periodToString()));    
    $this->deleteBooking();
    myPear_db()->query("DELETE FROM $this->TABLE WHERE $this->TABLEID = '$this->ID'");
    unset($this->formDB);
    unset($this->ID);
  }

  /*
   * Delete the given lease, merge the dates with the remaining ones.
   * If no leases left, then create an empty one for the whole period of the visit
   */
  public $deletingBooking = False;
  function deleteBooking($lease_id=Null,$verbose=True){
    $this->deletingBooking = True;
    $this->dbg(' ');
    if (!empty($lease_id)){
      //
      // Delete the specified booking
      //
      if ($l = myPear::getInstance('bForm_vm_Lease',$lease_id,array('strict'))){
	$lease_start_day = b_time::_($l->getValue('lease_start'));
	$lease_end_day   = b_time::_($l->getValue('lease_end'));
	if ($verbose) myPear::MESSAGE_add('Delete booking '.$l->_periodToString());
	$l->delete();
	
	// Extend the neighboring booking period
	foreach($this->get_allLeases() as $l){
	  $ls = b_time::_($l->getValue('lease_start',1));
	  $le = b_time::_($l->getValue('lease_end',1));
	  if(($ls == $lease_start_day) && ($le == $lease_end_day)) continue;
	  if ($le == $lease_start_day) {
	    if ($verbose) $l->updateDB(array('lease_end'  =>b_time::check_out($lease_end_day)));
	    myPear::MESSAGE_add("Modify booking end ".$l->_periodToString());
	    break; 
	  }
	  if ($ls == $lease_end_day)   {
	    $l->updateDB(array('lease_start'=>b_time::check_in($lease_start_day)));
	    if ($verbose) myPear::MESSAGE_add("Modify booking start ".$l->_periodToString());
	    break;
	  }
	}
      }else{
	myPear::WARNING_T("Cant find the lease",$this);
      }
    }else{
      //
      // Delete all bookings
      //
      $leases = $this->get_allLeases();
      if (empty($leases)){
	$this->dbg('No leases to delete...');
      }else{
	foreach($leases as $l) {
	  $l->debug = $this->debug;
	  $msg = $l->_periodToString();
	  $this->dbg($l->__toString().' '.$msg);
	  if ($verbose && $msg) myPear::MESSAGE_add(b_fmt::redText("Delete booking $msg"));
	  $l->delete();
	}
      }
    }
    $this->get_allLeases(True);
    $this->deletingBooking = False;
  }

  function setResource($what,$id,$dates){
    switch($what){
    case 'Office':  
      $this->updateDB(array('v_oid' => $id)); 
      break;
      
    case 'Apartment':
      $this->setBooking(array('lease_aid'  =>$id,
			      'lease_start'=>$dates['v_start'],
			      'lease_end'  =>$dates['v_end']));
      break;
    }
  }
  
  /*
   * array('lease_aid'=>ID,'lease_start'=>START,'lease_end'=>END)
   */
  function setBooking($lease=array(),$verbose=True){
    if (empty($lease)) return;
    if (!is_array($lease)){ // hence $lease is the apartment ID
      $a = myPear::getInstance('bForm_vm_Apartment',$lease);
      $lease = array('lease_aid'  =>$a->ID,
		     'lease_start'=>$this->getValue('v_start'),
		     'lease_end'  =>$this->getValue('v_end'));
    }
    $lease['lease_vid'] = $this->ID;
    
    // Sanity
    if (cnf_dev){
      if (myPear::getInstanceFromID($lease['lease_aid'])!='bForm_vm_Apartment') b_debug::internalError("setBooking($lease) is not bForm_vm_Apartment",$this); // sanity
      if (!$lease['lease_start'] ||!$lease['lease_end'])                        b_debug::internalError("setBooking($lease) no start/end time") ;           
    }
    
    // Create the booking (lease) if it does not exist      
    if (!$this->bookingExists($lease)){
      locateAndInclude('bForm_vm_Lease');
      $ll = array('lease_vid'  =>$this->ID,
		  'lease_start'=>$lease['lease_start'],
		  'lease_end'  =>$lease['lease_end']);

      if (!bForm_vm_Lease::get_ids($ll)){ 
	$l = new bForm_vm_Lease('new');
	$l->updateDB($ll);
      }
      foreach(bForm_vm_Lease::get_ids($ll) as $l_id){
	$l = new bForm_vm_Lease($l_id);
	$l->hookApartment($lease['lease_aid'],$verbose);
      }
      
      // Sanity
      if (!$this->bookingExists($lease)){
	myPear::MESSAGE_add(b_fmt::redText("Can't set booking '".b_fmt::joinX(', ',$lease)."' for ".$this->_periodToString()));
      }
    }
  }
  
  /*
   * Helper
   */
  private function bookingExists($lease=array()){
    locateAndInclude('bForm_vm_Lease');
    if (!$lease) $lease = array();
    $lease['lease_vid'] = $this->ID;
    $reply = (count(bForm_vm_Lease::get_ids($lease)) > 0);
    $this->dbg($reply);
    return $reply;
  }

  function exp_arePayed($lock=Null){
    $this->hookDependancies();
    $reply = (bool)(is_object($this->exp) && $this->exp->exp_arePayed());
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * The "external events" (those which have positive visit_code) are
   * not considered as clashes
   */ 
  static  $savedWhere = Null;
  function getClashingVisits($start=Null,$end=Null,$searchByName=False) {
    if (!$start) $start = $this->getValue('v_start',True);
    if (!$end)   $end   = $this->getValue('v_end',  True);
    $reply = self::_getClashingVisits($this->ID, $this->getValue('v_code',1), $this->getValue('v_type',1), $this->getValue('v_avid',1),
				     $start,$end,$searchByName);
    $this->queryString = self::$savedWhere;
    return $reply;
  }

  /*
   *
   */
  public $clashes = array();
  function getClashes($buffer=array()){
    $this->clashes = self::_getClashes($this->formDB,$verbose=True);
  }
  
  static function _getClashes($buffer=array(),$verbose=False){
    $clashes = array();
    $where = array("v_id != $buffer[v_id]",
		   "v_start <= '$buffer[v_end]'",
		   "v_end   >= '$buffer[v_start]'",
		   myPear_db()->quote($buffer['v_eid'], 1,'v_eid'),
		   myPear_db()->quote($buffer['v_type'],1,'v_type'),
		   myPear_db()->quote($buffer['v_avid'],1,'v_avid'),
		   );
    $q = myPear_db()->query("SELECT * FROM abs_visits ".
			    "    LEFT JOIN abs_events   ON v_eid = e_id ".
			    "    LEFT JOIN abs_expenses ON exp_vid = v_id ".
			    "    LEFT JOIN zzz_avatars  ON v_avid = av_id ".
			    "    LEFT JOIN zzz_list_members ON v_projectid = lm_id ".
			    " WHERE ".join(' AND ',$where));
    while ($r=myPear_db()->next_record($q)){
      if ($verbose){ 
	if (empty($msg)){
	  $msg = array('');
	  $msg[] = 'Overlap in visit dates detected for '.bForm_Avatar::_fmtName('fL',$buffer).':';
	  $msg[] = 'Current visit: '.b_time::period($buffer['v_start'],$buffer['v_end']);
	  
	  static $dejaVu = array();
	  if (!@$dejaVu[$buffer['v_id']]++){
	    locateAndInclude('bIcal_matrix_visits');
	    $m = new bIcal_matrix_visits();
	    $m->showLegend = False;
	    $m->day1 = $buffer['v_start']-3*86400;
	    $m->day9 = $buffer['v_end']  +3*86400;
	    $m->imposedEntries[] = $buffer;
	  }
	}
	if (is_object($m)){
	  $dejaVu[$r['v_id']]++;
	  $m->imposedEntries[] = $r;
	  $m->day1 = min($buffer['v_start']-3*86400,$m->day1);
	  $m->day9 = max($buffer['v_end']  +3*86400,$m->day9);
	}
      }
      $msg[] = 'Clashing visit: '.b_time::period($r['v_start'],$r['v_end']);
      $clashes[] = $r['v_id'];
    }

    // Print an error message if the visit is clashing
    if (!empty($clashes) && $verbose){
      $msg[] = x('strong','Please resolve the ambiguity.');
      myPear::ERROR(join('<br/>',$msg));
      if ($verbose && is_object($m))  $m->show('<hr/><br/>');
    }
    return $clashes;
  }
  
  /*
   * Check that the visit does clash with other visits
   */ 
  static  $savedQuery = Null;
  public static function _getClashingVisits($v_id, $v_code, $v_type, $v_avid, $start,$end,$searchByName=False) {
    $clashes = array();
    if ($av = bForm_Avatar::hook($v_avid)){
      $where = array("v_id    != '$v_id'",
		     "v_type   = '$v_type'",
		     "v_start <= '$end'",
		     "v_end   >= '$start'");
      if ($v_code>0) $where[] = "(v_code <= 0 OR v_code IS NULL)";
      if ($searchByName){
	$where[] = ($avSelect='('.join(' OR ',array(myPear_db()->quote($av->getValue('av_email',True,True),1,'av_email'),
						    "v_avid='$av->ID'",
						    '('.    myPear_db()->quote($av->getValue('av_firstname',1,1),'R','av_firstname').
						    ' AND '.myPear_db()->quote($av->getValue('av_lastname', 1,1),'R','av_lastname').')')).')');
      }else{       
	$where[] = "v_avid='$av->ID'";
      }
      locateAndInclude('bHolder_vm_Visits');
      $holder = new bHolder_vm_Visits();
      self::$savedWhere = $holder->where = array(join(' AND ',$where));
      self::$savedQuery = $holder->query();
      while ($r=myPear_db()->next_record(self::$savedQuery)) $clashes[] = $r['v_id'];
      if (empty($clashes))           self::$savedQuery = Null;
      elseif (!empty($avSelect))     myPear::WARNING_T($avSelect);
    }
    return    $clashes;
  }
  
  protected function showFatalErrorsSpecific(){ 
    if (@$this->queryString){
      ob_start();
      locateAndInclude('bHolder_vm_Visits');
      $holder = new bHolder_vm_Visits();
      $holder->noSort= True;
      $holder->where = $this->queryString;
      $holder->listEntries();
      $output = ob_get_contents();
      ob_end_clean();
      print $output;
      return True;
    }else{
      return False;
    }
  }

  /*
   *
   */
  public static function projectName($v_projectid,$compact=False){
    $reply = '';
    if ($m = VM_projects()->getMember($v_projectid)){
      $name = $m['lm_key']; 
      $code = $m['lm_value'];
      if ($compact){
	$reply = preg_replace('/^2... /','',preg_replace('/^.*-/','',$name));
      }else{
	$reply = $name;
	if (preg_match("/^[0-9]*$/",$code)) $reply = "$code: $name";
	else                                $reply = "$name: $code";
	if (preg_match("/^[0-9]*$/",$name)) $reply = "project $name";
	if (!$name)                         $reply = 'unspecified project';
      }
    }
    return b_fmt::escape($reply);
  }

  /*
   *
   */
  function comment($what='Apartment'){
    switch ($what){
    case 'Apartment':
      foreach ($this->get_allLeases() as $l){
	if ($ap = myPear::getInstance('bForm_vm_Apartment',$l->getValue('lease_aid',1))) return $ap->name();
      }
      break;
      
    case 'Office': 
      if ($o=myPear::getInstance('bForm_vm_Office',$this->formDB['v_oid'],'strict')) return $o->name();
      break;
    }
  }  

  /*
   *
   */
  function nDays($onlyWithinTheEvent=False) { 
    $this->hookEvent();
    $start = $this->getValue('v_start',1);
    $end   = $this->getValue('v_end',1);
    if ($this->e && $onlyWithinTheEvent){
      $start = max($start,$this->e->getValue('e_start'));
      $end   = min($end  ,$this->e->getValue('e_end'));
    }
    return (int)((b_time::_($end) - b_time::_($start))/86400);
  } 
  
  /*
   * Considering the visit status (pending/rejected/approved) check that all the formalities are completed:
   * - the applicant must be informed if an application was either rejected or accepted,
   * - the receipt should be sent to the tenant renting apartment
   * - etc.
   *
   * @return False - everything is ok
   *         STATUS_YES / STATUS_NO - the acknowledgment was not done for this type of decision
   *
   */
  public function formalitiesNotCompleted(){
    $buffer = $this->formDB;
    if ($this->hookEvent()) $buffer = array_merge($buffer,$this->e->formDB);
    return self::_formalitiesNotCompleted($buffer);
  }

  public static function _formalitiesNotCompleted(&$buffer){
    if (self::_visitOutOfScope($buffer)) return False;

    $policies = self::_getPolicies($buffer);

    locateAndInclude('bForm_vm_Event');    
    $VM_E_endorsed = bForm_vm_Event::_getPolicy(VM_E_endorsed,$buffer);
    
    $reply = False;
    switch($status=self::_getStatus($buffer)){
    case STATUS_YES:
      if (send_finalWelcomeEmail_to_registrants && $VM_E_endorsed && !empty($policies[VM_V_welcomeMailSent])){
	if (!self::_getPolicy(VM_V_welcomeMailSent,$buffer)) $reply = STATUS_YES;
      }
      break;
      
    case STATUS_NO:
      if (send_denialEmail_to_registrants && $VM_E_endorsed && !empty($policies[VM_V_denialMailSent])){
	// The denial letter should be, but was not sent
	if (!self::_getPolicy(VM_V_denialMailSent,$buffer))  $reply = STATUS_NO;
      }
      break;

    case STATUS_PENDING:
      $reply = STATUS_PENDING;
      break;

    default:
      b_debug::internalError('???');
    }

    //    if (cnf_show_pv && !empty($reply))  myPear::DEBUG(__FUNCTION__."($buffer[v_id]) <em>".(empty($reply) ? '&lt;OK&gt;' : $reply)."</em>");
    return $reply;
  }
  
  /*
   * Check the errors in the form
   */
  function formOKspecific(){
    if (!$this->hookAvatar()) b_debug::internalError("Can't hook Avatar",$this);
    
    // There should be no owner for the external visits                                                                                                                     
    if ($this->getValue('v_type',1) === VISIT_TYPE_PROGRAM){ 
      foreach(array('v_host_avid','v_owner_avid') as $item) $this->getValue_reset($item,0);
    }

    // Check for errors
    if ($this->visitOutOfScope()) $this->isVITAL = array();

    if (($d1=$this->getValue('v_start',1)) >= ($d9=$this->getValue('v_end',1)) && $d1) {
      $this->errorsInTheForm['v_end'] = x("'",$this->getDescr('v_start'))." must be before ".x("'",$this->getDescr('v_end'));
    }elseif ($this->getClashingVisits($d1,$d9)){
      $this->errorsInTheForm['v_start'] = 'Dates for the visit'.' '.b_time::period($d1,$d9).' '.'clash with another visit:';
    }

    if (!VM_manager_here && !$this->isOwner()) $this->errorsInTheForm = array();
    //    foreach (array_keys($this->errorsInTheForm) as $f) $this->printErrorAsIs[$f] = True; 

    // set the defaults for a newly created visit
    if ((@$_POST['_post_bForm_ID_once'] === 'new') || (@$_POST['_post_bForm_ID_once'] === 'empty')){
      if (!$this->getValue('v_created',1))         $this->getValue_reset('v_created',time());
      $this->getValue_reset('v_admin_avid',bAuth::$av->ID); 
    }

    if ($this->errorless()){

      // Save the avatar for a the visit to be created
      if ($v_avid = @$GET['v_avid'])      $this->getValue_reset('v_avid',$v_avid);
      
      // Set the "hosting person" to the submitter if not explicitly imposed
      foreach(array('v_host_avid','v_owner_avid') as $id){
	if (!(int)$this->getValue($id,1)) $this->getValue_reset($id,bAuth::$av->ID);
      }
    }       
    
    // Update policy 
    foreach(array_keys(VM::$v_policies) as $p){
      if (isset($_POST["_virt_policy_${p}"])){
	$OnOff = $this->checkBoxIsOn("_virt_policy_$p");
	$this->dbg(VM::$v_policies[$p]['d']." = '$OnOff'",cnf_dev);
	$this->setPolicy($p,$OnOff);
      }       
    }       
  }
  
  /*
   *
   */
  function _periodToString($name=''){
    if (empty($name)){
      $name = ($this->hookAvatar() ? $this->av->fmtName('fl') : 'Unknown');
    }
    return $name.' ' . $this->period();
  }
  
  function period(){
    return b_time::period($this->getValue('v_start',1),$this->getValue('v_end',1));
  }
  
  function isReadable(){
    return True;
  }

  /*
   * The VM_manager can edit visits from the past
   */
  private $isWritable = Null;
  function isWritable(){
    if ($this->isWritable === Null){ 
      bTiming()->cpu(__FUNCTION__);
      $this->hookDependancies();
      $exp_owner = (is_object($this->exp)
		    ? ($this->exp->getValue('exp_owner_avid') == @bAuth::$av->ID)
		    : False);
      $v_end = $this->getValue('v_end',True);
      if (empty($v_end)) $v_end = time()+AUTO_EXPIRATION*86400;
      $this->isWritable = (bool)b_cnf::check_conditions(array(False => array(//'pending'    => !$this->isApproved(),
									     'too late'   => ($v_end + AUTO_EXPIRATION*86400 < time()) && !VM_manager_here),
							      True  => array('endorser'   => (is_object(bAuth::$av) && 
											      in_array(bAuth::$av->ID,VM_endorsers()->getEndorsers($this->getValue('v_group',1)))),
									     'isOrganizer'=> VM_organizer_here,
									     'booker'     => VM_booker_here,
									     'owner'      => $exp_owner),
							      'default' => parent::isWritable()),
							$this,
							cnf_dev);
      bTiming()->cpu();
    }
    return $this->isWritable;
  }
  
  function formBlock_isWritable($block_ID){
    $reply = parent::formBlock_isWritable($block_ID);
    if ((int)$this->ID){
      switch($block_ID){
      case VM_visit_project:
	if (VM_reimberser_here)      $reply = True;
	break;
      case VM_visit_lease:
	if (VM_guests_handlare_here) $reply = True;
	break;
      case VM_visit_host:
      case VM_visit_policy:
	$dialog = array();
	if (empty($this->endorses)) $this->get_endorses($dialog);
	if (in_array(bAuth::$av->ID,array_keys($this->endorses))) $reply = True;
	if ($e_id=$this->getValue('e_id',1))    $reply = $reply || bAuth::$av->isOrganizer($e_id);                                                                                                      
	break;      
      }
    }
    $this->dbg($reply,$this->debug || cnf_dev);
    return $reply;
  }
  
}
