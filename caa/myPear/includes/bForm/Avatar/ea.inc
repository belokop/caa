<?php
/*
 * bForm_Avatar_ea - Employee's & Alumni Avatar class.       YB 2011-05-01
 *
 * At any given moment the EA-avatar might be either 'e' (EA_role_employee) or 'a' (EA_role_alumnus).
 * The state is defined by analyzing the avatar "employment record" (see class bList_eaEmpRecords) for the organization.
 *
 * During the carrier the EA-avatar might be a member of various organizations, 
 * the "employment record" keeps them all.
 *
 * The EA-avatars are members of the "EA unit" (see class bUnit_ea_members). 
 * This unit assigns the 'E' or 'A' state based on the most RECENT employment. 
 * This status is just a shortcut to improve performance, it does not have any structural load
 *   
 * The 'E' status might have a fine structure to distinguish between the "long term visitors"
 * and the "real" employee's. 
 */

/*
 * Auxiliary function
 */
function _ea_getAV($av_id=Null){
  
  // Sanity
  b_cnf::_unset('act_',True);
  b_cnf::_unset(array('ed_bList','edi_m_once','action_once','from_bForm','from_bList'));
  
  if (empty($av_id)) $av_id = $_GET['av_id'];
  if (is_object($av_id)) $av = (($av_id instanceof bForm_Avatar_ea)
				? $av_id
				: new bForm_Avatar_ea($av_id->ID));
  elseif(!empty($av_id)) $av = new bForm_Avatar_ea($av_id);
  
  if (!is_object($av)) b_debug::internalError("empty 'av_id'");
  return $av;
}

/*
 * See and edit the employment records
 */
function ea_updateER($av_id=Null){
  if (_ea_getAV($av_id)->hook_employment())   _ea_getAV($av_id)->hook_employment()->getForm();
}

/*
 * See & Edit the avatar.
 * For the employee & long term visitor ask for extra information (CV, Contact persons)
 */
function ea_myPage($av_id=Null){

  $av = _ea_getAV($av_id);


  if ($av->isE() || $av->isV()){
    //
    // Employee
    //
    locateAndInclude('b_tabs');
    $tabs = array('Personal data','CV','Employment');
    if (is_object(bAuth::$av)){
      if (!method_exists(bAuth::$av,'isE')) bForm_Avatar::set_context(bAuth::$av);
      if (bAuth::$av->isE() || bAuth::$av->isV()) $tabs[] = 'Contacts';
    }
    $b_tabs = new b_tabs_myPear($tabs);
    foreach($tabs as $k=>$tab){
      if ($b_tabs->body()){
        
        switch($k){
        case 0: // Personal data
	  $av->getForm();
          break;
          
        case 1: // CV
	  myOrg()->eaCV(myOrg_ID,$av)->show();
          break;
          
        case 2: // Employment
	  if ($av->hook_employment()) 	  $av->hook_employment()->getForm();
          break;
          
        case 3: // Contacts
          locateAndInclude('bUnit_ea_contacts');
          $c = new bUnit_ea_contacts($av->ID);
          $c->getForm();
          break;
        }
      }
    }
    $b_tabs->close();
  }elseif ($av->isA()){
    //
    // Alumni
    //
    $av->getForm();
  }else{
    //
    // Arbitrary person
    //
    $av = new bForm_Avatar::$bForm_Avatar($av->ID);
    $av->getForm();
  }
}

/*
 * The employee avatar class
 */
locateAndInclude('bForm_Avatar');
class bForm_Avatar_ea extends bForm_Avatar{

  private $eaEmpRecords = Null;
 
