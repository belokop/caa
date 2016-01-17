<?php
/*
 * bForm_Avatar_ea - Employee's & Alumni Avatar class.       YB 2011-05-01
 *
 * At any given moment the EA-avatar might be either 'E' (EA_role_employee) or 'A' (EA_role_alumnus).
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
 * See & Edit the avatar.
 * For the employeee & long term visitor ask for extra information (CV, Contact persons)
 */
function ea_myPage($av_id=Null){

  // Sanity
  b_cnf::_unset(array('ed_bList','edi_m_once','action_once'));

  if (empty($av_id)) $av_id = $_GET['av_id'];
  if (is_object($av_id)) $av = (($av_id instanceof bForm_Avatar_ea)
				? $av_id
				: new bForm_Avatar_ea($av_id->ID));
  elseif(!empty($av_id)) $av = new bForm_Avatar_ea($av_id);
  
  if (!is_object($av)){
    if (cnf_dev) b_debug::internalError("empty 'av_id'");
    return;
  }
  if ($av->isE() || $av->isV()){
    $tabs = array('Personal data','CV','Employment');
    if (is_object(bAuth::$av)){
      if (!method_exists(bAuth::$av,'isE')) bForm_Avatar::set_context(bAuth::$av);
      if (bAuth::$av->isE() || bAuth::$av->isV()) $tabs[] = 'Contacts';
    }
    locateAndInclude('b_tabs');
    $b_tabs = new b_tabs_myPear($tabs);
    foreach($tabs as $k=>$tab){
      if ($b_tabs->body()){
	b_cnf::_unset('see_myinfo');
        
        switch($k){
        case 0:
          $_GET['see_myinfo'] = 'yes';
          $av->getForm();
          break;
          
        case 1:
          myOrg()->eaCV(myOrg_ID,$av)->show();
          break;
          
        case 2:
          $emp = $av->hook_employment();
          $emp->getForm();
          break;
          
        case 3:
          locateAndInclude('bUnit_ea_contacts');
          $c = new bUnit_ea_contacts($av->ID);
          $c->getForm();
          break;
        }
      }
    }
    $b_tabs->close();
  }elseif ($av->isA()){
    $_GET['see_myinfo'] = 'yes';
    $av->getForm();
  }else{
    $_GET['see_myinfo'] = 'yes';
    $av = new bForm_Avatar::$bForm_Avatar($av->ID);
    $av->getForm();
  }
}

/*
 * The employee avatar class
 */
locateAndInclude('bForm_Avatar');
class bForm_Avatar_ea extends bForm_Avatar{

  private $eaRecords = Null;
 
  /*
   * For the complete definition of the employee records see bList_eaEmpRecords class,
   * this table gives mapping  bList_eaEmpRecords -> bForm_Avatar_ea
   */
  private static $av2er = array('emp_rate'       =>array('k'=>'e_rate',    'c'=>'orgTrades'), // The description comes from 'c', i.e. bList_orgTrades
				'e_gid'          =>array('k'=>'e_gid',     'c'=>'orgGroups'),
				'av_institute'   =>array('k'=>'e_inst',    'd'=>'Institute'),
				'av_residentship'=>array('k'=>'e_cc',      'd'=>'Residentship'),
				'av_position'    =>array('k'=>'lm_value',  'd'=>'Position'),
				'e_off'          =>array('k'=>'e_off',     'd'=>'Office'),
				'e_type'         =>array('k'=>'e_type',    'd'=>''),
				'e_org'          =>array('k'=>'e_org',     'd'=>''),
				'e_start'        =>array('k'=>'lm_key',    'd'=>''), // start of the employment period
				'e_end'          =>array('k'=>'lm_status', 'd'=>''), // end of the employment period
				'lm_id'          =>array('k'=>'lm_id',     'd'=>''),
				'emp_period'     =>array('k'=>CONST_noname,'d'=>'Employment period'),
				);

  /*
   * Helper to bForm_Avatar::delete
   */
  public static function delete_deps($where){
  }

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

    // Like that.
    if (!defined('myOrg_code')) b_debug::traceBack('?? myOrg_code not yet defined');
    elseif (myOrg_code != 'nordita') $this->want_legacy_maildb = False;

    parent::__construct($id, $mode);

