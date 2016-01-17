<?php
/*
 *
 */
locateAndInclude('bForm_Avatar_wiw');
class bForm_wiw_trip extends bForm{
  
  /*
   * Adjust the leave / arrival dates:
   * - set the leave / arrival time to noon
   * - optionally set the arrival to "-1 day", 
   *   old convention was "back to work", hence adjust it)
   */
  public static function _adjust_dates(&$t_departure, &$t_arrival,$old_convention = False){
    // Departure from home
    $t_departure = b_time::noon(0,0,0,$t_departure);
    $t_arrival   = b_time::noon(0,0,0,$t_arrival);
    list($d_was,$a_was) = array($t_departure,$t_arrival);

    // Back home (old convention was "back to work", hence adjust it)
    if ($old_convention){
      $t_arrival = b_time::noon(0,0,0,$t_arrival - 24 * 3600);
      // while(b_time::isRedDay($t_arrival)) $t_arrival = b_time::noon(0,0,0,$t_arrival - 24*3600);
    }
    $t_arrival = max($t_arrival,$t_departure);
    
    if (cnf_CLI) if (($t_arrival != $a_was) || ($t_departure != $d_was)) 
		   b_debug::xxx("<br/>old:\n".b_time::period($d_was,$a_was).
				"<br/>new:\n".b_time::period($t_departure,$t_arrival));
  }
  
  /*
   *
   */
  public static function getTrips($t_type=Null,$org_id=Null,$group=Null){
    $where[] = 1;
    if ($t_type) $where[] = "t_type = $t_type";
    $q = myPear_db()->query("SELECT t_id FROM wiw_trips WHERE ".join(' AND ',$where));
    
    $reply = array();
    while($r=myPear_db()->next_record($q)){
      $reply[] = $r['t_id'];
    }
    return $reply;
  }