  /*
   * For the complete definition of the employee records see bList_eaEmpRecords class,
   * this table gives mapping  bList_eaEmpRecords -> bForm_Avatar_ea
   */
  public static $er2av = Null; // Filled on the fly, see the constructor
  public static $av2er = array('e_rate'         =>array('k'=>'e_rate',    'c'=>'orgEmpTypes'), // The description comes from 'c', i.e. bList_orgEmpTypes
			       'e_gid'          =>array('k'=>'e_gid',     'c'=>'orgGroups'),
			       'rec_type'       =>array('k'=>'rec_type',  'd'=>''),
			       'e_org'          =>array('k'=>'e_org',     'd'=>''),
			       'e_start'        =>array('k'=>'lm_key',    'd'=>''), // start of the employment period
			       'e_end'          =>array('k'=>'lm_status', 'd'=>''), // end   of the employment period
			       'lm_id'          =>array('k'=>'lm_id',     'd'=>''),
			       'av_office'      =>array('k'=>'e_off',     'd'=>'Office'),
			       'av_position'    =>array('k'=>'lm_value',  'd'=>'Position'),
			       'av_institute'   =>array('k'=>'e_inst',    'd'=>'Institute'),
			       // 'av_residentship'=>array('k'=>'e_cc',      'd'=>'Residentship'), // e_cc this is Institute location, not private residentship
			       '_emp_period'     =>array('k'=>CONST_noname,'d'=>'Employment period'),
			       );

  /*
   * Helper to bForm_Avatar::delete
   */
  public static function delete_deps($where){}

  /*
   * Reject the candidate for merging if he is not known to EA
   */
  public static function findDuplicates_reject($r){
    locateAndInclude('bList_eaEmpRecords');
    $reply = True;
    foreach($r as $key=>$value) $where[] = "$key REGEXP '$value'";
    $q = myPear_db()->qquery("SELECT av_id FROM zzz_avatars WHERE ".join(' AND ',$where),cnf_show_mrg);
    while($r = myPear_db()->next_record($q)){
      if (bList_eaEmpRecords::isEA($r['av_id'],myOrg_ID)){
	$reply = False;
	break;
      }
    }
    return $reply;
  }

  /*
   *
   */
  function __construct($id, $mode='RO') {

    parent::__construct($id, $mode);

    // Fill EA -> AV naming conversion
    if (empty(self::$er2av)){
      self::$er2av = array();
      foreach(self::$av2er as $key=>$data){
	self::$er2av[$data['k']] = $key;
      }
    }

    // Like that.
    if (!defined('myOrg_code')) b_debug::traceBack('?? myOrg_code not yet defined');
    elseif (myOrg_code != 'nordita') $this->want_legacy_maildb = False;

    if (cnf_inside_fb){
      b_cnf::_unset('set_access_once');
    }elseif (isset($_GET['set_access_once']) && isset($_GET['av_id'])){
      myOrg()->eaMembers()->setMemberAccess($_GET['av_id'],$_GET['set_access_once']);
    }

    $this->modalRW_enabled = isset($_GET['block_once']);

    self::_init();

  }

  /*
   *
   */  
  static function _init(){
    // Fill the description for orgEmpTypes & orgGroups
    if (myPear_db()->isReady() && empty(self::$av2er['e_rate']['d'])){
      foreach(self::$av2er as $key=>$data){
	if (!empty($data['c'])){
	  self::$av2er[$key]['d'] = myOrg()->org_bList($data['c'])->getValue('l_member_title');
	}
      }
    }
  }

  function __clean_cache(){
    //    myPear::getInstance(get_class($this),$this->ID,'clean');
    parent::__clean_cache();
  }

  function name($fmt='fl') {
    return bForm_Avatar::_fmtName('fl',$this->formDB);
  }

