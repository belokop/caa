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
   * WM - Welcome Mail
   */
  public function upload_mailAttachment($id='WM'){
    static $name = '_virt_att';

    $c = VM::$formDummy;

    self::$_debug = $c::$_debug = True;

    VM::$formDummy->defineVariable('Please select the PDF file to be attached',$name);
    VM::$formDummy->defineTypes(array('expected_ext'=>array($name=>'.pdf')));
    VM::$formDummy->expected_ext[$name] = '.pdf';
    VM::$formDummy->dbg(VM::$formDummy->expected_ext,cnf_dev);

    ob_start();     
    VM::$formDummy->uploadFile($name);
    $dialog = ob_get_contents();
    ob_end_clean();
    var_dump($dialog);
    
    print x("span class='only_online'",
	    x("form action='".b_url::same('?resetcache_once=1')."' method='post' enctype='multipart/form-data' name='upload_mailAttachment'",
	      VM::$formDummy->uploadFile($name).//	      $dialog.
	      "<input type='hidden' name='_post_bForm_ID_once' value='".$this->ID."'/>".
	      "<input type='submit' value='submit'/>"));
  }

  
  /*
   * Check does the avatar have visit with the active policy,
   * say, with paid trip 
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

    // Force status update. This should be done as early as possible, so do it now...
    if ($this->myPost_bForm() && empty($_GET['a_once']) && 
	(($w=(string)@$this->formDB['v_status']) !==
	 ($n=(string)$this->getValue('v_status',True,True)))){
      $_GET['a_once'] = $n;
      $force_status = True;
      $this->dbg("Force v_status update $w --> $n",cnf_dev);
    }

    // Follow the endorser will
    if (isset($_GET['a_once'])){
      $this->dbg("Accepted? ".$_GET['a_once'],cnf_dev);
      $this->setStatus($_GET['a_once'],@$force_status);
      if ($_GET['a_once'] == STATUS_PENDING) $_SESSION[get_class($this).$this->ID] = '...';
    }

    // Hook the Avatar & Event which he is attending
    // $this->hookDependancies();
    $this->hookAvatar();
    $this->hookEvent();

    // Check the auto-accept
    $this->auto_accept_guest_invitations();
  }

  /*
   * Since 2017-03-20, a new head of administration...
   * New way of inviting guest researchers - no group leader confirmation is required. 
   */
  private function auto_accept_guest_invitations(){
    $reply = auto_accept_guest_invitations && ($this->getValue('v_type',1,1) === VISIT_TYPE_COLLABORATION);
    if ($reply &&
	$this->getValue('v_start',1) && 
	$this->getValue('v_end',1)){      
      // Check that the visit is not clashing with another one
      if ($this->getClashingVisits(True)){
	$this->getValue_reset_forced('v_status',STATUS_PENDING);
      }else{
	// Set status to "accept", unless the visit is already rejected
	$v_status = $this->getValue('v_status');
	if (($v_status != STATUS_NO) && ($v_status != STATUS_YES)){
	  $this->hookAvatar();
	  $this->getValue_reset_forced('v_status',STATUS_YES);
	  if (cnf_dev) myPear::MESSAGE("Visit of ".$this->av->fmtName('fL')." is auto-accepted");
	  b_debug::traceBack('resetting v_status to STATUS_YES');
	}
      }
    }
    $this->dbg($reply,True);
    return $reply;
  }
  
  /*
   *
   */
  function __clean_cache(){
    if (is_object($this->av))          $this->av->__clean_cache(); 
    if (is_object($this->exp))         $this->exp->__clean_cache(); 
    foreach($this->get_allLeases(True) as $l) $l->__clean_cache();
    myPear::getInstance_new(get_class($this),$this->ID,'clean');
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
    return self::_getPolicies(array('e_v_policy'=> VM_visit_policies($v_type,Null,$this->e),
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
	b_debug::_debug("Got unknown policy '$p', ignore...");
      }else{
	if (!cnf_dev) if ($p == VM_V_payPerdiem) continue;
	if (!cnf_dev) if ($p == VM_V_payOther)   continue;
	$policies[$p] = VM::$v_policies[$p];
      }
    }
    // b_debug::_debug(b_fmt::joinX(',',$policies));
    return $policies;
  }

  /*
   * Reg-Exp search thru GSF
   */
  function getGSF($what='travel.*support',$li='<li>'){
    $gsf = self::_getGSF($what,array('v_gsf'=>$this->getValue('v_gsf',True)),$li);
    if (empty($gsf))$reply = '';
    else  $reply = x('span class=smaller-text',empty($li)
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
   * (i.e. is the letter sent, should the per-diem to be paid, etc.)
   *
   * The policy might be 'locked', 
   * which means that all the decisions about the visit are signed,
   * no more changes is allowed.
   *
   * @param $policy - (int) policy code
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

    $reply = False;
    if (in_array($policy,array_keys(VM::$v_policies))){
      if (is_object($visit)){
	$buffer = array('id'         => $visit->__toString().'->',
			'v_policy'   => $visit->getValue('v_policy',True),
			'av_lastname'=> $visit->av->getValue('av_lastname',True,True));
      }elseif(!is_array($buffer)){
	$buffer = array('id'         => '',
			'av_lastname'=> 'anonymous',
			'v_policy'   => $buffer);
      }
      if (!empty($buffer['v_policy'])){
	$reply = (bool)b_mask::get($buffer['v_policy'],$policy);
	if(cnf_show_pv || ($onlyIfExpected === 'verbose')){
	  b_debug::_debug(VM::$v_policies[$policy]['d'].($reply?' YES ':' NO  '));
	}
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
      if (cnf_dev) b_debug::traceBack(__function__."() - recursion".$this->setPolicyRecursion);
      return;
    }

    // Sanity...
    $_virt_policy = '_virt_policy_'.$policy;
    unset($_POST[$_virt_policy]);
    $this->getValue_reset($_virt_policy);

    // Sanity, fix eventual mistypes of the organizers
    // Temporarily commented out...
    // ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ YB ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
    if ($this->getPolicy(VM_V_denialMailSent)  && !$this->isDenied())   b_debug::traceBack('$this->setPolicy(VM_V_denialMailSent, 0,cnf_dev,False,True)');
    if ($this->getPolicy(VM_V_welcomeMailSent) && !$this->isApproved()) b_debug::traceBack('$this->setPolicy(VM_V_welcomeMailSent,0,cnf_dev,False,True);');
    //    if ($this->getPolicy(VM_V_denialMailSent)  && !$this->isDenied())   $this->setPolicy(VM_V_denialMailSent, 0,cnf_dev,False,True);
    //    if ($this->getPolicy(VM_V_welcomeMailSent) && !$this->isApproved()) $this->setPolicy(VM_V_welcomeMailSent,0,cnf_dev,False,True);
    // ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ YB ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
    
    //
    // Cancel the visit policy if the event policy is off, say
    // no visit office allocation if the Event policy is "no office"
    foreach(VM::$E_V_policies as $p_v=>$p_e){
      if ($policy == $p_v){
	if ($this->hookEvent() && !$this->e->getPolicy($p_e)) $value = 0;
	if (empty($value)){
	  switch($p_e){
	  case VM_E_provideOffice:
	    // Remove office allocation if the Visit policy is "no office"
	    $this->getValue_reset_forced('v_oid');
	    $this->getValue_reset('_o_name');
	    break;
	  case VM_E_payTrip:
	  case VM_E_provideLunches:
	  case VM_E_conferenceDinner:
	    break;
	  default:
	    b_debug::internalError("Unexpected visit policy '$policy'");
	  }
	}
      }    
    }
    
    // Now set the requested policy
    if (defined('VM_prg_coordinator_here') && 
	(!(VM_prg_coordinator_here||VM_organizer_here) || 
	 (@VM::$v_policies[$policy]['t'] === auto_maintained) ||
	 ($this->getValue('v_end') < time()))) $verbose = False;
    
    
    if ($isChanged = $this->setBitMask($policy,'v_policy',$value,$check_only=True)){
      $this->setBitMask($policy,'v_policy',$value);
      if ($verbose || cnf_show_pv){
	if ($policy == VM_V_payAccommodation){
	  if(0)	  if (cnf_dev) b_debug::traceBack();
	}else{
	  myPear::MESSAGE_add(VM::$v_policies[$policy]['d']." ".x('em',($value?'YES':'NO')." for ".$this->av->fmtName()));
	}
      }
    }
    $this->setPolicyRecursion = 0;
  }
  
  /*
   * Set the status
   */
  public function setStatus($v_status=Null,$forced=False){
    if (!myPear::is_int($this->ID)) return;
    
    $was = $this->getValue('v_status');
    if (!$forced && ($was === $v_status)){
      $this->dbg("Status is not changed... $v_status",cnf_dev);
      return;
    }
    
    // Save the new status
    $this->getValue_reset_forced('v_status',$v_status);
    $this->dbg("v_status $was->$v_status",cnf_show_pv || cnf_dev);

    switch($v_status){
    case STATUS_NO:
    case STATUS_PENDING:
      unset($_POST['lease_id']);
      $this->deleteBooking(Null,$verbose=True);
    case STATUS_YES:
      // cancel/set policies of "when_approved" type
      $this->hookEvent();
      foreach(VM_visit_policies($this->getValue('v_type'),Null,$this->e) as $p){
	if (@VM::$v_policies[$p]['t'] === when_approved) $this->setPolicy($p,($v_status==STATUS_YES?1:0),True);
      }

      // cancel all the scholarships 
      if ($v_status !== STATUS_YES) foreach (array_keys(VM::$reimbursements) as $p) $this->setPolicy($p,0,True);
      break;
      
    default:
      myPear::WARNING_T("Unexpected status '$v_status'",$this);
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

     $reply = self::_getStatus($buffer,$this);
     //??????     if ((int)$this->ID)    $this->setStatus($reply);

     // Set also flag 'the approval formalities are done' if the status is auto-set to 'accepted'
     if ($reply === STATUS_YES){
       if ((int)$this->ID && empty($v_status)) $this->setPolicy(VM_V_welcomeMailSent,True,False);
     }

     $this->dbg($reply,cnf_show_pv);    
     return $reply;
   }

   /*
    *
    */
   static public function _getStatus($buffer=array(),$check_clashes=True){

     // Skip the visits which are not yet in the database
     if ((int)@$buffer['v_id'] && !empty($buffer['v_type'])){
       if (!is_object($check_clashes) && $check_clashes && self::_getClashingVisits($buffer,$verbose=False)){
	 $v_status = STATUS_PENDING;
       }else{

	 // Guess the status value if it is not defined yet,
	 // make an auto-approval for the transition period and for the past events
	 $v_status = @$buffer['v_status'];
	 $v_start  = @$buffer['v_start'];

	 if (empty($v_status)){
	   switch($buffer['v_type']){
	   case VISIT_TYPE_RENT:  
	     $v_status = STATUS_YES;   
	     break;
	     
	   case VISIT_TYPE_COLLABORATION:
	     if ($acceptIt = ($check_clashes instanceof bForm_vm_Visit
			      ? $check_clashes->auto_accept_guest_invitations()
			      : auto_accept_guest_invitations)){
	       if (is_object($check_clashes)) $v_status = $check_clashes->getValue('v_status',1);
	       elseif (($v_status != STATUS_YES) && $v_status != STATUS_NO) $v_status = STATUS_YES;   
	       break;
	     }
	   case VISIT_TYPE_PROGRAM:  
	   case VISIT_TYPE_OTHER: 
	     $v_status = STATUS_PENDING;
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
       $this->status_data = array('v_id' => $this->ID);
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
     if ((int)$this->ID) $reply = self::_isApproved($this->is_status_data(),$this);
     else                $reply = True;
     $this->dbg($reply);
     return $reply;
   }

   static function _isApproved($buffer,$class=Null){
     $reply = (self::_getStatus($buffer,$class)== STATUS_YES);
     return $reply;
   }

   /*
    *
    */
   function isDenied(){
     $reply = self::_isDenied($this->is_status_data(),$this);
     $this->dbg($reply);
     return $reply;
   }

   static function _isDenied($buffer,$class=Null){
     $reply = (self::_getStatus($buffer,$class) == STATUS_NO);
     return $reply;
   }

   /*
    *
    */
   function isPending(){
     $reply = (self::_getStatus($this->formDB,$this)== STATUS_PENDING);
     return $reply; 
   }


   /*
    * Hook all the leases (leases) for this visit
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
	   if (empty($v)){
	     if (!is_object($v_id) && !is_numeric($v_id)) b_debug::internalError("??? v_id = ".var_export($v_id,True));
	     $v = (is_object($v_id) ? $v_id : new bForm_vm_Visit($v_id));
	   }
	   $l = new bForm_vm_Lease($lease_id,$v);
	   $l->hookVisit($v);
	   $reply[] = $l;
	 }elseif($mode === 'hut_codes'){
	   $reply[] = $hut_code;
	 }else{
	   if ($hut_code !== HUT_BI) $reply[] = $lease_id;
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
     if (strpos($block_ID,  VM_visit_lease) !== False) $block_ID   = VM_visit_lease;

     // Show on one block if requested
     if ($block_only = @$_GET['block_once']){
       if (($block_only == VM_visit_expenses) && ($this->MODE == 'RO')) $block_only = $_GET['block_once'] = '';
       if (strpos($block_only,VM_visit_lease) !== False) $block_only = VM_visit_lease;
       if ($block_only != $block_ID) return False;
     }

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
	 if (!$this->getClashingVisits(0) && (int)$this->ID && $this->isPending()) myPear::WARNING("The visit is NOT yet approved");
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
	 if (0){ // KTH rules
	   $this->MODE = 'RW';
	   $reply = True;
	 }else{ // SU rules
	   $this->hookEvent();
	 }
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
       if (!parent::formBlock_isVisible($block_ID,$fields) || $this->isDenied() || $this->visitOutOfScope()){
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

     case 'Office':
       $reply = True;
       break;

     default:
       $reply = parent::formBlock_isVisible($block_ID,$fields);
       if (!empty($block_ID)) myPear::WARNING_T("Unexpected block '$block_ID'");
     }

     $this->dbg($reply,cnf_show_st);
     return $reply;
   }
   
   /*
    *
    */
   function hookEvent(){
     if ($this->isVisitType_program() &&
	 ($v_eid = $this->getValue('v_eid', True)) &&
	 !($this->e instanceof bForm_vm_Event)){
       $this->e = myPear::getInstance_new('bForm_vm_Event',$v_eid,'fatal');

       // Force the project id if known       
       locateAndInclude('bList_vm_projects');
       $v_projectid = bList_vm_projects::e2project_id($this->e->ID);
       if ($this->getValue('v_projectid') != $v_projectid){
	 myPear_db()->qquery("UPDATE abs_visits SET v_projectid = '$v_projectid' WHERE v_eid = ".$this->e->ID,cnf_dev);
       }
     }
     $reply = (is_object($this->e)
	       ? $this->e
	       : False);
     $this->dbg("".$reply,True);
     return $reply;
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
     
     // Ask for the residentship country if the travel is expected to be reimbursed
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
				//'virt_host'           => 'Host',
				'v_accompaning'       => 'Accompanying persons',
				'v_chance'            => 'Probability of visit',
				'v_comment'           => 'Comment (purpose of visit, lecture title, etc.)',
				'v_acc_wish'          => 'Accommodation wish',
				'v_created'           => 'Request date',
				'v_group'             => 'Hosting Research group',
				'v_oid'               => 'Office number',
				'_o_name'             => 'Office number',
				'v_projectid'         => 'Project to pay from',
				'v_start'             => 'Start of visit',
				'v_end'               => 'End of visit',
				'v_status'            => 'Is visit approved?',
				'v_type'              => 'Type of the visit',
				'v_eid'               => 'v_eid',       
				'v_avid'              => 'v_avid',
				'v_code'              => 'v_code',
				'v_owner_avid'        => 'Request owner',
				'v_admin_avid'        => 'v_admin_avid',
				'v_policy'            => 'v_policy',  
				'v_gsf'               => ' ',
				'_virt_owner'         => 'Request owner',
				'_virt_updated_by'    => 'Updated by',
				'lease_aid'           => 'Accommodation',
				),True);

    $this->defineTypes(array('submitOnChange'=>array('v_group' => !myPear::is_empty($this->getValue('v_group',1))),
			     'isADMIN'       =>array('v_comment'),
			     'isMBE'         =>array('v_projectid', 
						     'v_acc_wish',
						     'v_comment',
						     'v_chance',
						     'v_oid',
						     'v_code',
						     'v_policy',
						     'v_eid'      => ($this->getValue('v_type',1,1)!= VISIT_TYPE_PROGRAM),
						     'v_host_avid'=> ($this->getValue('v_type',1,1)!= VISIT_TYPE_COLLABORATION),
						     '_virt_owner'),
			     'isRO'          =>array('v_status' => !VM::hasRightTo('approve_guest_invitation',$this),
						     'v_start'  => ($this->getValue('v_type',1,1)== VISIT_TYPE_PROGRAM),
						     'v_end'    => ($this->getValue('v_type',1,1)== VISIT_TYPE_PROGRAM),
						     'v_type'   => ($this->getValue('v_type',1,1)== VISIT_TYPE_PROGRAM),
						     'v_eid',						
						     'v_gsf',					
						     'v_owner_avid',
						     'v_created',
						     'v_admin_avid',						
						     'e_name',
						     'v_oid'        => !VM::hasRightTo('edit_visit',$this),
						     '_virt_updated_by',
						     '_virt_owner'),
			     'isPRIVATE'     =>array('v_owner_avid',
						     //'v_accompaning',
						     'v_type',
						     //'v_acc_wish',
						     '_virt_updated_by',
						     '_virt_owner'),
			     'isVITAL'       =>($this->visitOutOfScope()
						? array()
						: array('v_start',
							'v_end',
							'v_type',
							'v_group'    => ($this->getValue('v_type',1,1)== VISIT_TYPE_COLLABORATION),
							'v_host_avid'=> ($this->getValue('v_type',1,1)== VISIT_TYPE_COLLABORATION),
							)),
			     //'keepRW'        =>array('v_type'),
			     ));
    
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
  }
  
  function readCompleteDefRecord(){
    $q = myPear_db()->query("SELECT * FROM $this->TABLE ".
			    " LEFT JOIN zzz_list_members ON v_projectid=lm_status ".
			    " LEFT JOIN abs_expenses     ON exp_vid=v_id ".
			    " LEFT JOIN abs_leases       ON lease_vid=v_id ".
			    " LEFT JOIN abs_rooms   ON lease_aid=a_id ".
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
  private static function _hut_codes($buffer,$without_BI=False){
    $reply = array_values(self::_v_get_allLeases($buffer['v_id']));
    if ($without_BI){
      if (empty($buffer['v_eid'])){
	$reply = array();
      }else{
	$e = myPear::getInstance_new('bForm_vm_Event',$buffer['v_eid'],'fatal');
	//$reply = array_diff($reply,array(HUT_BI),array_values($e->accommodationOptions()->get_BI(True)));
	$reply = array_diff($reply,array(HUT_BI),array_values(VM_accommodationOptions($e)->get_BI(True)));
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

    if (!$this->get_allLeases('count')) $this->dialog_accompaning($dialog,$this);
    
    // Show the block
    $this->formBlock(VM_visit_info,
		     'Visit information',
		     $dialog,
		     array('i'=>'i-portfolio_edit',
			   'l'=>$this->sameURL_but(array('mode'=>$mode))));
  }

  /*
   *
   */
  function dialog_accompaning(&$dialog,$class){
    // Accompanying persons
    if ($_virt_accompaning = $this->getGSF($trigger='accompan','')){
      
      $n = trim(str_replace(array('(',')'),' ',self::$_getGSF[$trigger]));
      if (is_int($n))                           $v_accompaning = $n;
      elseif(preg_match('/\b(four|4)\b/i',$n))  $v_accompaning = 4;
      elseif(preg_match('/\b(three|3)\b/i',$n)) $v_accompaning = 3;
      elseif(preg_match('/\b(two|2)\b/i',  $n)) $v_accompaning = 2;
      elseif(preg_match('/\b(one|1)\b/i',  $n)) $v_accompaning = 1;
      if (isset($v_accompaning) && $v_accompaning > 0){
	$this->getValue_reset_forced('v_accompaning',$v_accompaning);
	$class->defineVariable(array('v_accompaning'=>'Accompanying persons'),True);
	$class->getValue_reset('v_accompaning',$v_accompaning);
	$class->isRO['v_accompaning'] = True;
	$dialog['v_accompaning'] = array('selectMenu',array(0=>'coming alone',
							    1=>'me + 1',
							    2=>'me + 2',
							    3=>'me + 3',
							    4=>'me + 4',
							    ));
      }
    }

    // Show the v_accompaning wish
    if (0) if (!empty($_virt_accompaning)){
      $this->defineVariable(array('_virt_accompaning'=>' '),True);
      $this->formDB['_virt_accompaning']= $_virt_accompaning;
      $this->isRO['_virt_accompaning']  = True;
      $dialog['_virt_accompaning'] = array('textField');
    }
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
		    array('av_email'=>$this->getValue('av_email',1,1))) as $arg) if ($this->av = bForm_Avatar::hook($arg)) break;
      if(is_object($this->av)){
	// Set the office (for the visit period) for staff members
	bForm_Avatar::set_context($this->av,VM_MODULE);
	$this->av->hook_employment();
	if ($this->av->isE(True)){
	  $office = $this->av->get_staffOffice();
	  if (!empty($office)){
	    $o = myPear::getInstance_new('bForm_vm_Office',array('o_name'=>$office),'nocache');
	    if ($this->getValue('v_oid') != $o->ID){
	      $this->setPolicy(VM_V_provideOffice,1);
	      $this->getValue_reset_forced('v_oid',$o->ID);
	      myPear::MESSAGE("Office ".x('em',$office).' - '.$this->av->fmtName('fl'));
	    }
	  }
	}	
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
  public function getValue($name, $asIs=True, $d_MBE=True){
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
	/*
	// better to set isMBE / isVITAL
      case 'v_host_avid':
	if (empty($this->formDB['v_group'])) $err_msg = '';
      case 'v_group':
	if (!in_array(@$this->formDB['v_type'],array(VISIT_TYPE_OTHER,
						     VISIT_TYPE_COLLABORATION)))  $err_msg = '';
	break;
	*/	
      case 'av_email': // workaround... Should find where it comes from
      case 'v_comment':
      case 'v_acc_wish':
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
      if (($e_id=$this->getValue('v_eid')) > 0){
	// Visit for a regular event (program), to be approved by the event organizers
	$this->endorses[] = 'Event organizers:';
	foreach(VM_organizers($e_id)->getMembers() as $um_id=>$rec){
	  $this->endorses[$rec['um_avid']] = '&nbsp;'.bForm_Avatar::_fmtName('Lf',$rec);
	}
      }else{
	// another type of a visit
	foreach(VM_endorsers()->getEndorsers($this->getValue('v_group')) as $av_id){
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
    if ($v_gsf = $this->getValue('v_gsf')){    
      $dialog = array();
      $v_gsf = unserialize($v_gsf);
      $accommodation = (($v_acc_wish = $this->getValue('v_acc_wish')) 
			? array('Accommodation' => $v_acc_wish)
			: array());
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
    if (!$this->getValue('v_status')) $this->getValue_reset('v_status',STATUS_PENDING);

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
	// The event policy has precedence over the visit policy
	if (!empty($this->e) && (($p_e = @VM::$E_V_policies[$p]) && !$this->e->getPolicy($p_e))) continue;
	if (in_array($p,VM::$auto_maintained)) continue;
	
	$k = '_virt_policy_' . $p;
	if ($this->exp_arePaid()) $this->isRO[$k] = True;
	$this->defineVariable($descr['d'],$k);
	$this->formDB[$k] = ($this->getPolicy($p) ? 'yes' : 'no');
	$dialog[$k] = array('checkBox',0,
			    'i-finger_up-small', 
			    'i-finger_down-small');
      }
	  /*
	if ($this->MODE == 'RW' || ($value==='yes' || $this->isWritable())){
	  if ($this->exp_arePaid()) $this->isRO[$k] = True;
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
			   'i'=>($this->exp_arePaid()?'i-lock':'i-user1_edit'),
			   'l'=>$this->sameURL_but(array('mode' =>'RW'))));
  }
  
  /*
   * Show accommodation ('lease' objects)
   */
  private function accommodationBlock(){
    if (!$this->visitOutOfScope()){
      $all_leases = $this->get_allLeases();
      if (empty($all_leases) && ($this->getValue('v_type')==VISIT_TYPE_COLLABORATION)){
	$this->setBooking(VM::$OA->ID,$verbose);
      }else{
	foreach($all_leases as $l) $l->getForm();
      }
    }
  }
  
  /*
   * Project to pay from
   */
  function projectBlock(){
    
    if (!$this->auto_accept_guest_invitations() && $this->isApproved()){
      $where = array("lm_option != 'no'");
      if ($this->e) $where[] = "(lm_status = ".$this->e->ID." OR lm_status = 'RO')";
      else          $where[] = "(lm_status = '' OR lm_status = 0 OR lm_status = 'RO')";
      
      if ($this->isVisitType_program()){
	$this->defineVariable('Project(s) to pay from ','_v_projectid');
	$this->isRO['_v_projectid'] = True;
	$this->getValue_reset('_v_projectid',sprintf("%s - %s",$this->getValue('v_projectid'),$this->e->name()));
      }
      $this->formBlock(VM_visit_project,
		       'Budget source',
		       ($this->isVisitType_program()
			? array('_v_projectid' => array('textField'))
			: array( 'v_projectid' => array('selectMenu_bList',VM_projects(),$where,'project'))),
		       array('d'=>btn_edit_project,
			     'i'=>'i-wallet',
			     'l'=>$this->sameURL_but(array('block'=>'project','mode'=>'RW'))));
    }
  }

  /*
   * Office
   */  
  private function officeBlock(){
    if (!empty($this->e) && 
	$this->getPolicy(   VM_V_provideOffice) &&
	$this->e->getPolicy(VM_E_provideOffice)){
      if (($s = @$this->getValue('v_start')) && ($e = @$this->getValue('v_end'))){
	locateAndInclude('bForm_vm_Office');
	$v_oid = $this->getValue('v_oid');
	$this->isRO['_o_name'] = $this->av->isE(True) && !empty($v_oid);
	if ($o = myPear::getInstance_new('bForm_vm_Office',$v_oid,array('strict','nocache'))){
	  $this->formDB['_o_name'] = $o->name(False);
	}elseif($this->MODE == 'RO'){
	  $this->formDB['_o_name'] = b_fmt::redText('not allocated yet');
	}
	$this->formBlock('Office',
			 'Office',
			 array('_o_name' => array('textField')),
			 array('d'=>'set office',
			       'i'=>'i-desktop',
			       'l'=>$this->sameURL_but(array('block'=>'Office','mode'=>'RW'))));
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
      if(!$this->auto_accept_guest_invitations() && 
	 !$this->getValue('v_projectid') && 
	 ($this->getValue('v_end') > time()-AUTO_EXPIRATION*86400) &&
	 ($this->isWritable())){
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
  
  /*
   *
   */
  private function whatElseBlock(){
    $this->formBlock(VM_visit_whatElse,
		     'Comments', // 'What else can we get from the visit',
		     array('v_comment' =>'textArea'),
		     array('i'=>'i-user4_edit',
			   'l'=>$this->sameURL_but(array('mode'=>'RW'))));
    
  }

  /*
   * Visit hosting
   */
  private function hostingBlock($only_groups=False){ 
    if (!is_object(bAuth::$av)) return;
    
    // Prepare a list of valid hosts - the group members + the owner + the endorsers
    $valid_hosts = (array(bAuth::$av->ID => bAuth::$av->fmtName()) +
		    myOrg()->orgGroups()->get_groupMembers($this->getValue('v_group')));
    if (!auto_accept_guest_invitations) $valid_hosts += VM_endorsers()->getEndorsers(Null,'full');

    if (($v_host_avid = $this->getValue('v_host_avid')) && empty($valid_hosts[$v_host_avid])){
      if ((int)$v_host_avid){
	$av = myPear::getInstance_new('bForm_Avatar',$v_host_avid,array('nocache','strict'));
	if (is_object($av)) $valid_hosts[$av->ID] = $av->fmtName();
      }else{
	$this->getValue_reset('v_host_avid');
      }
    }
    asort($valid_hosts);

    // Dialog
    $this->formBlock(VM_visit_host,
		     'Visit hosting',
		     array('v_group'    => array('selectMenu', myOrg()->orgGroups()->get_groups()),
			   'v_host_avid'=> array('selectMenu',$valid_hosts),
			   '_virt_owner'=> array('textField')),
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
    if ($av = myPear::getInstance_new('bForm_Avatar_vm',$this->getValue('v_admin_avid',1,1),'strict')){
      $this->formDB['_virt_updated_by'] = $av->fmtName('fl',0).', '.b_time::medium_date($this->getValue('v_timestamp'));
      $this->textField('_virt_updated_by');
    }
  }

  /*
   * Visit dialog
   */
  function getForm(){

    // Sanity, catch invocation with an arbitrary v_id
    if (!$this->hookAvatar()){
      $this->dbg("Can't hook Avatar for the visit",cnf_dev);
      return;
    }

    // Hook dependencies
    $this->getFormOpen();
    $this->hookDependancies();
    $this->av->defineVariables();

    // Inform the administration about new guest invitation, if any
    if ($this->errorless() && ($this->getValue('v_type') == VISIT_TYPE_COLLABORATION)){
      // Check the auto-accept first
      $this->auto_accept_guest_invitations();
      // Send the mail(s) to administrators if not yet done
      VM_mailer()->inform_about_guest_invitation($this);
    }       
    
    // Start the dialog

    if (!$this->isApproved()){
      $this->tips['v_status'] = "The visit is not processed unless it is approved";
    }
    
    if (!$this->isEmbedded()) {
      myPear::H1($this->av->fmtName('sfL'),array('noTranslate','reset'));
      bForm::nameCounter();
    }


    // Do not allow any changes for the closed visits (closing is done by the finance) 
    $this->declareRO($this->exp_arePaid());
    
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
    if ($v_type = $this->getValue('v_type')){
      $this->dbg("v_type=$v_type",True);
      $this->periodBlock();
      $this->policyBlock();
      $this->accommodationBlock();
      switch($v_type){
      case VISIT_TYPE_OTHER:
      case VISIT_TYPE_POSSIBLE:
      case VISIT_TYPE_COLLABORATION:
	$this->hostingBlock();
      case VISIT_TYPE_PROGRAM:
	$this->officeBlock();
	if (empty($_GET['block_once'])) $this->expensesBlock();
	$this->projectBlock();
	$this->whatElseBlock();
      case VISIT_TYPE_RENT:
	break;
      }
	
      if (($date = $this->getValue('v_created',1)) && ($host=$this->getValue('v_owner_avid',1))){
	if ($av = myPear::getInstance_new('bForm_Avatar_vm',$host,'fatal')){
	  $this->formDB['_virt_owner'] = $av->fmtName('fl').' '.b_time::medium_date($date);
	}
      }
      
      $this->virt_updated_by();

    }elseif($this->MODE == 'RW'){
      /*
       * Request first the visit type,
       * since all the rest depends on it
       */
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
    
    if ($this->getClashingVisits(False)){
      //
      // Propose to delete this clashing visit
      //
      print b_btn::UL_buttons(array(b_btn::big_button('Cancel this visit',array('function'=>'vm_cancel_visit',
										'v_id'    =>$this->ID),
						      'red',True)));
    }elseif (!$this->visitOutOfScope() &&
	     empty($_SESSION[get_class($this).$this->ID]) &&
	     $this->isPending() &&
	     (($v_group=$this->getValue('v_group',1)) && in_array(bAuth::$av->ID,VM_endorsers()->getEndorsers($v_group)) ||
	      (is_object($this->e) && $this->e->isOrganizer()))
	     ){
      //
      // Easy button for endorsers
      //
      print b_btn::UL_buttons(array(b_btn::big_button('Deny visit',        array('form'  => get_class($this),
										 'id'    =>$this->ID,
										 'a_once'=>STATUS_NO,
										 'resetcache_once'=>1),
						      'red',True),
				    b_btn::big_button('I need to think...',array('form'  => get_class($this),
										 'id'    =>$this->ID,
										 'a_once'=>STATUS_PENDING,
										 'resetcache_once'=>1),
						      'grey',True),
				    b_btn::big_button('Approve visit',     array('form'  => get_class($this),
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
      $q = myPear_db()->query("SELECT o_id,o_capacity,o_name FROM abs_offices WHERE o_id>0 AND o_status='".OFFICE_AUTO_ALLOCATABLE."' ORDER BY o_name");
      while($r=myPear_db()->next_record($q)){
	$qq = myPear_db()->query("SELECT v_id FROM abs_visits WHERE ".
				    join(' AND ', array("v_oid = ".$r['o_id'],
							"v_start <= $end",
							"v_end   >= $start")));
	if ((myPear_db()->num_rows($qq) < $r['o_capacity']) ||
	    ($r['o_id'] == $this->formDB['v_oid']))$list[$r['o_id']] = $r['o_name'];
      }
    }else{
      $q = myPear_db()->query("SELECT o_id,o_name,o_status FROM abs_offices WHERE o_id>0 ORDER BY o_name");
      while($r=myPear_db()->next_record($q)){
	$list[$r['o_id']] = $r['o_name'];
	if ($r['o_status'] != OFFICE_AUTO_ALLOCATABLE) $list[$r['o_id']] .= ' - staff';
      }
    }
    if (($o_id=$this->formDB['v_oid']) && empty($list[$o_id])){
      if ($o = myPear::getInstance_new('bForm_vm_Office',$o_id,'strict')) $list[$o_id] = $o->name();
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
    $this->dbg($types,True);
    return $types;
  }

  /*
   * Canceling the visit
   */
  function delete(){
    $this->dbg($this->av->name(),cnf_dev);
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
      // Delete the specified lease
      //
      if ($l = myPear::getInstance_new('bForm_vm_Lease',$lease_id,'strict')){
	$lease_start_day = b_time::_($l->getValue('lease_start'));
	$lease_end_day   = b_time::_($l->getValue('lease_end'));
	if ($verbose) myPear::MESSAGE_add('Delete lease '.$l->_periodToString());
	$l->delete();
	
	// Extend the neighboring lease period
	foreach($this->get_allLeases() as $l){
	  $ls = b_time::_($l->getValue('lease_start',1));
	  $le = b_time::_($l->getValue('lease_end',1));
	  if(($ls == $lease_start_day) && ($le == $lease_end_day)) continue;
	  if ($le == $lease_start_day) {
	    if ($verbose) $l->updateDB(array('lease_end'  =>b_time::check_out($lease_end_day)));
	    myPear::MESSAGE_add("Modify lease end ".$l->_periodToString());
	    break; 
	  }
	  if ($ls == $lease_end_day)   {
	    $l->updateDB(array('lease_start'=>b_time::check_in($lease_start_day)));
	    if ($verbose) myPear::MESSAGE_add("Modify lease start ".$l->_periodToString());
	    break;
	  }
	}
      }else{
	myPear::WARNING_T("Cant find the lease",$this);
      }
    }else{
      //
      // Delete all leases
      //
      $leases = $this->get_allLeases();
      if (empty($leases)){
	$this->dbg('No leases to delete...',cnf_dev);
      }else{
	foreach($leases as $l) {
	  $l::$_debug = self::$_debug;
	  $msg = $l->_periodToString();
	  $this->dbg($l->__toString().' '.$msg,cnf_dev);
	  if ($verbose && $msg) myPear::MESSAGE_add(b_fmt::redText("Delete lease $msg"));
	  $l->delete();
	}
      }
    }
    $this->get_allLeases(True);
    $this->deletingBooking = False;
  }

  function setResource($what,$id,$dates=array(),$verbose=True){
    switch($what){
    case 'Offices':  
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
      $a = myPear::getInstance_new('bForm_vm_Room',$lease,'fatal');
      $lease = array('lease_aid'  =>$a->ID,
		     'lease_start'=>$this->getValue('v_start'),
		     'lease_end'  =>$this->getValue('v_end'));
    }
    $lease['lease_vid'] = $this->ID;
    
    // Sanity
    if (cnf_dev){
      if (myPear::getClassFromID($lease['lease_aid'])!='bForm_vm_Room') b_debug::internalError("setBooking($lease) is not bForm_vm_Room",$this); // sanity
      if (!$lease['lease_start'] ||!$lease['lease_end'])                b_debug::internalError("setBooking($lease) no start/end time") ;           
    }
    
    // Create the lease (lease) if it does not exist      
    if (!$this->leaseExists($lease)){
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
	$l->hookRoom($lease['lease_aid'],$verbose);
      }
      
      // Sanity
      if (!$this->leaseExists($lease)){
	myPear::MESSAGE_add(b_fmt::redText("Can't set lease '".b_fmt::joinX(', ',$lease)."' for ".$this->_periodToString()));
      }
    }
  }
  
  /*
   * Helper
   */
  private function leaseExists($lease=array()){
    locateAndInclude('bForm_vm_Lease');
    if (!$lease) $lease = array();
    $lease['lease_vid'] = $this->ID;
    $reply = (count(bForm_vm_Lease::get_ids($lease)) > 0);
    $this->dbg($reply);
    return $reply;
  }

  function exp_arePaid($lock=Null){
    $this->hookDependancies();
    $reply = (bool)(is_object($this->exp) && $this->exp->exp_arePaid());
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * The "external events" (those which have positive visit_code) are
   * not considered as clashes
   */ 
  static  $savedWhere = Null;
  function getClashingVisits_byDate($start,$end,$verbose=True) {

    if (True){      // Since 2017-03-28
      $reply = self::_getClashingVisits(array('v_start' => $start,
				       'v_end'   => $end,
				       'v_id'    => $this->getValue('v_id',True),
				       'v_avid'  => $this->getValue('v_avid',True)),
				 $verbose);
      
    }else{         // Before 2017-03-28
      $reply = self::_getClashingVisits_byDate($this->ID, $this->getValue('v_code',1), $this->getValue('v_type',1), $this->getValue('v_avid',1),
					       $start,$end);
      $this->queryString = self::$savedWhere;
    }
    $this->dbg($reply,True);
    return $reply;
  }

  /*
   * Detect clashes in visit dates (only for the invited guests, not event attenders)
   */
  function getClashingVisits($verbose=True){
    // Get the data buffer
    foreach(array('v_id','v_start','v_end','v_avid') as $f) $buffer[$f] = $this->getValue($f,True);
    // Call the engine
    $clashes = self::_getClashingVisits($buffer,$verbose);
    $this->dbg($clashes,True);
    return $clashes;
  }
  
  static function _getClashingVisits($buffer=array(),$verbose=False){

    $v_id = $buffer['v_id'];    
    if (!(int)$v_id) return array();

    static $clashes = array();
    if (!isset($clashes[$v_id])){
      $clashes[$v_id] = array('msg' => array(),
			      'ids' => array());
      $buffer['v_start'] = b_time::txt2unix($buffer['v_start']);
      $buffer['v_end']   = b_time::txt2unix($buffer['v_end']);
      
      $where = array("v_id    !=  $v_id",
		     "v_start <= '$buffer[v_end]'",
		     "v_end   >= '$buffer[v_start]'",
		     myPear_db()->quote(VISIT_TYPE_COLLABORATION,1,'v_type'),
		     myPear_db()->quote($buffer['v_avid'],1,'v_avid'),
		     );
      
      $q = myPear_db()->qquery("SELECT av_lastname,av_firstname,av_institute,v_id,v_start,v_end FROM abs_visits ".
			       " LEFT JOIN zzz_avatars  ON v_avid = av_id ".
			       " WHERE ".join(' AND ',$where),cnf_dev);
      while ($r=myPear_db()->next_record($q)){
	if (empty($clashes[$v_id]['msg'])){
	  $clashes[$v_id]['msg'][] = x('strong','Overlap in visit dates detected:');
	  $clashes[$v_id]['msg'][] = 'Current visit: '.b_time::period($buffer['v_start'],$buffer['v_end']);
	  
	  locateAndInclude('bIcal_matrix_visits');
	  ob_start();
	  $bIcal = new bIcal_matrix_visits();
	  $bIcal->showLegend = False;
	  $bIcal->graphTitle = ''; // 'Clashing visits';
	  $bIcal->day1 = $buffer['v_start']-3*86400;
	  $bIcal->day9 = $buffer['v_end']  +3*86400;
	  $bIcal->imposedEntries[] = $buffer;
	}

	$bIcal->imposedEntries[] = $r;
	$bIcal->day1 = min($buffer['v_start']-3*86400,$bIcal->day1);
	$bIcal->day9 = max($buffer['v_end']  +3*86400,$bIcal->day9);
	
	$clashes[$v_id]['msg'][] = 'Clashing visit: '.b_time::period($r['v_start'],$r['v_end']);
	$clashes[$v_id]['ids'][] = $r['v_id'];
      }
      
      if (!empty($clashes[$v_id]['msg'])){
	// complete the bIcal graph
	$bIcal->show('<hr/><br/>');
	$bIcal_visits = ob_get_contents();
	ob_end_clean();

	$clashes[$v_id]['msg'][] = '&nbsp;';
	$clashes[$v_id]['msg'][] = x('strong','Please resolve the ambiguity.');
	$clashes[$v_id]['msg'][] = '&nbsp;';
	$clashes[$v_id]['msg'][] = $bIcal_visits;
      }
    }else{
      //b_debug::var_dump($clashes[$v_id],"dejaVu...");
    }

    // print the messages, but only once
    if ($verbose && !empty($clashes[$v_id]['msg'])){
      myPear::WARNING(join('<br/>',$clashes[$v_id]['msg']));
      $clashes[$v_id]['msg'] = array();
    }

    // return the list of clashing visit IDs
    $reply = $clashes[$v_id]['ids'];
    if ($reply) b_debug::_debug(join(', ',$reply)); 
    return $reply;
  }
  
  /*
   * Check that the visit does clash with other visits
   */ 
  static  $savedQuery = Null;
  private static function _getClashingVisits_byDate($v_id, $v_code, $v_type, $v_avid, $start,$end) {
    $clashes = array();
    if ($av = bForm_Avatar::hook($v_avid)){
      $where = array("v_id    != '$v_id'",
		     "v_type   = '$v_type'",
		     "v_start <= '$end'",
		     "v_end   >= '$start'",
		     "v_avid   = '$av->ID'");
      if ($v_code>0) $where[] = "(v_code <= 0 OR v_code IS NULL)";
      
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
	if ($ap = myPear::getInstance_new('bForm_vm_Room',$l->getValue('lease_aid',1),'strict')) return $ap->name();
      }
      break;
      
    case 'Office': 
      if ($o=myPear::getInstance_new('bForm_vm_Office',$this->formDB['v_oid'],'strict')) return $o->name();
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
    if (self::_visitOutOfScope($buffer) || VM_tooLateForEmail($buffer)){
      $reply = False;
    }else{
      $policies = self::_getPolicies($buffer);
      
      locateAndInclude('bForm_vm_Event');    
      $VM_E_endorsed = bForm_vm_Event::_getPolicy(VM_E_endorsed,$buffer);
      
      $reply = False;
      switch($status=self::_getStatus($buffer)){
      case STATUS_YES:
	if (send_welcomeMail_to_applicants && $VM_E_endorsed && !empty($policies[VM_V_welcomeMailSent]) &&
	    !VM_mailer()->getStatus($buffer['v_id'],VM_V_welcomeMailSent) &&
	    !VM_mailer()->getStatus($buffer['v_id'],VM_V_welcomeMailQueued)){
	  //!self::_getPolicy(VM_V_welcomeMailSent,$buffer) && 
	  //!self::_getPolicy(VM_V_welcomeMailQueued,$buffer)){
	  // The welcome letter should be, but has not been sent 
	  $reply = STATUS_YES;
	  b_debug::_debug("!VM_V_welcomeMailSent && !VM_V_welcomeMailQueued", cnf_show_fnc);
	}
	break;
	
      case STATUS_NO:
	if (send_denialMail_to_applicants && $VM_E_endorsed && !empty($policies[VM_V_denialMailSent]) &&
	    !VM_mailer()->getStatus($buffer['v_id'],VM_V_denialMailSent) &&
	    !VM_mailer()->getStatus($buffer['v_id'],VM_V_denialMailQueued)){
	  //!self::_getPolicy(VM_V_denialMailSent,$buffer) &&
	  //!self::_getPolicy(VM_V_denialMailQueued,$buffer)){
	  // The denial letter should be, but has not been sent 
	  $reply = STATUS_NO;
	  b_debug::_debug("!VM_V_denialMailSent && !VM_V_denialMailQueued", cnf_show_fnc);
	}
	break;
	
      default:
	b_debug::xxx($msg="??? unexpected status=".var_export($status,True));
	b_debug::print_r($buffer,$msg);
	//      b_debug::internalError("??? status=".var_export($status,True));
      case STATUS_PENDING:
	$reply = STATUS_PENDING;
	b_debug::_debug("STATUS_PENDING",cnf_show_fnc);
	break;
	
      }
    }
    b_debug::_debug((empty($reply) ? '&lt;OK&gt;' : $reply),cnf_show_fnc);
    return $reply;
  }
  
  /*
   * Check the errors in the form
   */
  function formOKspecific(){
    if (!$this->hookAvatar()) b_debug::internalError("Can't hook Avatar",$this);
    
    // There should be no owner for the external visits, force it to be empty
    if ($this->getValue('v_type',1) === VISIT_TYPE_PROGRAM){ 
      foreach(array('v_host_avid','v_owner_avid') as $item) $this->getValue_reset($item,0);
    }

    // Check for errors
    if (($d1=$this->getValue('v_start',1)) >= ($d9=$this->getValue('v_end',1)) && $d1) {
      $this->errorsInTheForm['v_end'] = x("'",$this->getDescr('v_start'))." must be before ".x("'",$this->getDescr('v_end'));
    }elseif ($clashes=$this->getClashingVisits(True)){
      $this->errorsInTheForm['v_start'] = 'Dates for the visit'.' '.b_time::period($d1,$d9).' '.'clash with another visit:';
      foreach($clashes as $v_id){
	$v = new bForm_vm_Visit($v_id);
	b_debug::xxx(b_time::period($v->getValue('v_start'),$v->getValue('v_end')));
      }
    }

    if (!VM_manager_here && !$this->isOwner()) $this->errorsInTheForm = array();
    //    foreach (array_keys($this->errorsInTheForm) as $f) $this->printErrorAsIs[$f] = True; 

    // set the defaults for a newly created visit
      if (!$this->getValue('v_created',1))         $this->getValue_reset('v_created',time());
      $this->getValue_reset('v_admin_avid',bAuth::$av->ID); 
    // Set the request owner
    if (!$this->getValue('v_owner_avid',1)) $this->getValue_reset('v_owner_avid',bAuth::$av->ID);

    // Update office number
    if ($_o_name = $this->getValue('_o_name',1,1)){
      $o = myPear::getInstance_new('bForm_vm_Office',array('o_name'=>$_o_name),array('strict','nocache'));
      if (is_object($o)){
	$this->getValue_reset('v_oid',$o->ID);
      }else{
	$this->getValue_reset_forced('v_oid');
	$this->getValue_reset('_o_name');
	myPear::ERROR(sprintf("Office <em>%s</em> is not known",$_o_name));
      }
    }
    

    // Save the avatar for a the visit to be created
    if ($this->errorless()){
      if ($v_avid = @$GET['v_avid'])      $this->getValue_reset('v_avid',$v_avid);
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
      bTiming()->cpu(__function__);
      $this->hookDependancies();
      $exp_owner = (is_object($this->exp)
		    ? ($this->exp->getValue('exp_owner_avid') == @bAuth::$av->ID)
		    : False);
      $v_end = $this->getValue('v_end',True);
      if (empty($v_end)) $v_end = time()+AUTO_EXPIRATION*86400;
      $this->isWritable = (bool)b_cnf::check_conditions(array(False => array(//'pending'    => !$this->isApproved(),
									     'archived'   => (is_object($this->e) ? $this->e->isArchived() : False),
									     'too late'   => ($v_end + AUTO_EXPIRATION*86400 < time()) && !VM_manager_here),
							      True  => array('endorser'   => (is_object(bAuth::$av) && 
											      in_array(bAuth::$av->ID,VM_endorsers()->getEndorsers($this->getValue('v_group',1)))),
									     'isOrganizer'=> VM_organizer_here,
									     'booker'     => VM_prg_coordinator_here,
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
    $this->dbg($reply);
    return $reply;
  }
  
}
