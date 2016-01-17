<?php
/* Position class 
 *
 *
 * Classes which have bForm_jam_Position as the "parent", i.e. the classes referring to it:
 *   - bForm_jam_Applicant applicants applying for the position 
 *   - bForm_jam_RF        Research fields associated with the positions
 *   - bUnit_jam           Search Committee for the position
 *
 */
locateAndInclude('bForm_jam');
class bForm_jam_Position extends bForm_jam {
  
  var $TABLE  = 'jam_positions';
  var $TABLEID= 'pos_id';
  var $debug  = False;
  var $sc     = NULL;    /* link to SC */
  private $_rf= array(); /*links to array of forRF */

  var $tips_byMode = array('RW'=>array('pos_bcc'         => 'Gets copies of all the Emails sent by JAM',
				       'pos_logo'        => 'Select partners',
				       'pos_type'        => 'Positions which are "outside" JAM only provide a link to the announcement', 
				       'pos_namesc1'     => 'The members have right to evaluate the applications and to participate in the final selection',
				       'pos_namesc2'     => 'The members have right to evaluate the applications',
				       'pos_email'       => 'This address is shown as "From:" in the mails sent to applicants and/or referees',
				       'pos_av_noreply'  => 'If "no" the sender address for referees is "noreply"',
				       'pos_ap_noreply'  => 'If "no" the sender address for applicants is "noreply"',
				       'pos_n_referees'  => 'Usually 3 referees is enough, but there are exceptions...',
				       'pos_deadline'    => 'Last day when the applications are still accepted (local time)',
				       'pos_startup'     => 'Applications are accepted from the noon this date',
				       'pos_desc'        => 'Optional text shown on the application form',
				       'pos_status'      => 'The position status',
				       'pos_nickname'    => 'Short UNIQUE name for the position (no spaces), like "profs2012", "phd2011", etc.',
				       'pos_req_phd_year'=> 'Optional restriction on the "Phd age" (i.e. the position is for yang researchers)',
				       '_virt_observers' => 'The members have right to browse the applications',
				       '_virt_admins'    => 'Person responsible for the position has full access to all the documents',
				       '_virt_secretary' => 'Secretary has read-only access to  all the documents, gets Email notifications about new applicants, sends JAM e-mails',
				       ));
  
  private static $ranks = array(RANK_jam_SC1=>'pos_namesc1', 
				RANK_jam_SC2=>'pos_namesc2',
				RANK_jam_observer=>'');
  /*
   *
   */
  public function getValue($name, $asIs=True, $d_MBE=True){
    return parent::getValue($name,$asIs,True);
  }

  /*
   * Get array of pos_id's for all positions with the organization
   */
  public static function getPositions($org_id=Null,$pos_id=0){
    $reply = array();
    if (myPear_db()->columnExists('pos_status','jam_positions')){
      $where = array(1);
      if ($org_id) $where[] = "pos_orgid = $org_id";
      if ($pos_id) $where[] = "pos_id = $pos_id";
      $q = myPear_db()->query("SELECT pos_id,pos_status,pos_type,pos_name FROM jam_positions ".
				" WHERE ".join(' AND ',$where).
				" ORDER BY pos_deadline DESC");
      while ($r = myPear_db()->next_record($q)){
	$reply[$r['pos_id']] = $r;
      }
    }
    return $reply;
  }
  public static function getAll($org_id=Null,$ignore_rank=False){
    JAM_access();
    $w = array(1);
    if (!$ignore_rank && !JAM_manager_here) $w[] = "(pos_status='' OR pos_status IS NULL)";
    if (!empty($org_id))                    $w[] = "org_id = $org_id";

    $q = myPear_db()->qquery("SELECT pos_id FROM jam_positions ".
			       " LEFT JOIN zzz_organizations ON pos_orgid = org_id ".
			       " WHERE ".join(' AND ',$w).
			       " ORDER BY pos_deadline DESC",cnf_dev);
    $reply = array();
    while ($r = myPear_db()->next_record($q))      $reply[] = $r['pos_id'];
    return $reply;
  }