  /*
   * Update the 'av_position' & 'av_institute' & 'av_office' field in the avatar record from the current EA record(s)
   */
  private $dejuVu_update_AVpos_AVinst = 0;
  private function update_AVpos_AVinst(){
    if (!$this->isEA()) return;
    if ($this->dejuVu_update_AVpos_AVinst++) return;
    
    $this->isMBE['av_birthdate']    = !$this->isE();
    $this->isMBE['av_birthcountry'] = !$this->isE();
    
    foreach(array('av_office','av_institute','av_position','e_rate') as $what){

      // Get the current employment from eaEmpRecords
      if ($this->hook_employment()) $currentItem = $this->hook_employment()->get_currentItem(self::$av2er[$what]['k'],
											     array('e_org'=>myOrg_ID),
											     ($what == 'av_position' ? 2 : 1),
											     $this->getValue('av_ddate',True));
      if (empty($currentItem)) continue;
      
      switch($what){
      case 'av_office':
      case 'av_institute':
      case 'av_position':
	$elements = preg_split(";( |/|,);",str_replace('  ',' ',strToLower($currentItem)));
	$currentItem_compressed = b_fmt::ucwords(join(' ',array_unique($elements)));
	
	$this->dbg("$what=$currentItem_compressed");
	if ($this->getValue($what,1) != $currentItem_compressed){
	  $this->getValue_reset_forced($what,$currentItem_compressed);
	  // myPear::MESSAGE_add($this->name()." - set current ".self::$av2er[$what]['d']." '$currentItem_compressed'");
	}
	break;
	
      case 'e_rate':
	$e_rate = myOrg()->orgEmpTypes()->code2text($currentItem);
	$this->dbg($e_rate,True);
	$this->getValue_reset($what,$e_rate);
	// myPear::MESSAGE_add($this->name()." - set current ".self::$av2er[$what]['d']." '$_virt_e_rate'");
	break;
	
      default:
	b_debug::internalError('Unexpected agrument $what="'.var_export($what,True).'"');
      }
    }
  }

  /*
   * Create (if not yet done) the instance of bList_eaEmpRecords 
   */
  function hook_employment($just_hook=False){
    
    locateAndInclude('bList_eaEmpRecords');
    if (!is_object($this->eaEmpRecords) && $this->isEA()){
      if ($this->eaEmpRecords = bList::getListInstance($this,'bList_eaEmpRecords',True)){
	$this->tips        = $this->eaEmpRecords->tips;
	$this->tips_byMode = $this->eaEmpRecords->tips_byMode;
	
	// Update av_office in EArecords
	if ($this->myPost_bForm() && ($av_office = @$_POST['av_office'])){
	  foreach($this->eaEmpRecords->get_currentEmploymentRecords() as $r){
	    $this->eaEmpRecords->updateMember(array('e_off'=>$av_office),$r['lm_id']);
	  }
	}
	
	// Hook the bList_eaEmpRecords updates as if they come for the Avatar 
	if ($this->eaEmpRecords->myPost_bList() && !isset($_POST['_post_bForm_ID_once'])){
	  myPear::WARNING_T("Enforce _POST to be Avatar");
	  $_POST['_post_bForm_ID_once']    = $this->ID;
	  $_POST['_post_bForm_class_once'] = get_class($this);
	}
	
	if (!$just_hook){      
	  // Update avatar av_institute & av_position.
	  // We do it on the fly since "myOrg" might change since the previous invication 
	  $this->update_AVpos_AVinst();    
	  
	  // Optionally adjust statuses
	  if ((@$_GET['av_id'] == $this->ID) || (@$_GET['id'] == $this->ID)) $this->adjust_status();
	}
      }else{
	$this->dbg("No eaEmpRecords yet...",True);
      }
    }
    return $this->eaEmpRecords;
  }

  /*
   * At any given moment the EA-avatar might be either 'e' (EA_role_employee) or 'a' (EA_role_alumnus).
   * The state is defined by analyzing his "employment record" (see class bList_eaEmpRecords) 
   */
  function role(){
    if    ($this->isE())  $reply = EA_role_employee;
    elseif($this->isV())  $reply = EA_role_lt_visitor;
    elseif($this->isEA()) $reply = EA_role_alumnus;
    else                  $reply = Null;
    return $reply;
  }

  /*
   * am I employee OR alumnus?
   */
  function isEA($av_id=Null){
    $reply = $this->is($av_id);
    $this->dbg($reply,cnf_dev);
    return $reply;
  }
  
  /*
   * am I employed?
   */
  function isE($allTypes = False){
    $roles = array(EA_role_employee);
    if ($allTypes) $roles[] = EA_role_lt_visitor;

    $reply = False;
    foreach($roles as $role){
      if ($this->is(Null,$role)){
	$reply = True;	break;
      }
    }
    return $reply;
  }