  /*
   * The constructor
   */
  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);

    // This will be redefined later
    $this->defineVariable(array('t_avid'=>'t_avid'),'only_if_not_defined');

    // Hook Avatar
    foreach(array($this->getValue('t_avid',True,1),
		  @$_GET['av_id'],
		  ) as $av_id) if (!empty($av_id)){
      $this->getValue_reset('t_avid',$av_id);
      break;
    }
  }
  
  /*
   * Set the access control
   * - private:   only the owner sees the trip
   * - protected: only the group members see the trip
   * - public:    anyone on the web can see it
   *
   * The trip is not public if
   * - the owner explicitly sets the acl to 'protected/private'
   * - the trip owner is not any more the group member.
   */
  private function adjust_acl(){
    return;
  }
  
  /*
   * The trip is not visible for the "old organization" if the traveler
   * has retired AND the travel is within the "new organization"
   */
  public static function _trip_visible(&$formDB){
    $reply = True;

    if ($formDB['t_avid'] == @bAuth::$av->ID){
      if (cnf_dev) myPear::MESSAGE("Own trips are always visible");
    }else{
      // Check if the person has ever changed the organization
      $orgs = array_values(bUnit::get_unitsMembership(Null,RANK__ea_member,$formDB['t_avid'],'bUnit_ea_members'));
      $orgs = $orgs[0];
      if (count($orgs) > 1){
	$margin = 33 * 24 * 3600;
	// Yes, the organization was changed at least once. 
	// Check that the trip is within the emp.period of the current organization  
	$day1 = myOrg(myOrg_ID)->eaMembers()->get_memberInfo($formDB['t_avid'],'day1');
	if ($formDB['t_arrival']+$margin < $day1){ 
	  if (cnf_dev) myPear::DEBUG($formDB['av_lastname']." started at ".myOrg_nameS.date(' Y-m-d',$day1).", trip \"".b_time::period($formDB['t_departure'],$formDB['t_arrival']).'" is not visible');
	  $reply = False;
	}

	$day9 = myOrg(myOrg_ID)->eaMembers()->get_memberInfo($formDB['t_avid'],'day9');
	if ($formDB['t_departure']-$margin > $day9){ 
	  if (cnf_dev) myPear::DEBUG($formDB['av_lastname']." retired from ".myOrg_nameS.date(' Y-m-d',$day9).", trip \"".b_time::period($formDB['t_departure'],$formDB['t_arrival']).'" is not visible');
	  $reply = False;
	}
      }      
      
      // Check ACL
      $t_acl  = (($a = @$formDB['t_acl']) ? $a : wiw_acl_public);
    }
    return $reply;
  }
  
  /*
   * Set the departure/arrival time to noon
   */
  private function adjust_dates(){
    self::_adjust_dates($t_departure=$this->getValue('t_departure',True),
			$t_arrival  =$this->getValue('t_arrival',True));
    $this->getValue_reset('t_departure',$t_departure);
    $this->getValue_reset('t_arrival',  $t_arrival);
  }

  /*
   * Define variables
   */
  function defineVariables() {
    $this->defineVariable(array('t_departure'  => 'Departure date',  
				't_arrival'    => 'Arrival date', 
				't_type'       => 'Type of the trip',
				't_destination'=> 'Going to -SKIP- <br/><em>e.g. Country, City, Institution, etc.</em>',
				't_contact'    => 'How to contact -SKIP- <br/>'.x('i',nbsp('(phone number, email, etc.)')),
				't_comment'    => 'Other info -SKIP- <br/><i>e.g.purpose of the trip<br />title of talk<br /></i>',
				't_avid'       => 'The traveler',
				't_acl'        => 'Visibility',
				),True);
    $this->defineVariable(array('_virt_period' => ($this->getDescr('t_departure').
						   ' -> '.
						   $this->getDescr('t_arrival'))),
			  True);
    
    $this->defineTypes(array('isMBE'  => array('t_acl',
					      't_comment',
					      't_contact'),
			     'isVITAL'=> array('t_type',
					       't_destination')));
  }

  /*
   * The trip owner is the traveler
   */
  private $isOwner = Null;
  function isOwner(){
    if ($this->isOwner === Null)
      $this->isOwner = (bool)b_cnf::check_conditions(array(True => array('owner'=>($this->getValue('t_avid',True) == @bAuth::$av->ID),
									 'mngr' => WIW_manager_here),
							   'default'=>parent::isOwner()),
						     $this,
						     cnf_dev);
    return $this->isOwner;
  }
  
  /*
   * Helper function for the getValue
   */
  protected function getValue_validate_empty($name,&$value=Null){
    $err_msg = '';
    if (myPear::is_empty($value)){
      $err_msg = parent::getValue_validate_empty($name,$value);
      switch($name){
      case 't_orgid':
	$err_msg = '';
	$value = myOrg_ID;
	break;
      }
    }
    return $err_msg;
  }
  
  /*
   *
   */
  function getForm() {

    // Open the dialog
    $this->getFormOpen();

    if (WIW_manager_here && !$this->getValue('t_avid',True)){
      // The manager mode.
      // Ask who is the trip owner
      
      myPear::MESSAGE("You are the manager of the \"WhoIsWhere\" database, ".
		      "hence may submit the travel request on behalf of any group member");
      
      // get the group members list
      foreach(myOrg(myOrg_ID)->eaMembers()->get_memberInfo(Null,EA_role_employee,'get_byStatus') as $av_id=>$r){
	$new_travelers[$av_id] = mb_strToUpper($r['av_lastname']).' '.$r['av_firstname'];
      }
      $new_travelers[bAuth::$av->ID] = " myself";
      asort($new_travelers);
      
      $this->submitOnChange['t_avid'] = True;
      $this->formBlock('wiw',
		       'Who is the traveler?',
		       array('t_avid'=>array("selectMenu",$new_travelers)),
		       array('d'=>'auto',
			     'i'=>'i-portfolio_edit',
			     'l'=>b_url::same("?form=wiw_trip&mode=RW&id=".$this->ID)));
      
    }else{
      
      if ($this->MODE == 'RO') $this->showCalendar('t_departure','t_arrival');
      
      // Embed the Avatar
      if (!$this->getValue('t_avid',True)) $this->getValue_reset('t_avid',bAuth::$av->ID);
      $av = new bForm_Avatar_wiw($this->getValue('t_avid',True));
      myPear::H1($av->fmtName('fl'));
      
      // Show the owner contact information
      $av->defineVariables();
      $av->isEmbedded($this);
      $av->isLocked(True);
      $av->contactsBlock();
      
      // Show the trip details
      if (!$this->getValue('t_contact',1)) $this->getValue_reset('t_contact',$av->getValue('av_email',1));
 
      $dialog = array('_virt_period' => array('datePickerField2','t_departure','t_arrival','i-a-depart','i-a-arrive'),
		      't_type'       => array('selectMenu_bList',WIW::_TT($this->getValue('t_orgid'))),
		      't_destination'=> array('textField'),
		      't_contact'    => array('textField'),
		      't_comment'    => array('textArea'),
		      't_avid'       => array('textHidden',$this->getValue('t_avid',True)));
      if ($this->MODE == 'RO') unset($dialog['_virt_period']);
      
      $this->formBlock('ti',
		       'Trip information',
		       $dialog,
		       array('d'=>'auto',
			     'i'=>'i-portfolio_edit',
			     'l'=>b_url::same("?form=wiw_trip&mode=RW&id=".$this->ID)));
    }			   
    
    $this->closeForm(($this->MODE=='RW'?'submit':''));
    bIcons()->explain();
  }

  /*
   *
   */
  function typeOptions(){
    static $types = array();    
    if (empty($types)){
      $q = myPear_db()->query("SELECT * FROM wiw_ttype ORDER BY tt_id");
      while ($r=myPear_db()->next_record($q)) $types[$r['tt_id']] = $r['tt_name'];
    }
    return $types;
  }

  function formOKspecific() { 
    if (0) if ($this->getValue('t_departure',True) >= $this->getValue('t_arrival')) {
      $this->printErrorAsIs['t_arrival'] = True;
      $this->errorsInTheForm['t_arrival'] = x("'",$this->getDescr('t_departure')).
	" must be before ".x("'",$this->getDescr('t_arrival'));
      }
    if (0) if (($this->getValue('t_departure',True)) &&
	($this->getValue('t_departure',True) == $this->getValue('t_arrival'))){
      switch(b_btn::confirm()){
      case 'no':
	$this->getValue_reset('t_arrival',$this->getValue('t_departure',True) + 25*3600);
	break;
	
      case 'yes':
	$this->getValue_reset('t_departure',b_time::check_out($this->getValue('t_departure',True)));
	$this->getValue_reset('t_arrival',  b_time::check_in ($this->getValue('t_arrival',True)));
	break;
	
      case Null:
      default:      // ask for confirmation
	foreach(array('t_avid',
		      't_type',
		      't_destination',
		      't_contact',
		      't_comment') as $a) $hidden_args[$a] = $this->getValue($a,True);
	b_btn::confirm("The departure & arrival are the same day. Are you sure you want to do this?", True, $hidden_args);
      }
    }
  }

  /*
   *
   */
  protected function cancelButton(&$buttons,$text='cancel'){
    
    if (isset($_GET['btt_once'])){ // btt = back to trip
      $buttons['back to my trips'] = b_url::same("?tab_force=tab_wiw_myTrips",array('t_avid'));
    }else{
      $buttons[$text] = $this->sameURL_but("?cancel_once=t_avid&btt_once=1&",array('t_avid'));
    }
    if (cnf_dev){
      foreach($buttons as $txt=>$url){
	$u = b_crypt::parse_query_string($url);
	b_debug::print_r($u,"Button \"$txt\"");
      }
    }
  }
}