  /*
   */
  function   defineVariables() {
    static $defineVariable_counter = 0;
    if ($defineVariable_counter++ > 0) return;
   
    // Shortcut
    $pos_email = (myPear::is_empty($e=$this->getValue('pos_email',True)) ? "" : '<br/>'.x('em',"($e)"));

    // All variables
    $this->defineVariable(array('pos_ap_noreply'      => "Show the communication Email address to applicants?$pos_email",
				'pos_av_noreply'      => "Show the communication Email address to referees?$pos_email",
				'pos_bcc'             => 'Registrator Email',
				'pos_deadline'        => 'Deadline for the applications',
				'pos_desc'            => 'Position description',
				'pos_email'           => 'Email address for communication',
				'pos_orgid'           => 'Institution',
				'pos_logo'            => 'Position logo',
				'pos_n_referees'      => 'Required number of referees',
				'pos_name'            => 'Position title',
				'pos_namesc1'         => 'Section 1 (selection)',
				'pos_namesc2'         => 'Section 2 (search)',
				'pos_nickname'        => 'Position short title',
				'pos_refnumber'       => 'Reference number',
				'pos_req_nation'      => 'Ask for the citizenship?',
				'pos_req_ms'          => 'Master Degree required?',
				'pos_req_phd'         => 'PhD required?',
				'pos_req_phd_year'    => 'Restrict "PhD age"',
				'pos_sc_maybe_referee'=> 'May the members of Selection Committee act as Referees?',
				'pos_startup'         => 'When to announce the position',
				'pos_status'          => 'Position status',
				'pos_status_auto'     => 'Position status',
				'pos_type'            => 'Use JAM as the applications manager',
				'pos_url'             => 'URL where position is announced',
				'pos_agelimit'        => 'Age limit',
				'_virt_admins'        => 'Position responsible',
				'_virt_secretary'     => 'Secretaries',
				'_virt_observers'     => 'Observes',
				'_virt_fields'        => 'Optional Research Fields (sub-positions)',
				'_virt_docs'          => 'Documents to be attached to application',
				),True);
  

    $this->defineVariable('Please define the names for the Search&Selection Committee sections.-BR-Set the name to "'.CONST_noname.'" if the section is not needed:',
			  'pos_namesc');
    
    $this->defineTypes(array('isMBE'         => array('pos_startup',
						      'pos_bcc',
						      'pos_desc',
						      'pos_logo',
						      'pos_refnumber',
						      'pos_url',
						      'pos_namesc1',
						      'pos_namesc2',
						      'pos_status',
						      'pos_status_auto',
						      ),
			     'isRO'          => array('pos_orgid' => True,
						      'pos_namesc'=> True,
						      'pos_type'  => !JAM_manager_here,
						      'pos_logo'  => !JAM_manager_here),
			     'isVITAL'       => array('pos_deadline',
						      'pos_name',
						      ),
			     'submitOnChange'=> array('pos_type')));
    
    // Avoid recursion, do it at the end
    $pos_req_phd = $this->getValue('pos_req_phd');
    $this->defineTypes(array('isMBE'         => array('pos_req_phd_year'=>($pos_req_phd != JAM_yes_but)),
			     'isVITAL'       => array('pos_req_phd_year'=>($pos_req_phd == JAM_yes_but))
			     ));
    $this->dbg('completed',True);
  }
  