  /*
   * am I alumnus?
   */ 
  function isA($av_id=Null){
    return $this->is($av_id,EA_role_alumnus,False);
  }

  /*
   * am I Long-Term-Visitor?
   */ 
  function isV($av_id=Null){
    return $this->is($av_id,EA_role_lt_visitor);
  }
  
  /*
   * am i "$role"?
   */
  private function is($av_id=Null,$role=Null,$exact_match=True){
    $reply = False;
    if (myPear::is_int($this->ID)){
      if(empty($av_id))  $av_id  = $this->ID;
      $reply = self::_is($av_id,$role,$exact_match);
    }
    return $reply;
  }

  /*
   * Get the EA member status
   */
  public static function _is($av_id,$role,$exact_match){
    if(empty($role)){
      $reply = myOrg()->eaMembers()->isMember($av_id);
    }else{
      $status= myOrg()->eaMembers()->getUnitMemberStatus($av_id);
      $reply = ($exact_match
		? ($status === $role)
		: (strpos($status,$role) !== False));
    }
    return $reply;
  }

  function askMS(){
    $reply = $this->isEA() || $this->askPhD(); 
    $this->dbg($reply);
    return $reply;
  }
  
  function askPhD(){
    $reply = $this->isEA(); 
    $this->dbg($reply);
    return $reply;
  }

  protected function askSalutation()  { 
    $reply = !$this->isEA(); 
    return $reply;
  }

  protected function ask_av_SSN()  { 
    $isE = $this->isE(True) || $this->getValue('av_ssn',True);
    $reply = $isE || parent::ask_av_SSN();
    $this->defineTypes(array('isMBE' =>array('av_ssn' => !$isE && !($reply && $this->MODE=='RW'))));
    $this->dbg($reply);
    return $reply;
  }

  protected function askBirthPlace()  { 
    return  $this->isEA(); 
  }

  protected function askInstitute()   { 
    return !$this->isE(True); 
  }

