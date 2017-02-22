<?php
/*                                YB 2011-05-07
 * Employees unit for a (small) organization.
 * 
 * Member list record:
 *  um_id
 *  um_uid
 *  um_avid
 *  um_status - join(<EA_role> , <a_flag ('a' for alumni>))
 *  um_option - packed (<board membership> , <a_flag> , <acl ('p' for public)>)
 */

locateAndInclude('bUnit_ea');
class bUnit_ea_members extends bUnit_ea {

  var $debug    = False;
  var $see_only = Null;
  
  protected $packed_items= array('role2','acl','day1','day9');
  
  
  /*
   * EA unit constructor
   */
  public $myOrg_ID = Null;
  function __construct($parent_id=myOrg_ID,$rank=RANK__ea_member){
    parent::__construct(($this->myOrg_ID = $parent_id),$rank);
  }
  
  /*
   * This method should be ran regularly, better as a cron job.
   *
   * Rebuild the EA unit.
   * Check all the empl. period and "move" members to alumni or back
   */ 
  function rebuild(){
    locateAndInclude('bForm_Avatar_ea');
    set_time_limit(0);

    myPear_db()->transaction('start');
    myPear::MESSAGE_open(sprintf("Rebuilding %s",$this->name()));
    foreach($this->getUnitMemberIDs() as $av_id){
      if (empty($av_id)){
	if (cnf_dev) b_debug::traceBack("empty unit member???");
	$this->deleteMember(array('um_avid'=> '$av_id'),cnf_dev||cnf_demo);
	continue;
      }
      ob_start();
      $av = new bForm_Avatar_ea($av_id);
      myPear::MESSAGE_add($av->fmtName('Lf'),True);
      $av->want_legacy_maildb = $av->isE(True) && (myOrg($this->myOrg_ID)->getCode() == 'nordita');
      $av->getForm();
      $av->adjust_status();
//    $av->update_AVpos_AVinst('av_institute');
//    $av->update_AVpos_AVinst('av_position');
      $x = ob_get_contents();
      ob_end_clean();
      // We do not need an output, just kill it
      unset($x);
    }
    myPear::MESSAGE_close();
    $this->get_memberInfo(0,0,'reset');
    myPear_logs()->add('rebuild',0,$this->name());
    myPear_db()->transaction('end');
    myPear_logs()->syslog('Rebuild EA unit for '.$this->name());
  }

  /*
   * Rebuild EA lists for all the organizations
   */
  public static function rebuild_all(){
    locateAndInclude('bForm_Organization');
    foreach(bForm_Organization::getAll() as $org_id){
      $ea = new bUnit_ea_members($org_id);
      $ea->rebuild();
    }
    return True;
  }
 
  /*
   *
   */
  function name($show_listType=False){
    return Null;
  }

  /*
   *
   */
  protected function add_unitMember_postFly($av_id,$alreadyThere){
    $this->bUnit_buttons_row[] = b_btn::submit("see ".$this->get_memberInfo($av_id,'sfl'),
					       b_url::same("?form=".bForm_Avatar::$bForm_Avatar."&id=$av_id"),
					       b_url::get_GET('(.*ed_bList.*)',True));
  }
  
  /*
   * Add avatar to the unit, create an empty employment record
   */
  function add_unitMember($av_id,$verbose=True,$um_status=Null){
    if ($av_id instanceof bForm_Avatar){
      $av = $av_id; 
      $av_id = $av->ID; 
    }
    if (!(@$av instanceof bForm_Avatar_ea)) $av = new bForm_Avatar_ea($av_id);

    if ($this->isMember($av_id)){
      $reply = parent::add_unitMember($av,$verbose,$um_status);
    }else{
      $um_status = $this->see_only;
      $um_id = parent::add_unitMember($av,$verbose,$um_status);
      $av->hook_employment();
      $reply = $um_id;
    }
  }

  /*
   *
   */
  protected function setPageHeader() {
    //   if (($this->see_only === EA_role_alumnus) && !in_array(EA_role_alumnus,$role2)) $accept = False;
    //    if (($this->see_only !== EA_role_alumnus) &&  in_array(EA_role_alumnus,$role2)) $accept = False;
    myPear::H1($this->see_only,'reset');
  }
  