  /*
   *
   */  
  public function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);

    // Shortcuts
    //????    foreach(array('pos_email','pos_orgid') as $name) $this->defineVariable(array($name=>$name),'only_if_not_defined');
    
    if (!$this->getValue('pos_orgid',True)){
      $this->getValue_reset('pos_orgid',myOrg_ID);
    }

    $this->createRF();
    
    // Set names for the selection Committee units
    if ($this->ID === JAM_posID()){
      foreach(JAM::$SC_ranks as $rank){
        if (mb_strToLower($name=$this->SC_rank2name($rank)) !== CONST_noname) b_reg_bUnit::$_bUnits[JAM_MODULE][$rank]['d'] = $name;
      }
    }
    $this->setStatus();
    $this->isLocked($this->isStatus(JAM_pos_archived));
  }

  /*
   * Reset the cache when a new position is created
   */
  protected function _recordCreated(){
    unset($_SESSION[myPear_cache]);
  }
  
  /*
   * Call-back function
   */
  function deadline_reached(){
    $reply = $this->isStatus(JAM_pos_deadline);
    $this->dbg($reply);
    return $reply;
  }

  function createRF(){
    return;
    if ((int)$this->ID){
      $q = myPear_db()->query("SELECT * FROM jam_applicants ".
				 " WHERE ap_posid = ".$this->ID.
				 " GROUP BY ap_rfid");
      while ($r = myPear_db()->next_record($q)) {
	if (preg_match("/^other/i",$r['ap_field1'])) $r['ap_field1'] = 'Other';
	locateAndInclude('bForm_jam_RF');
	new bForm_jam_RF("name:".trim($r['ap_field1']),
		   'RO',
		   $this);
      }
    }
  }
  
  function getDNR(){
    if ($DNR = $this->formDB['pos_refnumber']) return "(Ref.N. $DNR)";
    else return '';
  }

  function getEmail(){
    return trim(mb_strToLower($this->getValue('pos_email',1)));
  }

  function getEmail_secretary() {
    return JAM_unit(RANK_jam_secr,$this->ID)->getEmails();
  }
  
  function getEmail_positionAdmin() {
    return JAM_unit(RANK_jam_posadmin,$this->ID)->getEmails();
  }
  
  function numberOfReseachFields() {
    return count($this->getRF_id());
  }

  function getRF_id() {
    $reply = array();
    $q = myPear_db()->query("SELECT rf_id FROM jam_rf WHERE $this->ID = rf_posid");
    while($r = myPear_db()->next_record($q)) $reply[] = $r['rf_id'];
    $this->dbg(join(', ',$reply));
    return $reply;
  }
    

  function nickname(){
    if (!($nickname = $this->getValue('pos_nickname',1))) $nickname = $this->getValue('pos_refnumber',1);
    if (! $nickname) $nickname = $this->name();
    return $nickname;
  }

  function SC_rank2name($rank){
    if ($rank == RANK_jam_observer)     $reply = b_reg_bUnit::$_bUnits[JAM_MODULE][$rank]['d'];
    elseif (empty(self::$ranks[$rank])) $reply = '';
    else                                $reply = $this->getValue(self::$ranks[$rank],True);
    return $reply; 
 }

  function name() {
    if ($n = $this->getValue('pos_name',True))  return $n;
    else                                        return 'New position';
  }

  /*
   * Read the Position record, join the Organization
   */
  function readDefRecord(){
    $this->formDB = array();
    if ((int)$this->ID){
      $q=myPear_db()->query("SELECT * FROM $this->TABLE ". 
                              " LEFT JOIN zzz_organizations ON pos_orgid = org_id ".
			      " WHERE $this->TABLEID='$this->ID'");
      while ($r=myPear_db()->next_record($q))  $this->formDB = $r;
      $this->formDB['pos_agelimit'] = JAM_age_restriction;
    }
  }
  
  /*
   * Is position managed by JAM?
   */
  public function managedByJAM(){
    if (!$this->getValue('pos_type',1,1))   $this->getValue_reset('pos_type','yes');
    return self::_managedByJAM(array('pos_type'=>$this->getValue('pos_type',1,1)));
  }

  public static function _managedByJAM($formDB=array()){
    $reply = !in_array(@$formDB['pos_type'],array('off','no'));
    return $reply;
  }

  /*
   * Check the status
   */
  public function isStatus($id,$formDB=array()){
    if (empty($formDB)) $formDB = array('pos_status' => $this->getValue('pos_status',1));
    $formDB['pos_status_auto'] = $this->getValue('pos_status_auto',1);
    
    return self::__isStatus($id,$formDB,'pos_status');
  }

  /*
   * @param $exclude_superUser 
   */
  function isConsidered($exclude_superUser=False){
    if (!$exclude_superUser && JAM_manager_here) $reply = True;
    else $reply = (!$this->isStatus(JAM_pos_archived) && !$this->isStatus(JAM_pos_in_preparation));
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Position owner is the JAM manager
   */
  function isOwner() {
    return $this->isManager();
  }
  
  /*
   * Position administrator
   */
  function isManager() {
    // Avoid an endless loop when applying updates
    if (!defined('JAM_posadmin_here')) return True;
    else                                  return JAM_posadmin_here;
  }

  /*
   * Hook the bForm_Avatar_jam.
   * The problem is that the Position class might be executed from outside the module context
   * (i.e. from a search engine) hence we do not want to set the "global context" to JAM
   */
  private function hookAvatar($av_id=Null){
    if (empty($av_id))                     $av = bAuth::$av;
    elseif($av_id instanceof bForm_Avatar) $av = $av_id;
    else                                   $av = myPear::getInstance('bForm_Avatar_jam',$av_id,array('nocache','strict'));
    if(($av instanceof bForm_Avatar) && !($av instanceof bForm_Avatar_jam)){
      $av = myPear::getInstance('bForm_Avatar_jam',$av->ID,array('nocache','strict'));
    }
    return $av;
  }

  /*
   * Check that the person is in the SC for this position
   */
  function isSCmember($av_id=Null,$ranks=Null) {
    if($reply = is_object($av=$this->hookAvatar($av_id))){
      $reply = $av->isSCmember($this,$ranks);
    }
    return $reply;
  }

  /*
   * Check that the logged-in person is of SC rank or higher for this position
   */
  function hasSCrank(){
    if($reply = is_object(bAuth::$av)){
      $reply = $this->isSCmember(bAuth::$av) || bAuth::$av->isSecretary($this->ID) || $this->isManager();
    }
    $this->dbg($reply,(bool)$this->debug||cnf_show_a);;
    return $reply;
  }

  
  function isReferee($av_id=False) {
    if($reply = is_object($av=$this->hookAvatar($av_id))){
      $reply = $av->isReferee($this->ID);
    }
    return $reply;
  }

  /*
   * This function constructs the logo as the list of favicons
   */
  function getLogo(){
    if (!$this->getValue('pos_logo',True))  $this->formDB['pos_logo'] = $_POST['pos_logo'] = myOrg_favicon;
    if ($this->MODE == 'RO'){
      $list = array();
      foreach (preg_split('/,/',$this->getValue('pos_logo',True)) as $org){
	$list[] = myOrg_favicon;
      }
      return join('',$list);
    }else{
      return $this->getValue('pos_logo',True);
    }
  }
  
  /*
   *
   */
  function getForm() {

    // Set the current position as the default one 
    if ((int)$this->ID) JAM_posID($this->ID);
    $this->managedByJAM();

    $button = array();    
    $edit_button = (($this->MODE == 'RO')
		    ? '[ update ]' // . bIcons()->get('i-edit')
		    : '');
    
    myPear::getPageHeader($this->name());
    
    //
    // Open the form with the status
    $this->getFormOpen();

    $this->statusBlock('pos_status');
    
    /*
     * General information block
     */
    $logo = $this->getLogo();
    $this->formBlock('','General info',
		     array('pos_type'     =>array('checkBox'),
			   'pos_orgid'      =>array('selectMenu',$this->allOrgs()),
			   'pos_name'     =>array('textField'),
			   'pos_nickname' =>array('textField'),
			   'pos_refnumber'=>array('textField'),
			   'pos_url'      =>array('textField'),
			   'pos_startup'  =>array('datePickerField'),
			   'pos_deadline' =>array('datePickerField'),
			   ),
		     'auto');
    
    if ($this->managedByJAM()){

      $this->textArea ("pos_desc");
      
      /*
       * Research fields block
       */
      $this->formBlock_bList(array('bList_jam_subPositions'=>array('method'=>'get_subPositions',
								   'arg1'  => $this->ID,
								   'field' =>'_virt_fields',
								   'icon'  =>'i-oxford')));
      /*
       * Required documents
       */
      $this->formBlock_bList(array('bList_jam_requiredDocs'=>array('method'=>'get_requiredDocs',
								   'arg1'  => $this->ID,
								   'field' =>'_virt_docs',
								   'icon'  =>'i-oxford')));
      /*
       * General policy
       */
      $dialog = array('pos_req_nation'  => array('radioButton',array(JAM_yes,JAM_no)),
		      'pos_req_ms'      => array('radioButton',array(JAM_yes,JAM_bonus,JAM_no)),
		      'pos_req_phd'     => array('radioButton',array(JAM_yes,JAM_yes_but,JAM_bonus,JAM_no)));
      // Check dependancies
      $this->require_MS();
      if ($this->getValue("pos_req_phd",1)==JAM_yes_but){
	foreach(array(1,2,3,4,5,7,10) as $y){
	  $list[$y] = ($y > 1 ? "within last  $y years" : "since a year");
	}
	$dialog['pos_req_phd_year'] = array('selectMenu',$list);
      }else{
	$this->getValue_reset_forced('pos_req_phd_year',0);
      }
      $this->formBlock('','General policy',
		       $dialog,
		       'auto');

      
      /*
       * Search Committee 
       */
      $this->formDB['pos_namesc'] = '&nbsp;';
      $dialog = array('pos_namesc1' ,'pos_namesc2' );
      if ($this->MODE == 'RW') array_unshift($dialog,'pos_namesc');
      $this->formBlock('','Search & Selection Committee structure',
		       $dialog,
		       'auto');
      //
      // Position observers (i.e. the persons who can brows the applications)
      //
      $this->formBlock_bList(array('bUnit_jam'=>array('method'=>'get',
						      'arg1'  => $this->ID,
						      'arg2'  =>RANK_jam_observer,
						      'field' =>'_virt_observers',
						      'icon'  =>'i-oxford')));
      
      
      //
      // How many referees should be provided
      //
      $this->formBlock('','Referees & Search committee policy',
		       array(
			     'pos_n_referees'=>array('selectMenu',array('-1'=>'no referees',1=>1,2=>2,3=>3,4=>4,5=>5)),
			     'pos_sc_maybe_referee'=>array('radioButton',array(JAM_yes,JAM_no))),
		       'auto');
      
      //
      // Communication with applicants/referees
      //
      $this->formBlock('','Communication with applicants and referees',
		       array('pos_bcc'       =>array('textField'),
			     'pos_email'     =>array('textField'),
			     'pos_ap_noreply'=>array('radioButton',array(JAM_yes,JAM_no)),
			     'pos_av_noreply'=>array('radioButton',array(JAM_yes,JAM_no))),
		       'auto');
      
      //
      // Position administrators (i.e. the persons who employ the applicants)
      //
      $this->formBlock_bList(array('bUnit_jam'=>array('method'=>'get',
						      'arg1'  => $this->ID,
						      'arg2'  =>RANK_jam_posadmin,
						      'field' =>'_virt_admins',
						      'icon'  =>'i-oxford')));
      
      //
      // Position secretaries (i.e. the persons who employ the applicants)
      //
      $this->formBlock_bList(array('bUnit_jam'=>array('method'=>'get',
						      'arg1'  => $this->ID,
						      'arg2'  =>RANK_jam_secr,
						      'field' =>'_virt_secretary',
						      'icon'  =>'i-oxford')));
    }
    
    if (0)    if ($this->MODE == 'RO') {
	if ($this->isWritable()) $submit['update the position policy'] = b_url::same("?form=jam_Position&mode=RW&id=$this->ID");
	$submit['preview the application form'] = b_url::same("?form=jam_Applicant&id=preview:".$this->formDB['pos_id']);
      }
    if (empty($submit)) $this->closeForm();
    else                $this->closeForm($submit);
  }

  /*
   * Ask the applicants for the citizenship?
   */
  public function askCitizenship(){
    return self::_require('pos_req_nation',array('pos_req_nation'=>$this->getValue('pos_req_nation',True)));
  }

  /*
   * Ask the applicants for the Master Degree?
   */
  public function require_MS(){
    if ($this->require_PhD())                   $this->getValue_reset_forced('pos_req_ms',JAM_yes);
    elseif(!$this->getValue('pos_req_ms',True)) $this->getValue_reset_forced('pos_req_ms',JAM_bonus);
    return self::_require('pos_req_ms',array('pos_req_ms'=>$this->getValue('pos_req_ms',True)));
  }

  /*
   * Ask the applicants for PhD?
   */
  public function require_PhD(){
    return self::_require_PhD(array('pos_req_phd' => $this->getValue('pos_req_phd')));
  }

  public static function _require_PhD($buffer){
    return self::_require('pos_req_phd',$buffer);
  }

  /*
   *
   */
  private static function _require($what,$buffer=array()){
    if (empty($buffer[$what])){
      myPear::WARNING_T("???empty '$what'");
      $reply = False;
    }else{
      $reply = ($buffer[$what] === JAM_yes) || ($buffer[$what] === JAM_yes_but);
    }
    return $reply;
  }
  
  private function allOrgs(){
    $list = array();
    $q = myPear_db()->query("SELECT * FROM zzz_organizations");
    while ($r = myPear_db()->next_record($q))       $list[$r['org_id']] = $r['org_name'];
    return $list;
  }

  /*
   *
   */
  function formOKspecific() {
    if (!$this->managedByJAM()){
      foreach (array("pos_name", "pos_url", "pos_deadline") as $k=>$v){ 
	if (!$_POST[$v])  $this->errorsInTheForm[$v] = 'missing value';
      }
    }else{
      
      // Set names for the selection Committee units
      foreach(JAM::$SC_ranks as $rank){
	if (mb_strToLower($this->SC_rank2name($rank)) == CONST_noname){
	  if (JAM_unit($rank,$this->ID)->get_unitMemberIDs()){
	    myPear::ERROR("Can't remove section <em>'".b_reg_bUnit::$_bUnits[JAM_MODULE][$rank]['d']."s'</em>");
	    $this->errorsInTheForm[self::$ranks[$rank]] = 'not empty';
	  }
	}
      }
    }
  }
    
  /*
   *
   */
  public function setStatus($what=Null){
    //    $this->defineVariables();
    if (!defined('JAM_pos_deadline')){
      b_debug::traceBack('no JAM_pos_deadline');
    }elseif ((int)$this->ID){

      $pos_status_auto = array();
      
      // Adjust the deadline
      if (!$this->getValue('pos_deadline',1))  $this->getValue_reset_forced('pos_deadline',time() + 24*3600);

      // Adjust the startup
      $n_days_in_advance = 15;
      $s = $this->getValue('pos_startup', 1);
      $d = $this->getValue('pos_deadline',1);
      if (empty($s) || ($d  < $s+$n_days_in_advance*24*3600))     $this->getValue_reset_forced('pos_startup', $d - $n_days_in_advance*24*3600);
      $this->dbg('Application campaign: '.b_time::period($this->getValue('pos_startup'),$this->getValue('pos_deadline')));

      foreach(JAM::$statuses as $id=>$a){
        switch($id){
	  
	case JAM_pos_deadline:
	  // be nice, the deadline is the last day for applying
	  $status = ($this->getValue('pos_deadline',True)  < time() - 24*3600);
          break;

	case JAM_pos_not_yet:
	  // return 'False' if the application campaign is already started,
	  $status = ($this->getValue('pos_startup',1) >= time());
	  break;
	  
	default:
	  $status = False;
        }
        if ($status) $pos_status_auto[] = $id;
      }
      if (myPear_db()->columnExists('pos_status_auto','jam_positions')) {
	$this->getValue_reset_forced('pos_status_auto',serialize($pos_status_auto));
      }else{
	$this->formDB['pos_status_auto'] = serialize($pos_status_auto);
      }
      if ($this->debug) b_debug::print_r(array_merge(array($this->getValue('pos_status',1)),$pos_status_auto),"$this->".__FUNCTION__.'()');
    }
  }
  
  function mayRefereeBeSC() {
    return ($this->getValue('pos_sc_maybe_referee') == JAM_yes)
      ? True
      : False;
  }
}