    if (cnf_inside_fb){
      b_cnf::_unset('see_myinfo');
      b_cnf::_unset('set_access_once');
    }elseif (isset($_GET['set_access_once']) && isset($_GET['av_id'])){
      myOrg()->eaMembers()->setMemberAccess($_GET['av_id'],$_GET['set_access_once']);
    }

    // Optionally adjust statuses
    $this->modalRW_enabled = isset($_GET['block_once']);
    if ((@$_GET['av_id'] == $this->ID) || (@$_GET['id'] == $this->ID)) $this->adjust_status();

    // Fill the description for orgTrades & orgGroups
    if (myPear_db()->isReady() && empty(self::$av2er['emp_rate']['d'])){
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
   * Update the 'av_position' & 'av_institute' field in the avatar record from the current EA record(s)
   */
  private function update_AVpos_AVinst($what){
    if ($this->isEA() && !empty(self::$av2er[$what])){
      $this->hook_employment();
      $this->isMBE['av_birthdate']    = !$this->isE();
      $this->isMBE['av_birthcountry'] = !$this->isE();
      $ea_value_a = array();
      foreach(($i=explode(',',$this->eaRecords->get_currentItem(self::$av2er[$what]['k'],array(),2,$this->getValue('av_ddate',True)))) as $value){
	// "Slash separated" positions in chronologival order, get the latest one only
	$v_a = explode('/',$value);
	$v   = trim(array_pop($v_a));
	if (empty($ea_value_a))                                                                    $ea_value_a[] = $v;
	elseif (!preg_grep("/$v/i",$ea_value_a) && !preg_match("/".join('|',$ea_value_a)."/i",$v)) $ea_value_a[] = $v;
      }
      // Compact "current position"
      if (($what == 'av_position') && !empty($ea_value_a)){
	foreach($ea_value_a as $pp) $positions[] = explode(' ',str_replace('  ',' ',b_fmt::ucwords($pp)));
	switch(count($positions)){
	case 1:
	  $p = $ea_value_a;	  break;
	case 2:
	  $p = array_intersect($positions[0],$positions[1]);	  break;
	case 3:
	  $p = array_intersect($positions[0],$positions[1],$positions[2]);	  break;
	case 4:
	default:
	  $p = array_intersect($positions[0],$positions[1],$positions[2],$positions[3]);	  break;
	}
	if (!empty($p)) $ea_value_a = array(join(' ',$p));
      }
      $ea_value = join(',',$ea_value_a);
      if (!empty($ea_value) && (mb_strToLower($ea_value) != mb_strToLower($this->getValue($what,1)))){
	$this->updateDB(array($what=>$ea_value));
	myPear::MESSAGE_add($this->name()." - set current ".self::$av2er[$what]['d']." '$ea_value'");
      }
    }
  }

  /*
   * Create (if not yet done) the instance of bList_eaEmpRecords 
   */
  function hook_employment(){
    locateAndInclude('bList_eaEmpRecords');
    if (!is_object($this->eaRecords) && $this->isEA()){
      $this->eaRecords = new bList_eaEmpRecords($this);
      $this->tips        = $this->eaRecords->tips;
      $this->tips_byMode = $this->eaRecords->tips_byMode;
      
      // Hook the bList_eaEmpRecords updates as if they come for the Avatar 
      if ($this->eaRecords->myPost_bList() && !isset($_POST['_post_bForm_ID_once'])){
	myPear::WARNING_T("Enforce _POST to be Avatar");
	$_POST['_post_bForm_ID_once']    = $this->ID;
	$_POST['_post_bForm_class_once'] = get_class($this);
      }
    }
    return $this->eaRecords;
  }

  /*
   * At any given moment the EA-avatar might be either 'E' (EA_role_employee) or 'A' (EA_role_alumnus).
   * The state is defined by analyzing the avatar "employment record" (see class bList_eaEmpRecords) for the organization.
   */
  function role(){
    if    ($this->isE())  $reply = EA_role_employee;
    elseif($this->isV())  $reply = EA_role_lt_visitor;
    elseif($this->isEA()) $reply = EA_role_alumnus;
    return $reply;
  }

  /*
   * am I employee OR alumnus?
   */
  function isEA($av_id=Null){
    return $this->is($av_id);
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
    if ((int)$this->ID){
      if(empty($av_id))  $av_id  = $this->ID;
      $reply = self::_is($av_id,$role,$exact_match);
    }
    return $reply;
  }

  /*
   * Get the EA member status
   */
  public static function _is($av_id,$role=Null,$exact_match=True){
    if(empty($role)){
      $reply =  myOrg()->eaMembers()->isMember($av_id);
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
    if (1) $reply = !$this->isEA(); 
    else   $reply = True;
    return $reply;
  }

  protected function ask_av_SSN()  { 
    $isE = $this->isE() || $this->getValue('av_ssn',True);
    $reply = $isE || parent::ask_av_SSN();
    $this->defineTypes(array('isMBE' =>array('av_ssn' => !$isE && !($reply && $this->MODE=='RW'))));
    $this->dbg($reply);
    return $reply;
  }

  protected function askBirthPlace()  { 
    return  $this->isEA(); 
  }

  protected function askInstitute()   { 
    return !$this->isEA(); 
  }

  protected function askHomeAddress() { 
    $reply = $this->isE(); 
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Save the results of the dialog
   * coming from the emp record AND/OR "update personal data"
   *
   * Note, by this moment the "explicit fields" of the Avatar record is already updated, 
   * hence we should update only the EA-related fields 
   */ 
  private function update_EA(){

    $this->hook_employment();
    // editing/creating the bList_eaEmpRecords record from the bForm buffer
    $lm_id = @$_POST['lm_id'];
    if ( empty($lm_id)) $lm_id = @$_POST['_virt_emp_record'];
    if (!empty($lm_id)){
      $updates = array();
      if ((int)$lm_id) $updates['lm_id'] = $lm_id;
      foreach(self::$av2er as $av=>$er){
	$new_v = Null;
	if (isset($_POST[$av]))	     $new_v = $this->getValue($av,True,True);
	if (isset($_POST[$er['k']])) $new_v = $this->getValue($er['k'],True,True); 
	
	if (!is_null($new_v) && $er['k'] != CONST_noname){
	  switch($key=$er['k']){
	  case 'lm_key':
	  case 'lm_status':
	    $new_v = b_time::short_date($new_v);
	    break;
	    
	  case 'e_cc':
	    $new_v = b_cc::getCC($new_v);
	    break;
	    
	  default:
	  }
	  $updates[$key] = $new_v;
	}
	$this->formDB_modified[$er['k']] = $this->formDB_modified[$av] = @$this->eaRecords->formDB_member[$er['k']];
      }
      $this->eaRecords->updateMember($updates,$lm_id);
    }

    /*
    //????? This piece of code is kept for reference,
    //????? the work should be already done. i think.
    //
    // study group in the Avatar record
    $updates = array();
    foreach(array('ms','phd') as $c){
      foreach(array('year','country','institute') as $k){
	$f = "av_${c}_${k}";
	if (isset($_POST[$f])) if ($v = $this->getValue($f,True)) $updates[$f] = trim($v);
      }
    }
    if (!empty($updates)) $this->updateDB($updates);
    */
    
    $this->adjust_status();
    if (cnf_show_er) $this->eaRecords->dump();
  }
  

  /*
   * Check that the EA status (i.e. 'emploee', 'visitor', etc.) is not changed by changing 
   * the employment period,  auto-adjust the status then.
   *
   * Read the records in chronological order, hence the latest records have then "bigger weight", i.e. redefine the previous.
   * We read all the records (and not just the latest one) because the person might have 
   * multiple positions at a time.
   *
   * THERE IS SOMETHING WRONG HERE - THE FUNCTION IS HORRIBLY SLOW 
   *
   */
  private function adjust_status($timestamp=Null){
    if (!$this->hook_employment()) return;
    if (empty($timestamp)) $timestamp = time();

    // Sanity, convert "visiting major position" to "visiting activity", 
    // set the start/retire date, etc.
    $this->eaRecords->sortArgs = ' ORDER BY '.self::$av2er['e_end']['k'].' ASC ';
    foreach($this->eaRecords->getMembers(array('e_org' => myOrg_ID)) as $r_id=>$r){
      if (!($e_type = @$r['e_type'])){
	myPear::WARNING("Missing 'Employment type'");
	continue;
      }
      // Set record type "Visiting appointment" for any visit marked as "major position"
      if (stripos($r['lm_value'],'visit') !== False){
	myPear::WARNING_T(sprintf("Change e_type '%s' -> '40_mp' for position '%s'",$e_type,$r['lm_value'])); 
	$e_type = '40_va';
      }elseif($e_type == '40_va'){
	// ?? is it correct? Does the visiting position always contain word "visit"?
	myPear::WARNING_T(sprintf("Change e_type '40_va' -> '20_mp' for position '%s'",$r['lm_value'])); 
	$e_type = '20_mp';
      }

      // Change the record type if is was wrongly entered
      if ($e_type != $r['e_type']) $this->eaRecords->updateMember(array('e_type'=>$e_type),$r['lm_id']);

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
    
    // Walk thru the EA records and adjust the status according to the start/end date of the position. 
    // The most fresh record comes last
    $this->eaRecords->sortArgs = ' ORDER BY '.self::$av2er['e_end']['k'].' ASC ';
    foreach($this->eaRecords->getMembers(array('e_org' => myOrg_ID)) as $r_id=>$r){
      $stats = preg_split('//',myOrg()->eaMembers()->getUnitMemberStatus($this->ID), -1,PREG_SPLIT_NO_EMPTY);
      $position_is_expired = (b_time::txt2unix($r[self::$av2er['e_end']['k']]) < $timestamp);

      // Special field for the board members,
      // let's keep those people happy...
      if (stripos($r['lm_value'],'Board') !== False){
	$role2 = ($position_is_expired
		  ? EA_role_boardMember.EA_role_alumnus 
		  : EA_role_boardMember);
	myOrg()->eaMembers()->updateMember(array('role2'=>$role2),array('um_avid'=>$this->ID));
	$role2 = preg_split('//',$role2,-1,PREG_SPLIT_NO_EMPTY);
      }else{
	$role2 = array();
      }

      // Now check the status field and update it if needed
      $status = array();
      switch($type=(string)@$r['e_type']){
      case '20_mp':
	// "Major position",   has precedence over all the other emp. types
	$status[] = EA_role_employee;
	if ($position_is_expired) $status[] = EA_role_alumnus;
	break;
	
      case '40_va':
	// "Long term Visitor",
	//  has precedence only over "affiliated"
	if (array_intersect($stats,array(EA_role_employee))) $status[] = EA_role_employee;
	else                                                 $status[] = EA_role_lt_visitor;
	if ($position_is_expired)                            $status[] = EA_role_alumnus;
	break;
	
      case '22_op':
	// "Other activity", 
	//  set EA status to "affiliated" if the person is neither visitor, employee nor alumnus
	if ($rr=array_intersect($stats,array(EA_role_employee,
					     EA_role_lt_visitor))) $status[] = $rr[0];
	else                                                       $status[] = EA_role_affiliate;
	if ($position_is_expired)                                  $status[] = EA_role_alumnus;
	break;
	
      case '':
	if (cnf_dev) b_debug::traceBack('empty e_type');
	
      default:
	$status[] = EA_role_affiliate;
	if ($position_is_expired) $status[] = EA_role_alumnus;
      }
      // $ddd = array(); foreach(array('e_type','lm_key','lm_value','lm_status') as $k) $ddd[$k] = $r[$k];      b_debug::print_r($ddd,join('',$stats).'-'.join('',$role2)." = $status");      
    }
    // Get the latest status and save it as the EA member status 
    if (!empty($status)){
      $this->dbg($status);
      myOrg()->eaMembers()->setStatus($this->ID,join('',$status),$verbose=cnf_dev);
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
      case 'e_end':
      case 'e_off':
      case 'e_gid':
      case 'e_rate':
      case 'emp_rate':
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
	$this->eaRecords->updateMember($updates);
      }
    }
  }

  /*
   * The method assumes that the Avatar is employed by the "current organization"
   */
  function getForm(){

    // Complete the definition of variables used during the dialog
    $this->defineVariables_fine_tuning();

    // SSN has precedence over the av_birthdate
    $this->isRO['av_birthdate'] = SSN::ssn2birthDate($this->getValue('av_ssn',True),$this);
    
    // Sanity
    if ((b_reg::$current_module != EA_MODULE) || !$this->isEA()){

      parent::getForm();

    }else{

      // Switch to the bForm dialog if a block is requested
      if (!empty($_GET['block_once'])) b_cnf::set('see_myinfo','yes');
      
      $this->update_AVpos_AVinst('av_institute');
      $this->update_AVpos_AVinst('av_position');

      // apply updates
      $this->update_EA();

      // and go to the dialog
      switch($this->MODE){
      case 'RO': 
        //
        // show the employment records
        //
        if (!b_cnf::get('see_myinfo')){
          $this->eaRecords->getForm();
	  return;
        }

      case 'RW':
        //
        // show the Avatar dialog
        //
	// Header
	$this->getFormHeader();
	$this->getFormOpen();
	// Information blocks
	$this->avInfoBlock();
	$this->studyBlock();
	$this->officeBlock();
	$this->addressBlock();
	
	// Hook the "legacy software" block if available
	if ($this->connect_legacy_maildb()){
	  $lm = new legacy_maildb($this);
	  $lm->getForm('legacy_maildb');
	}
	
        if ($this->MODE == 'RO'){
	  $this->closeForm('');
	  locateAndInclude('b_tabs');
	  if (!cnf_inside_fb && ! b_tabs::$inside) 
	    print b_btn::UL_buttons(array(b_btn::submit('update employment records',
							b_url::same("?see_myinfo=0&mode=RO&form=Avatar_ea&av_id=$this->ID&id=$this->ID"))));
        }else{
	  $this->closeForm();
	}
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
	$reply = (bool)(EA_manager_here && ($this->isE(True) || $this->isV(True)) && !$this->getValue('av_ddate',True,1));
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
	
      case 'office_contract':
      case 'office_off':
      case 'office_rg':
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
   * if $this->isE() ask all the employment details (office, group, phone, etc.)
   * if $this->isA() ask what is/was the recent employment
   */
  private function officeBlock(){

    //
    // Get the current employment from the database
    // and populate Avatar class
    //
    $this->hook_employment();
    $records = $this->eaRecords->get_currentEmploementRecords(($this->isE(True) 
							       ? array('e_org'=>myOrg_ID)
							       : array()),
							      999,
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
    }
    
    foreach(self::$av2er as $av=>$er){
      $this->defineVariable(array($av=>$er['d']),True);
    }

    //
    // Employees
    //
    $icon = myOrg()->getValue('org_code');

    //
    // Contract dates
    //
    $e_end = min(CONST_eternity,$this->getValue('e_end',1,1));
    $this->formDB_modified['emp_period'] = b_time::short_date($this->getValue('e_start')). ' -> ' .b_time::short_date($e_end);
    
    $dialog = array('_virt_emp_record'=> array('textHidden'),
		    'emp_period'      => array('datePickerField2','e_start','e_end'),
		    'av_position'     => array('textField'));
    if ($trades=myOrg()->orgTrades()->get_trades()) $dialog['emp_rate'] = array('selectMenu',$trades);
    $this->formBlock('office_contract',
		     myOrg_name.' employment',
		     $dialog,
		     array('i'=>$icon,
			   'l'=>$this->sameURL_but('RW')));

    //
    // Group
    //    
    if ($groups=myOrg()->orgGroups()->get_groups()){
      $this->formBlock('office_rg',
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
    if ($groups) $dialog['e_off'] = array('textField');
    $this->formBlock('office_off',
		     'Office location',
		     $dialog,
		     array('i'=>$icon,
			   'l'=>$this->sameURL_but('RW')));
    
    
      //
      // Non-employees
      //
    $this->defineVariable('Country','av_residentship');
    if (!$this->getValue('e_type')) $this->formDB_modified['e_type'] = '20_mp';
    
    $dialog = array('av_position'     => array('textField'),
		    'av_institute'    => array('textField'),
		    'av_city'         => array('textField'),
		    'av_residentship' => array('textFieldCountry'),
		    'emp_period'      => array('datePickerField2','e_start','e_end'),
		    '_virt_emp_record'=> array('textHidden'),
		    'e_type'          => array('textHidden'),
		    );
    // All the fields are optional
    foreach($dialog as $k=>$v)       $this->isMBE[$k] = True;

    $this->formBlock('recent_emp',
		     'Recent employment',
		     $dialog,
		     array('i'=>'i-building',
			   'l'=>$this->sameURL_but('RW')));
  }
  
  /*
   * 
   */
  function isManager(){
    myPear_access(EA_MODULE);
    $reply = EA_admin_here;
    return $reply;
  }
}