  /*
   *
   */
  function setStatus($av_id,$status,$verbose=True){
    if ($this->isMember($av_id)){
      $current_status = $this->getUnitMemberStatus($av_id);
      if (empty($current_status) && empty($status)) $status = EA_role_alumnus;
      return parent::setStatus($av_id,$status,$verbose);
    }
  }

  /*
   *
   */
  function unitMemberIcon($av_id=Null){
    $d = b_reg_bUnit::get_description($this->rank);
    $d['d'] = $this->unitMemberTitle($av_id);
    return bIcons()->get($d);
  }

  /*
   * Give back the detailed title of a EA member
   */
  function unitMemberTitle($av_id=Null){
    $this->dbg();
    $s2t=array(EA_role_employee                   => EA_title_employee,
	       EA_role_employee.EA_role_alumnus   => EA_title_alumnus,
	       EA_role_lt_visitor                 => EA_title_lt_visitor,
	       EA_role_lt_visitor.EA_role_alumnus => EA_title_alumnus.' '.EA_title_lt_visitor,
	       EA_role_boardMember                => EA_title_boardMember,
	       EA_role_boardMember.EA_role_alumnus=> EA_title_alumnus.' '.EA_title_boardMember,
	       );
    $s2a=array(EA_role_affiliate                  => EA_title_affiliated,
	       EA_role_affiliate.EA_role_alumnus  => EA_title_alumnus.' '.EA_title_affiliated);

    $status = $this->get_memberInfo($av_id,'um_status');
    $role2  = $this->get_memberInfo($av_id,'role2');

    if    ($title = @$s2t[$status])    $reply = $title;
    elseif($title = @$s2t[$role2])     $reply = $title;
    elseif($title = @$s2a[$status])    $reply = $title;
    if (empty($reply)){
      $reply = parent::unitMemberTitle($av_id);
    }else{
      $reply = myOrg($this->myOrg_ID)->name(True) . ' ' . $reply;
    }
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * Add EA specific items to the members cache
   */
  protected function membersInfo_toSelect($extended=False){
    return array_merge(parent::membersInfo_toSelect($extended),
		       array('av_institute',
			     'av_position',
			     'day1',
			     'day9',
			     'role2'));
  }
  
  /*
   * This function tries to resolve the egg-chicken problem - it is called in 2 contradictory environments 
   * - to build the list of tabs
   * - check access for the given tab
   */
  protected function accept_for_tab(&$rec,$tabs=Null,$tab=''){

    if (empty($rec)) return False;

    // Get all the affiliations    
    static $affil=array(), $default_affiations=array();
    if (empty($default_affiations)){
      $default_affiations = myPear_set_roles('affiliation');
      $affil = myOrg()->getAffiliation();
    }

    $accept = True;
    foreach($affil as $role=>$accepted){
      if ((strpos(@$rec['um_status'],$role) !== False) && !$accepted) $accept = False;
      if ((strpos(@$rec['role2'],$role)     !== False) && !$accepted) $accept = False;
    }
    
    // If the affiliation type is accepted, check further down
    if ($accept){
      $stats = preg_split('//', @$rec['um_status'], -1, PREG_SPLIT_NO_EMPTY);
      $role2 = preg_split('//', @$rec['role2'],     -1, PREG_SPLIT_NO_EMPTY);
      if ($accept = (($tabs === Null)
		     ? True
		     : parent::accept_for_tab($rec,$tabs,$tab))){
	if ($tab == $default_affiations[EA_role_boardMember]['group']){ // Board
	  if (($this->see_only === EA_role_alumnus) && !in_array(EA_role_alumnus,$role2)) $accept = False;      
	  if (($this->see_only !== EA_role_alumnus) &&  in_array(EA_role_alumnus,$role2)) $accept = False;      
	}else{
	  if (($this->see_only === EA_role_alumnus) && !in_array(EA_role_alumnus,$stats)) $accept = False;      
	  if (($this->see_only !== EA_role_alumnus) &&  in_array(EA_role_alumnus,$stats)) $accept = False;      
	}
      }
      
      if ($accept){
	switch($tab){
	  
	case $default_affiations[EA_role_lt_visitor]['group']: // Visitors
	  $trigger = EA_role_lt_visitor;
	  break;
	  
	case $default_affiations[EA_role_boardMember]['group']: // Board
	  $trigger = EA_role_boardMember;
	  break;
	  
	case $default_affiations[EA_role_affiliate]['group']: // Affiliated
	  $trigger = ($affil[EA_role_boardMember] && in_array(EA_role_boardMember,$role2)
		      ? 'No'
		      : EA_role_affiliate);
	  break;
	  
	default:  
	  if (empty($this->see_only)) $trigger = @$stats[0];
	  else                        $trigger = $this->see_only;
	}
	$accept = in_array($trigger,$stats) || in_array($trigger,$role2);
      }
    }
    return $accept;
  }

  protected function header(){
    return array('name'=>'', 'av_institute'=>'','av_position'=>'');
  }

  /*
   *
   */
  function isWritable(){
    $reply = b_cnf::check_conditions(array(True => array('parent' => parent::isWritable(),
							 'admin'  => $this->isAdmin() && !cnf_inside_fb,
							 ),
					   'default'=> False),
				     $this,
				     False);
    return $reply;
  }
  
  /* 
   * 
   */
  function isMemberAccess_public($av_id,$check_DB_only=False){
    if ($check_DB_only) $reply = False;
    else                $reply = parent::isMemberAccess_public($av_id);
    if (!$reply || $check_DB_only){
      $r = $this->getUnitMember($av_id);
      $reply = (@$r['acl'] === EA_acl_public);
    }
    return $reply;
  }

  /*
   *
   */
  function setMemberAccess($av_id,$acl){
    if ($this->isMember($av_id)){
      $this->updateMember(array('acl'=>$acl, 'um_avid'=>$av_id),$this->get_memberInfo($av_id,'um_id'));
      myPear::MESSAGE($this->get_memberInfo($av_id,'sfl').' - set access level to "'.($acl===EA_acl_public ? 'public' : 'protected').'"');
    }
  }
  
  /*
   * Editable either by the owner or by the administrator
   */
  protected function isMemberEditable(){
    $reply = $this->isWritable() || (@bAuth::$av->ID === $this->rec['av_id']);
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Not deletable if there is an employment record "associated with organization"
   */
  protected function isUnitMemberDeletable(){
    locateAndInclude('bList_eaEmpRecords');
    return b_cnf::check_conditions(array(False    =>array('isEA'=>bList_eaEmpRecords::isEA($this->rec['um_avid'],$this->myOrg_ID)),
					 'default'=> parent::isUnitMemberDeletable()),
				   $this); 
  }

  protected function all_members_tidy_name($code=''){
    if ($code === '') $code = $this->see_only;
    switch($code){
    case EA_role_alumnus:  $reply = 'Alumni'; break;
    case EA_role_employee: $reply = 'Staff';  break;
    default:               $reply = 'Staff & Alumni';
    }
    return $reply;
  }

  /*
   * Messages confirming alumni "creation"
   */
  protected function message_url($av_id){
    if (cnf_CLI) {
      $reply = '';
    }else{
      $url = b_url::same("?form=".bForm_Avatar::$bForm_Avatar."&id=$av_id",
			 array('ed_bList','holder'));
      $reply = (False
		? bJS()->modal_ajax($url,'[ see the record ]',Null,True)
		: b_btn::link('see the record',$url));
    }
    return $reply;
  }
  

  /*
   * NORDITA CUSTOMIZED....
   */
  protected function detect_selectedTab($tabs){
    if (($reply = parent::detect_selectedTab($tabs))==0 && (count($tabs)>0) && is_object($this->av_justAdded)){
      if ($this->see_only == EA_role_employee && !empty(EA::$extra_tabs_to_show)){
	switch($status = $this->getUnitMemberStatus($this->av_justAdded->ID)){
	case EA_role_lt_visitor:
	  $reply = 1;
	  break;

	case EA_role_boardMember:
	  $reply = 2;
	  break;
	}
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  
  /*
   * Extend the EA list by a few optional tabs (like "Visitors", "Board members", etc.),
   * but check first that they are populated 
   */
  protected function build_tabs_for_show(){

    // get the "root tab(s)"
    $reply = parent::build_tabs_for_show();

    // get the affiliations used by the organization
    $affiliations = myOrg()->getAffiliation();

    // walk thru all the posible affiliations and select those used by the organization
    foreach(myPear_set_roles('affiliation') as $role=>$descr){
      // Skip the "root tab" (those which have the 'page' argument),
      // they are processed by the parent method
      if ($affiliations[$role] && !@$descr['page']){
	$ok = False;
	foreach($this->get_memberInfo() as $av_id=>$r){
	  if ($this->accept_for_tab($r,Null,$descr['group'])){
	    $ok = True;
	    break;
	  }
	}
	if ($ok) $reply[$descr['group']] = $descr['group'];
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Get the members by status
   */
  function getMembers_byStatus($statuses=array(EA_role_employee,
					       EA_role_lt_visitor)){
    return $this->get_memberInfo(Null,$statuses,'get_byStatus');
  }
  
  /*
   * Get list of employees for the given time period
   */
  function getMembers_byDate($day_1,$day_9){
    if ((int)$day_1) $day_1 = date('Y-m-d',$day_1);
    if ((int)$day_9) $day_9 = date('Y-m-d',$day_9);
    
    // Look for the packed bList_eaEmpRecords records
    self::$_serialized = 'lm_option';
    $where = self::args2select(array('e_org'=>$this->myOrg_ID),
			       array('zzz_lists','zzz_list_members'));
    $where[] = "um_uid = $this->ID AND ".
      "((lm_key    >= '$day_1' AND lm_key    <= '$day_9') OR ".
      " (lm_status >= '$day_1' AND lm_status <= '$day_9') OR ".
      " (lm_key    <= '$day_1' AND lm_status >= '$day_9') OR ".
      " (lm_key    <= '$day_1' AND lm_status  = 'present'))";
    
    $q = myPear_db()->query("SELECT * FROM zzz_avatars ".
			    " LEFT JOIN zzz_unit_members ON av_id = um_avid ".
			    " LEFT JOIN zzz_lists        ON av_id = l_parent ". // employment history
			    " LEFT JOIN zzz_list_members ON l_id  = lm_lid ".
			    " WHERE " . join(' AND ',$where) . myPear_db()->orderBy(),
			    $this->debug);
    $reply = array();
    while ($r=myPear_db()->next_record($q)){
      if ($o = $r['lm_option']) bList::_unserialize($r,'lm_option',array_keys(unserialize($o)));
      $reply[] = $r;
    }
    $this->dbg(count($reply).' members '.b_time::period(b_time::txt2unix($day_1),b_time::txt2unix($day_9)));
    return $reply;
  }

  /*
   *
   */
  function format(){
    if(0)    if (EA_administrator_here){
      array_unshift($this->t->extraTD,bIcons()->getButton(array('i' =>'i-mail-send',
								'm' =>'forced',
								'li'=>"?tab_force=ea_remind_password&av_id=".$this->rec['um_avid'],
								'd' =>'remind password',
								'f' =>'RO')));
    }
  }
  
  /*
   *
   */
  function getValue($name,$asIs=True,$description_MBE=True) {
    switch ($name){
    case 'l_name':
      if     ($this->see_only == EA_role_alumnus)  $reply =myOrg($this->myOrg_ID)->name().' alumni'; 
      elseif ($this->see_only == EA_role_employee) $reply =myOrg($this->myOrg_ID)->name().' staff';
      else                                         $reply =myOrg($this->myOrg_ID)->name().' employees & alumni';
      break;

    case 'l_member_title':
      if     ($this->see_only == EA_role_alumnus)  $reply = 'alumnus'; 
      elseif ($this->see_only == EA_role_employee) $reply = 'employee';
      else                                         $reply = 'EA';
      break;

    default:
      return parent::getValue($name);
    }
    return $this->getValue_return("see_only_".$this->see_only,$reply);
  }

  /*
   * Send the password to EA
   */
  function remind_password($av_id) {
    locateAndInclude('bMailer_ea');
    myPear::H1('empty');
    EA_mailer()->remind_password($av_id);
  }
}