  protected function askHomeAddress() { 
    $reply = $this->isE(True); 
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Save the results of the dialog
   * coming from the emp record AND/OR "update personal data"
   *
   * Note, by this moment the "explicit fields" of the Avatar record are already updated, 
   * hence we should update only the EA-related fields 
   */ 
  private function update_EA(){
    
    $lm_id = (empty($_POST['lm_id'])
	      ? @$_POST['_virt_emp_record']
	      :  $_POST['lm_id']);
    if (empty($lm_id)) return;
    if (!$this->hook_employment()) return;
    
    $this->dbg(starting);
    
    $updates = array();
    if ((int)$lm_id) $updates['lm_id'] = $lm_id;
    foreach(self::$av2er as $av=>$er){
      $this->dbg("$av => $er[k]",cnf_dev);
      $new_v = Null;
      if (isset($_POST[$av]))	   $new_v = $this->getValue($av,True,True);
      if (isset($_POST[$er['k']])) $new_v = $this->getValue($er['k'],True,True); 
      if (($new_v===Null) || ($er['k'] != CONST_noname)) continue;
      
      switch($key=$er['k']){
      case 'lm_key':     // e_start
      case 'lm_status':  // e_end
	$new_v = b_time::short_date($new_v);
	break;
	
      case 'e_cc':
	$new_v = b_cc::getCC($new_v);
	break;
	
      default:
      }
      
      $updates[$key] = $new_v;
      foreach(array($er['k'],$av) as $item) $this->getValue_reset($item,@$this->hook_employment()->formDB_member[$er['k']]);
      if (cnf_dev) myPear::MESSAGE(__METHOD__.': updates='.b_fmt::joinX($updates));
      $this->hook_employment()->updateMember($updates,$lm_id);
    }

    $this->adjust_status();
    if (cnf_show_er) $this->hook_employment()->dump();
    $this->dbg(exiting);
  }
  

  /*
   * Check that the EA status (i.e. 'employee', 'visitor', etc.) is not changed by changing 
   * the employment period,  auto-adjust the status then.
   *
   * Read the records in chronological order, hence the latest records have then "bigger weight", i.e. redefine the previous.
   * We read all the records (and not just the latest one) because the person might have 
   * multiple positions at a time.
   *
   * THERE IS SOMETHING WRONG HERE - THE FUNCTION IS HORRIBLY SLOW 
   *
   */
  private $dejaVu_adjust_status = 0;
  function adjust_status($timestamp=Null){
    if ( $this->dejaVu_adjust_status++) return;
    if (!$this->hook_employment()) return;
    if (empty($timestamp)) $timestamp = time();
    
    // Sanity, convert "visiting major position" to "visiting activity", 
    // set the start/retire date, etc.
    $this->hook_employment()->sortArgs = ' ORDER BY '.self::$av2er['e_end']['k'].' ASC ';
    foreach($this->hook_employment()->getMembers(array('e_org' => myOrg_ID)) as $r_id=>$r){
      if (!($rec_type = @$r['rec_type'])){
	myPear::WARNING("Missing 'Employment type'");
	continue;
      }
      // Set record type "Visiting appointment" if "major position" matches 'visit' 
      if (stripos($r['lm_value'],'visit') !== False){
	$e = '40_va';
	if ($rec_type != $e) myPear::WARNING_T(sprintf("Change rec_type '%s' -> '%s' for position '%s'",$rec_type,$e,$r['lm_value'])); 
	$rec_type = $e;
      }elseif($rec_type == '40_va'){
	$e = '20_mp';
	myPear::WARNING_T(sprintf("Change rec_type '$rec_type' -> '$e' for position '%s'",$r['lm_value'])); 
	// ?? is it correct? Does the visiting position always contain word "visit"?
	$rec_type = $e;
      }
      
      // Change the record type if is was wrongly entered
      if ($rec_type != $r['rec_type']) $this->hook_employment()->updateMember(array('rec_type'=>$rec_type),$r['lm_id']);
      
      // Set the retirement date as the latest date of any affiliation with the organization
      if ($e_start = @$r['lm_key']){
	$e_start = b_time::txt2unix($e_start);
	if (!isset($day1)) $day1 = $e_start;
	$day1 = min($day1 , $e_start);
      }
      if ($e_end = @$r['lm_status']){
	$e_end = b_time::txt2unix($e_end);
	if (!isset($day9)) $day9 = $e_end;
	$day9 = max($day9 , $e_end);
      }
    }

    if (isset($day1) && isset($day9)){
      // $this->dbg(b_time::period($day1,$day9),cnf_dev);
      myOrg()->eaMembers()->updateMember(array('day1'=>$day1,'day9'=>$day9),array('um_avid'=>$this->ID));
    }
    
    // Walk thru the EA records IN CHRONOLICAL ORDER and adjust the status according to the start/end date of the position. 
    // The most fresh record comes last
    $stat = array();
    myOrg()->eaMembers()->setStatus($this->ID,'');
    $this->hook_employment()->sortArgs = ' ORDER BY '.self::$av2er['e_start']['k'].' ASC ';
    foreach($this->hook_employment()->getMembers(array('e_org' => myOrg_ID)) as $r_id=>$r){
      if (empty($r['rec_type'])) continue;
      $position_is_expired = (b_time::txt2unix($r[self::$av2er['e_end']['k']]) < $timestamp);
      $id = ($position_is_expired ? 'was' : 'now');
      
      switch($r['rec_type']){
      case '20_mp': $stat[$id] = EA_role_employee;   break;
      case '40_va': $stat[$id] = EA_role_lt_visitor; break;
      default: 
      }
    }
    
    // Save the latest status as the EA member status 
    if(empty($stat)) $stat['was'] = '';
    $status = (empty($stat['now'])
	       ? $stat['was'].EA_role_alumnus
	       : $stat['now']);
    $status_was = myOrg()->eaMembers()->getUnitMemberStatus($this->ID);
    if (!empty($status) || !empty($status_was)){
      if ($status_was !== $status) $this->dbg(sprintf("status: '%s' --> '%s'",
						      $status_was,
						      $status),
					      cnf_dev);
      myOrg()->eaMembers()->setStatus($this->ID,$status,$verbose=cnf_dev);
    }
  }
  
  /*
   * Validate fields specific for the EA_Avatar
   */
  protected function getValue_validate_empty($k,&$v){
    $err_msg = '';
    if (myPear::is_empty($v)){
      $err_msg = parent::getValue_validate_empty($k,$v);
      switch($k){
      case 'av_office':
      case 'e_off':
      case 'e_end':
      case 'e_gid':
      case 'e_rate':
	if (!$this->isE()) $err_msg = '';
	break;

      default:
      }
    }
    return $err_msg;
  }

  /*
   *
   */
  function formOKspecific() {
    parent::formOKspecific();
    if (isset($_POST['_virt_emp_record']) &&
	($av2er = array_intersect(array_keys($_POST),array_keys(self::$av2er)))){
      if ($this->hook_employment()){
	$updates = array('lm_id' => $_POST['_virt_emp_record']);
	foreach($av2er as $k) $updates[$k] = $_POST[$k];
	$this->hook_employment()->updateMember($updates);
      }
    }
  }

  /*
   * The method assumes that the Avatar is employed by the "current organization"
   */
  private $getForm_dejaVu = 0;
  function getForm(){
    
    if ($this->getForm_dejaVu++){
      var_dump("??? dejaVu ".__method__);
      return;
    }

    // Complete the definition of variables used during the dialog
    $this->defineVariables_fine_tuning();

    // SSN has precedence over the av_birthdate
    $this->isRO['av_birthdate'] = SSN::ssn2birthDate($this->getValue('av_ssn',True),$this);
    
    // Do not show the EA page if we are not in the EA context
    if ((b_reg::$current_module != EA_MODULE) || !$this->isEA()){

      parent::getForm();

    }else{

      // update EA records (however, I am not sure that this is needed....)
      //      $this->update_EA();

      //
      // Show the employment records instead of Avatar form, if the referrer is bList_eaEmpRecords and the form is complete
      //
      // $dbg=sprintf(__METHOD__.'(): from_bList=%s',var_export(b_cnf::get('from_bList'),True)); static $dbg_was='?'; if ($dbg != $dbg_was) var_dump($dbg); $dbg_was=$dbg;

      // We are called from "update EA dialog" to update Phd/MS/Birth blocks.
      // Update the Avatar record and then show the EA records (as the user expects)
      if (($block_once = @$_GET['block_once']) || b_cnf::get('from_bList')){
	$this->formOK();
	if ($this->MODE == 'RO'){
	  if ($this->hook_employment()) $this->hook_employment()->getForm();
	  return;
	}
      }

      // An "inversed" case. We are called from the "Avatar dialog", but will query an EA record. 
      // The results will go both to the EA & Avatar records
      if (($block_once === 'block_employment') && ($this->MODE == 'RW') && $this->hook_employment()){
	$records = $this->hook_employment()->get_currentEmploymentRecords(($this->isE(True) 
									   ? array('e_org'=>myOrg_ID)
									   : array()),
									  $this->getValue('av_ddate',True));
	$r_exists = (bool)($r = @$records[0]);
	foreach(array('from_bForm' => 1,
		      'from_bList' => 0,
		      'act_parent' => $this->ID,
		      'act_code'   => ($r_exists ? 'edit'     : 'add'),
		      'act_id'     => ($r_exists ? $r['l_id'] : $this->eaEmpRecords->ID),
		      'act_lmid'   => ($r_exists ? $r['lm_id']: 'new'), 
		      'lm_id'      => ($r_exists ? $r['lm_id']: 'new'), 
		      ) as $k=>$v) $_POST[$k] = $_GET[$k] = $_REQUEST[$k] = $v;
	$this->dbg();
	$bList_eaEmpRecords = bList::getListInstance($this,'bList_eaEmpRecords');
	$bList_eaEmpRecords->getForm();
	return;
      }

      /*
Seem to be useless....
	if (!empty($_REQUEST['action_once']) && preg_match('/'.myOrg_name.' employment/',$_REQUEST['action_once']) && ($ea = $this->hook_employment())){
	  $records = $ea->get_currentEmploymentRecords(array('e_org'=>myOrg_ID),$this->getValue('av_ddate',True));
	  foreach($records as $r){
	    $lm_id = $r['lm_id'];
	    break; // take just the first record if there are many
	  }
	  if (!empty($lm_id)) b_url::header(b_url::same(array('act_once'  =>1,
							      'act_id'    =>$ea->ID,
							      'act_code'  =>'edit',
							      'act_parent'=> $this->ID,
							      'arg1_once' => $this->ID,
							      'act_lmid'  => $lm_id,
							      'lm_id'     => $lm_id,
							      'ed_bList'  =>'bList_eaEmpRecords',
							      'resetcache_once'=>1,
							      )));
	}
      */
	
      //
      // Show the Avatar dialog
      //
      // Header
      $this->getFormHeader();
      $this->getFormOpen();
      
      // Information blocks
      $this->avInfoBlock();
      $this->studyBlock();
      $this->officeBlock();
      $this->residentshipBlock();
      
      // Hook the "legacy software" block if available
      if ($this->connect_legacy_maildb()){
	$lm = new legacy_maildb($this);
	$lm->getForm('legacy_maildb');
      }
      
      if ($this->MODE == 'RO'){
	$this->closeForm('');
	if ($this->isWritable()){
	  locateAndInclude('b_tabs');
	  if (!cnf_inside_fb && ! b_tabs::$inside) 
	    print b_btn::UL_buttons(array(b_btn::submit('update employment records',
							b_url::same("?from_bForm=0&function=ea_updateER&av_id=".$this->ID))));
	}
      }else{
	$this->closeForm();
      }
    }
  }

  /*
   * Interface with the legacy software.
   * The legacy maildb is a very robust product, but provides only a limited set of functions.
   * So we keep it as a useful RO interface for the users
   */
  public $want_legacy_maildb = True;
  private function connect_legacy_maildb(){

    if (!$this->want_legacy_maildb) return False;

    static $reply = Null;
    if ($reply === Null){
      myPear_access(EA_MODULE);
      
      $reply = (function_exists('module_exists') && defined('LEGACY_MODULE') && module_exists(LEGACY_MODULE) && 
		locateAndInclude('legacy_maildb'));
      if ($reply){
	$reply = (bool)(EA_ORG_manager_here && ($this->isE(True) || $this->isV(True)) && !$this->getValue('av_ddate',True,1));
      }else{
	myPear::WARNING("LEGACY_MODULE is not active, can't communicate with 'maildb'"); 
      }
    }
    return $reply;
  }

  /*
   *
   */  
  protected function formBlock_isVisible($block_ID,$fields=array()) {

    // Workaround... To be understood...
    // There is a mismatch between answers from $this->isV() and group membership.
    // See aaz, for ex.

    if ($block_ID === @$_GET['block_once']) return True;
     
    if ($reply = parent::formBlock_isVisible($block_ID,$fields)){
      
      switch($block_ID){
      case 'legacy_maildb':
	$reply = $this->connect_legacy_maildb();
	break;
	
      case 'av_birth_block':
	//	$reply = !(int)$this->ID || $this->isEA();
	break;
	
      case 'block_employment':
      case 'block_office':
      case 'block_researchGroup':
	$reply = ($this->isE(True) || $this->isV());
	break;
	
      case 'recent_emp':
      $reply = $this->isA() || $this->isV();
      break;
      }
    }
    return $reply;
  }

  /*
   * Get the user office name
   */
  function get_staffOffice(){
    $office = Null;
    if ($ea = $this->hook_employment()){
      $records = $ea->get_currentEmploymentRecords(array('e_org'=>myOrg_ID),
						   $this->getValue('av_ddate',True));
      foreach($records as $k=>$r){ 
	// Check the campus room numbering/naming convention
	if (preg_match('/'.VM_office_regexp.'/',@$r['e_off'],$m)){
	  $office = $m[1];
	  break;
	}
      }
    }
    $this->dbg($this->fmtName('fl')." - office = '$office'");
    return $office;
  }

  /*
   * if $this->isE() ask all the employment details (office, group, phone, etc.)
   * if $this->isA() ask what is/was the recent employment
   */
  private function officeBlock(){

    //
    // Get the current employment from the database
    // and populate Avatar class
    //
    if (!$this->hook_employment()) return;
    $records = $this->hook_employment()->get_currentEmploymentRecords(($this->isE(True) 
								       ? array('e_org'=>myOrg_ID)
								       : array()),
								      $this->getValue('av_ddate',True));
    if (($n = count($records)) > 1){
      myPear::WARNING_T("There are $n \"current employment\" records for ".
			$this->name().". The result might be ambiguous...");
    }
    
    $this->formDB_modified['_virt_emp_record'] = 'new';
    foreach($records as $k=>$r){
      $this->formDB_modified['_virt_emp_record'] = $r['lm_id'];
      foreach(self::$av2er as $av=>$er){
	$this->formDB_modified[$av] = @$r[$er['k']];
      }
      break;
    }
    
    foreach(self::$av2er as $av=>$er){
      $this->defineVariable(array($av=>$er['d']),True);
    }
    
    //
    // Employees
    //
    if ($this->isE(True)){
      $icon = myOrg()->getValue('org_code');
      
      //
      // Contract dates
      //
      $e_end = min(CONST_eternity,$this->getValue('e_end',1,1));
      $this->formDB_modified['_emp_period'] = b_time::short_date($this->getValue('e_start')). ' -> ' .b_time::short_date($e_end);

      $dialog = (($types=myOrg()->orgEmpTypes()->get_empTypes())
		 ? array('_virt_emp_record'=> array('textHidden'),
			 '_emp_period'     => array('datePickerField2','e_start','e_end'),
			 'e_rate'          => array('selectMenu',$types),
			 'av_position'     => array('textField'))
		 : array('_virt_emp_record'=> array('textHidden'),
			 '_emp_period'     => array('datePickerField2','e_start','e_end'),
			 'av_position'     => array('textField')));

      $this->submitOnChange['e_rate'] = True;
      $this->formBlock('block_employment',
		       myOrg_name.' employment',
		       $dialog,
		       array('i'=>$icon,
			     'l'=>$this->sameURL_but('RW')));
      
      //
      // Group
      //    
      if ($groups=myOrg()->orgGroups()->get_groups()){
	$this->formBlock('block_researchGroup',
			 'Research group',
		       array('_virt_emp_record' => array('textHidden'),
			     'e_gid'            => array('checkBoxes',$groups),
			     ),
		       array('i'=>$icon,
			     'l'=>$this->sameURL_but('RW')));
      }
      
      //
      // Office
      //
      $dialog = array('_virt_emp_record' => array('textHidden'),
		      'av_phone'   => array('textField'));
      if ($groups) $dialog['av_office'] = array('textField');
      $this->formBlock('block_office',
		       'Office location',
		       $dialog,
		       array('i'=>$icon,
			     'l'=>$this->sameURL_but('RW')));
      
      
      //
      // Non-employees
      //
    }else{
      $this->defineVariable('Country','av_residentship');
      if (!$this->getValue('rec_type')) $this->formDB_modified['rec_type'] = '20_mp';
      
      if (!$this->getValue('av_ddate',True,True)){
	$dialog = array('av_position'     => array('textField'),
			'av_institute'    => array('textField'),
			'av_city'         => array('textField'),
			'av_residentship' => array('textFieldCountry'),
			'_emp_period'     => array('datePickerField2','e_start','e_end'),
			'_virt_emp_record'=> array('textHidden'),
			'rec_type'        => array('textHidden'),
			);
	// All the fields are optional
	foreach($dialog as $k=>$v)       $this->isMBE[$k] = True;
	
	$this->formBlock('recent_emp',
			 'Recent employment',
			 $dialog,
			 array('i'=>'i-building',
			       'l'=>$this->sameURL_but('RW')));
      }
    }
  }
  
  /*
   * 
   */
  function isManager(){
    myPear_access(EA_MODULE);
    $reply = EA_administrator_here;
    return $reply;
  }
}

