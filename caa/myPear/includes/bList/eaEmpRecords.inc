<?php
locateAndInclude('bList');
locateAndInclude('bForm_Avatar_ea');

define('emp_education' , 'Education');
define('emp_major_pos' , 'Major Positions');
define('emp_lt_visits' , 'Visiting Appointments');
define('emp_awards'    , 'Selected Awards and Honors');
define('emp_other'     , 'Other Activity');

class bList_eaEmpRecords extends bList{
  
  protected $isReady  = True;
  public $tips_byMode = array('RW'=>array('lm_period'  => "If you don't know the future end date of the period, type '2099-01-01'",
					  '_emp_period'=> "If you don't know the future end date of the period, type '2099-01-01'",
					  ));
  var $rec_type_menu = array();
  var $rec_type_menuD= array('separator00'=> '',
			   '20_mp_org'  => 'ORG employment',
			   '40_va_org'  => 'ORG visiting appointment',
			   '22_op_org'  => 'ORG other activity',
			   'separator01'=> '',
			   '10_cp'      => 'current position',
			   '20_mp'      => 'major position',
			   '22_op'      => 'other activity',
			   'separator10'=> '',
			   '30_h'       => 'habilitation',
			   '34_phd'     => 'phd degree',
			   '38_ms'      => 'master\'s degree',
			   'separator20'=> '',
			   '40_va'      => 'visiting appointments',
			   '50_ah'      => 'awards and honors',
			   'separator30'=> '',
			   );
  
  // this information is packed into the lm_option field
  protected $packed_items = array('e_org', 'e_cc',  'e_gid', 'e_inst', 'e_off', 'e_rate','rec_type');

  var $list_quering= array('rec_type'   =>'Type of entry',
			   'e_rate'     =>'', // the description will be set on the fly, see "adjust_list_quering"
			   'e_inst'     =>'Institute',
			   'e_cc'       =>'Country',
			   'lm_value'   =>'Position',  
			   'lm_key'     =>'Start of position',
			   'lm_status'  =>'End of position',
			   );

  var $isMBE       = array('e_inst'     =>'Institute',
			   'e_cc'       =>'Country',
			   'lm_status'  =>'End of position');
 
  var $list_showing= array('favicon'    => ' ',
			   '_emp_period'=> ' ',
			   'lm_value'   => ' ',
			   'e_inst'     => ' ',
			   'e_cc'       => ' ');
  // The order of the blocks
  static $emp_groups   = array(emp_education  => array('30_h','34_phd','38_ms'),
			       emp_major_pos  => array('20_mp'),
			       emp_lt_visits  => array('40_va'),
			       emp_awards     => array('50_ah'),
			       emp_other      => array('22_op'),
			       );
  var $av    = Null;
  var $MODE  = 'RO';

  protected $orderBy    = 'lm_key';
  protected $orderByDir = 'DESC';
  private   $ask_for_period = Null;
  private   $dejaVu = array();


  /*
   * Check is the given person affiliate to the organization
   * @param $av_id - avatar ID
   * @param $org_id - (optional) organization ID. If missing, all the organizations are checked 
   */
  public static function isEA($av_id,$org_id=Null){
    static $reply = array();
    if (!isset($reply[$av_id])){
      $where = array("l_class='bList_eaEmpRecords'",
		     "l_parent=$av_id");
      if ($org_id) $where[] = "lm_option REGEXP '[[:<:]]e_org[[:>:]].*[[:<:]]".$org_id."[[:>:]]'";
      $q = myPear_db()->qquery("SELECT l_parent FROM zzz_lists ".
			       " LEFT JOIN zzz_avatars ON l_parent = av_id ".
			       ($org_id ? " LEFT JOIN zzz_list_members ON l_id = lm_lid " : "").
			       " WHERE ".join(' AND ',$where));
      $reply[$av_id] = (myPear_db()->num_rows($q) > 0);
    }
    return $reply[$av_id];
  }
  
  /*
   * The parent is an Avatar
   */
  private $e_org = Null;
  function __construct($parent_ID){

    $this->show_gv = True;

    //    static $hghgh=0; if ($hghgh++ < 4) b_debug::traceBack();
    $this->dbg(starting);

    // Hook the parent Avatar
    if ($parent_ID instanceof bForm_Avatar){
      $this->av = $parent_ID;
      if (bForm_Avatar_ea::$_debug) self::$_debug = True;
    }elseif((int)$parent_ID){
      $this->av = new bForm_Avatar_ea($parent_ID);
    }
    
    // Set the postponed naming 
    bForm_Avatar_ea::_init();

    // Complete the class
    parent::__construct($this->av->ID);
    $this->dbg($this->av->name(),cnf_dev);
    
    // Adjust the pop-up menu to the organization name
    foreach ($this->rec_type_menuD as $k=>$v) $this->rec_type_menu[$k] = str_replace('ORG',myOrg_name,$v);
    
    // Replace the virtual employment type by the real one
    switch(@$_POST['rec_type']){
    case '20_mp_org':
    case '22_op_org':
    case '40_va_org':
      // "Position within organization" = "Major position" + "Organization number" 
      $this->e_org = myOrg_ID;
      $this->getValue_reset('rec_type',str_replace('_org','',$_POST['rec_type']));
      $this->getValue_reset('e_org',$this->e_org);
      break;
    case '10_cp':
      // "Current position" is an endless "Major position"
      $this->getValue_reset('rec_type','20_mp');
      $this->getValue_reset('lm_status',CONST_eternity);
      break;
    default:
    }

    // Replace the position key by a text value
    if ( !empty($_POST['lm_value']) && (@$_POST['lm_lid']==$this->ID)){
      $lm_id = $this->getValue('lm_id',True,True);
      if (empty($lm_id)){
	var_dump($_POST);
	b_debug::internalError('??? empty "lm_id"');
      }
      $lm_value = myOrg()->orgEmpTypes()->code2text($_POST['lm_value'],True);
      $this->dbg(sprintf("lm_id=%s lm_value=%s=>%s",
			 $lm_id,
			 $_POST['lm_value'],
			 $lm_value),
		 cnf_dev);
      myPear_db()->qquery("UPDATE zzz_list_members SET `lm_value` = '$lm_value' WHERE lm_id=$lm_id",True);
      //      $this->updateMember(array('lm_value'=>$lm_value),$lm_id);
    }
    $this->dbg(exiting);
  }
  
  /*
   *
   */
  protected function unpack($buffer=array(), $updates=array()){
    if ($buffer = parent::unpack($buffer,$updates)){
      
      // Capitalize the position & Institute
      if (empty($buffer['e_org'])){
	foreach(array('lm_value','e_inst') as $k){
	  if (!empty($buffer[$k])) $buffer[$k] = b_fmt::ucwords($buffer[$k]); 
	}
      }else{
	$buffer['e_inst_virtual'] = myOrg($buffer['e_org'])->name(); 
	unset($buffer['e_cc']);
	unset($buffer['e_inst']);
      }
      
      if ((@$buffer['lm_status'] == 'present') ||
	  (@$buffer['lm_status'] == 'permanent') ||
	  (date('Y',(int)b_time::txt2unix($buffer['lm_status'])) > date('Y')+20)){ 
	$buffer['lm_status'] = CONST_eternity;
      }
    }
    ksort($buffer);
    return $buffer;
  }

  /*
   *
   */
  public function get_boardMembership(){
    $reply = $reply_a = array();
    // get as the regular expression
    $this->where_regexp = array('lm_value' => 'board',);
    foreach($this->getMembers(array('e_org'=>myOrg_ID)) as $r){
      $role = (self::is_current($r,$av_ddate=null)
	       ? EA_role_employee
	       : EA_role_alumnus);
      $reply_a[$role][] = $r['lm_value'];
    }
    foreach($reply_a as $role=>$positions){
      $reply[$role] = join(',',array_unique($positions));
      $this->dbg(sprintf("%s: %s",$role,$reply[$role]),True);
    }
    return $reply;
  }
  
  /*
   * Count employers by emp_type
   */
  static function countEmpByType($e_rate){
    if (empty($_SESSION[__METHOD__][$e_rate])){
      $where = array("l_class='bList_eaEmpRecords'",
		     "lm_status >= '".date('Y-m-d',time())."'",
		     "lm_key    <= '".date('Y-m-d',time())."'",
		     "lm_option REGEXP '[[:<:]]e_org[[:>:]].*[[:<:]]".myOrg_ID."[[:>:]]'",
		     "lm_option REGEXP '[[:<:]]e_rate[[:>:]].*[[:<:]]".$e_rate."[[:>:]]'",
		     );
      $_SESSION[__METHOD__][$e_rate] = myPear_db()->num_rows(myPear_db()->qquery("SELECT lm_id FROM zzz_lists ".
										 " LEFT JOIN zzz_list_members ON l_id = lm_lid ".
										 " WHERE ".join(' AND ',$where),
										 True));
      b_debug::_debug(' '.$_SESSION[__METHOD__][$e_rate],True);
    }
    return $_SESSION[__METHOD__][$e_rate];
  }
  

  /*
   *
   */
  static function is_current($r,$av_ddate=null,$verbose=False,$include_future=True){
    $reply = False;
    if (!empty($r['rec_type'])){
      $e_end = ($av_ddate // end of employment period
		? $av_ddate-30*24*3600 
		: time());
      $reply = (b_time::txt2unix($r['lm_status']) >= $e_end &&  // End of position
		($include_future
		 ? True
		 : b_time::txt2unix($r['lm_key']) <= time()) && // Start of position
		in_array($r['rec_type'],array_merge(self::$emp_groups[emp_major_pos],
						    self::$emp_groups[emp_lt_visits])));
    }
    if ($verbose)MSG::DEBUG(sprintf("%s(%s) - %s",__METHOD__,b_time::period($r['lm_key'],$r['lm_status']),var_export($reply,True)));
    return $reply;
  }

  /*
   * Extract current (if there are such) OR future (if there is no current ones)
   * records of the types "Major positions" & "Long Time Visits".
   */
  function get_currentEmploymentRecords($args=array(),$av_ddate=Null,$limit=99){
    $recs = array();
    foreach($this->getMembers($args) as $r){
      if (!self::is_current($r,$av_ddate)) continue;
      $lm_status = b_time::txt2unix($r['lm_status']);
      if ($av_ddate && ($lm_status > $av_ddate)){
	$r['lm_status'] = date('Y-m-d',min($av_ddate,$lm_status)); 
      }
      if (!empty($r['e_org'])) $r['e_cc'] = myOrg($r['e_org'])->getValue('org_country',True,True);
      $recs[$r['lm_key']] = $r;
    }

    // Remove record(s) from the future if a current one exists
    $current = $future = array();
    foreach($recs as $start=>$r){
      if ($start > date('Y-m-d',time())) $future[] = $start;
      else                               $current[]= $start;
    }
    if (!empty($current)){
      foreach($future as $date) unset($recs[$date]);
    }

    // The latest records first...
    krsort($recs);

    $reply = $debug = array();
    foreach($recs as $date=>$r){
      if (count($reply) < $limit){
	$reply[] = $r;
	$debug[] = $date;
      }
    }
    $this->dbg($debug,True);
    return $reply;
  }
  
  /*
   * Return the current emp. record item.
   * In case of multiple "current records" return a comma-separated list of items. 
   */
  function get_currentItem($what='e_inst',$args=array('e_org'=>myOrg_ID),$limit=999,$av_ddate=Null){
    $reply_a = array();
    foreach($this->get_currentEmploymentRecords($args,$av_ddate,$limit) as $r){
      if    ($v = @$r[$what])            $reply_a[] = trim($v);
      elseif($v = @$r[$what.'_virtual']) $reply_a[] = trim($v);
    } 
    $reply = join(',',array_unique($reply_a));
    $this->dbg($reply,True);
    return $reply;
  }

  /*
   * debug utility
   */
  function dump($only_ea=False){
    $this->dbg('',True);
    $a[] = $this->av->name();
    $a[] = $this->ID .' - '. $this->__toString();

    $birth = $study = array();
    if (!$only_ea){
      $birth = $this->birth_record();
      foreach(array('phd','ms') as $c){
	if ($r = $this->get_studyRecord_fromAvatar($c)) $study[] = $r;
      }
    }
    
    $l = array();
    foreach(array_merge($birth,$study,$this->getMembers()) as $r){
      // horizontal line
      if (empty($r['rec_type'])) $r['rec_type'] = '?? type';
      $a[] = sprintf("%6d: %10s - %10s %-6s %s @ %s",
		     $r['lm_id'],
		     (string)@$r['lm_key'],
		     (string)@$r['lm_status'],
		     (string)@$r['rec_type'],
		     (string)@$r['lm_value'],
		     (string)@$r['e_inst']);
      // Array as is
      ksort($r);
      $rec_type = $r['rec_type'];
      foreach(array('rec_type','l_locale','l_timestamp',
		    'lm_locale','lm_lid','lm_option',) as $f) unset($r[$f]);
      $l[] = x('td',b_debug::print_r($r,$rec_type,$doNotPrint=True));
    }
    MSG::DEBUG(x("table class='align_left b_table' ",x('tr',join('',$l))));
  }
  
  /*
   *
   */
  protected function sanity_action($w){
    $q = myPear_db()->qquery("DELETE  FROM ".$this->TABLE_member." WHERE ".join(' AND ',$w),True);
    MSG::ERROR(myPear_db()->num_rows($q).' deleted');
    // restart sanity check after deleting the record(s)
    $this->sanity();
  }
  
/*
   * This bList is writable by the EA module administrators
   */
  function isWritable(){
    return b_cnf::check_conditions(array(True => array('parent' => parent::isWritable(),
						       'admin'  => $this->isAdmin() && !cnf_inside_fb,
						       ),
					 'default'=> False),
				   $this);
  }
  
  /*
   *
   */
  function isOwner(){
    return $this->av->isOwner();
  }
  
  /*
   * Deletable ?
   */ 
  protected function isMemberDeletable(){
    $reply = (!empty($this->r['e_org']) && ($this->r['e_org'] != myOrg_ID)
	      ? False
	      : $this->isWritable());
    return $reply;
  }
  
  /*
   *
   */
  protected function isMemberEditable(){
    if ($reply = $this->isWritable()){
      $what = @$this->r['edit_what'];
      if (!empty($this->r['e_org']) && ($this->r['e_org'] != myOrg_ID)){
	$reply = False;
      }else{
	if     ($what === 'phd') $reply = $this->av->askPhD();
	elseif ($what ===  'ms') $reply = $this->av->askMS();
	elseif ((int)$what)      $reply = False; 
      }
    }
    return $reply;
  }

  /*
   *
   */
  protected function message_createList(){
    if (!cnf_CLI)  MSG::MESSAGE("Start ".$this->name(True)." for ".$this->av->name());
    MSG::MESSAGE("Creating new employment record");
  }

  /*
   *
   */
  protected function message_createMember(){
    return $this->message_addMember();
  }
  protected function message_addMember($r=array()){
    MSG::MESSAGE("New ".$this->name().(empty($r) ? "" : " for ".$this->av->name()));
  }

  /*
   *
   */
  protected function message_deleteMember($args){
    MSG::WARNING_T("Clean duplicated employment record");
  }
  

  /*
   * Remove spurious record with empty employment types
   */
  protected function getMembers_removeDuplications(&$reply){
    // Do the default cleanup
    parent::getMembers_removeDuplications($reply);
    return;

    // ?? Is this correct?
    // A newly created record fits this definition of "empty".. (?)

    // Look for empty types
    $toRemove = array();
    foreach($reply as $r_id=>$r){
      if (empty($r['rec_type']) && empty($_POST['rec_type']))  $toRemove[] = $r_id;
    }
    
    foreach($toRemove as $member_ID) {
      $this->message_deleteMember_custom = 'empty "rec_type"';
      $this->deleteMember($member_ID);
      if (cnf_dev){
	b_debug::print_r($reply[$member_ID],get_class($this)." remove empty $member_ID");
      }
      unset($reply[$member_ID]);
    }
  }
  
  /*
   *
   */
  protected function logIt(){
    if (!myPear_db()->num_rows(myPear_db()->qquery("SELECT lm_id FROM $this->TABLE_member ".$this->hookJOIN()." WHERE ".$this->hookParent(),cnf_dev))){
      MSG::INFO("Start ".$this->av->name()."'s employment history");
      myPear_logs()->add('ea',$this->av->ID);
    }
  }
  
  /*
   * Check is the list of position is trict for the record
   */
  private function strict_list_of_positions($start,$end){
    $r0 = $r1 = $r2 = $r3 = Null;
    $strict_list_of_positions = ($r0=!empty($this->e_org) && 
				 (($r1=empty($start)) || // A new record (no start date), force "strict list" 
				  ($r2=(time() >= b_time::txt2unix($start) && time() <= b_time::txt2unix($end))) ||
				  ($r3=$this->av->isE(True)))); // currently employed by the Org
    $this->dbg(sprintf("%s (r1=%s r2=%s R3=%s)",var_export($strict_list_of_positions,True),
		       var_export($r1,True),var_export($r2,True),var_export($r3,True)),
	       cnf_dev);
    return $strict_list_of_positions;
  }

  /*
   * setup list_quering
   */
  protected function adjust_list_quering(){
    static $e_rate = Null, $e_rates=Null;

    if ($e_rate === Null && $e_rates === Null){
    $this->dbg(starting);

    $this->list_quering['e_rate'] = bForm_Avatar_ea::$av2er['e_rate']['d'];
    $was = $this->list_quering;
    b_debug::print_r($was,'was this->list_quering','nosort');

    // Strict list of positions?
    $lm_id = $this->getValue('lm_id',True,'isMBE');
    $e_rate = $this->getMemberValue($lm_id,'e_rate');
    $e_rates= myOrg()->orgEmpTypes()->get_empTypes();
    $this->dbg($e_rate,cnf_dev);

    $lm_key   = $this->getMemberValue($lm_id,'lm_key');
    $lm_status= $this->getMemberValue($lm_id,'lm_status');

    if ($this->strict_list_of_positions($lm_key,$lm_status) && !$e_rate){ // No e_rate, hence dont ask for period of stay, rate first
      $this->dbg('no e_rate, hence dont query YET position & period of stay',cnf_dev);
      unset($this->list_quering['lm_value'],
	    $this->list_quering['lm_key'],
	    $this->list_quering['lm_status']
	    );
      $this->ask_for_period = False;
    }

    // Check the rest
    switch($rec_type=$this->getMemberValue(0,'rec_type')){
    case '50_ah':
      $this->list_quering['lm_value'] = 'Award / Honor';
      unset($this->list_quering['e_inst']);
    case '30_h':
    case '34_phd':
    case '38_ms':
      if (isset($this->list_quering['e_inst'])){
	// Cosmetic Al - remove "add education" if it is already there
      }
      unset($this->list_quering['lm_status']);
      $this->list_quering['lm_key'] = 'Awarded';
      break;
      
    case '10_cp':
      unset($this->list_quering['lm_status']);
    }
    $this->dbg("rec_type = $rec_type",cnf_dev);    

    // Do not ask country/institute for myOrg
    if ($this->getMemberValue(0,'e_org') || !empty($this->e_org)){
      unset($this->list_quering['e_cc']);
      unset($this->list_quering['e_inst']);
    }

    // Ask for the e_rate only if this is a major position (within organization)
    if (!EA_members()->isMember() || !in_array($rec_type,array('20_mp','40_va'))) unset($this->list_quering['e_rate']);
    if (empty($this->list_quering['e_rate'])) $this->isMBE['e_rate'] = True;

    // See the difference
    if ($was !== $this->list_quering){
      b_debug::print_r($this->list_quering,'now this->list_quering','nosort');
      $this->dbg(array_diff($was,$this->list_quering),cnf_dev);
    }
    $this->dbg(exiting);
    }
    return array($e_rate,$e_rates);
  }

  /*
   * Update the Avatar if nessesary
   * In fact only av_institute & av_position are updated 
   */
  protected function declare_action_completed($message=''){

    $this->dbg(starting);

    // If the record is "current", copy the data to AV block
    $e_start = @$_POST[bForm_Avatar_ea::$av2er['e_start']['k']];
    $e_end   = @$_POST[bForm_Avatar_ea::$av2er['e_end']['k']];
    if (empty($e_start)) $e_start = time()-2;
    if (empty($e_end))   $e_end = $e_start;
    if (b_time::inInterval(time(),time(),b_time::txt2unix($e_start),b_time::txt2unix($e_end))){
      foreach(array_intersect(array_keys(bForm_Avatar_ea::$er2av),array_keys($_POST)) as $ea){
	list($key,$value) = array(bForm_Avatar_ea::$er2av[$ea],$_POST[$ea]);
	if (myPear_db()->columnExists($key,$this->av->TABLE)){
	  $this->dbg(sprintf("ACCEPT %s => %s",$key,$value));
	  $this->av->updateDB(array($key=>$value));
	}else{
	  $this->dbg(sprintf("SKIP %s => %s",$key,$value));
	}

      }
    }

    // Apply the parental actions
    parent::declare_action_completed($message);
    $this->dbg(exiting);
  }

  /*
   * Check also that the EA vital keys (start/end of position, title) present
   */
  function getFormAdd_OK(){
    static $vital_fields = array('lm_value','lm_key','lm_status');

    $reply = parent::getFormAdd_OK();
    if ($reply && ($reply !== 'new')){ 
      foreach(array_intersect($this->list_quering,$vital_fields) as $f){
	if ($this->getValue($f)) continue;
	$this->dbg("missing '$f'",cnf_dev);
	$reply = False;
      }
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   * Input form
   */
  protected function getFormAdd(){
    // Create the record if it is empty
    $this->dbg("member_ID=".$this->member_ID,cnf_dev);
    if (!b_posix::is_int($this->member_ID)){
      $this->logIt();
      $this->member_ID = $this->createMember();
      $this->getValue_reset('lm_id',$this->member_ID);
    }
    
    //
    // For PhD & MS break the processing and redirect to the avatar's "study dialog" 
    // since those 2 blocks are stored in the zzz_avatars, and not in zzz_list_members
    $lm_id = $this->getValue('lm_id',True,'isMBE');
    static $conter_shit=0;    if (empty($lm_id)) $this->dbg((++$conter_shit).' - lm_id='.var_export($lm_id,True),cnf_dev);
    $rec_type = $this->getMemberValue($lm_id,'rec_type');
    $this->dbg("lm_id=".var_export($lm_id,True).", rec_type=".var_export($rec_type,True),cnf_dev);
    if (!empty($rec_type)){
      list($x,$degree) = explode('_',$rec_type);
      if (in_array($degree,array('phd','ms'))){
	if (!empty($lm_id)) $this->deleteMember($lm_id);
	// Break the processing and go directly to edit the Avatar class
	b_url::header(b_url::same("?from_bList=1&form=Avatar_ea&id=".$this->av->ID."&mode=RW&block_once=edu_$degree"));
      }
    }      

    //
    if (!empty($lm_id)) $this->e_org = $this->getMemberValue($lm_id,'e_org');

    // Adjust list_quering
    list($e_rate,$e_rates) = $this->adjust_list_quering();
    $this->bForm($this->list_quering,$inversed=True);

    // Rise standard bForm_List dialog for the other items
    $this->f->defineVariable('Period of stay','lm_period'); 
    $this->f->submitOnChange['rec_type'] = True;
    $this->f->isRO['lm_id'] = True;
    $this->f->formDB = $this->formDB_member;

    foreach($this->packed_items as $item) if (empty($this->list_quering[$item])) $this->f->isMBE[$item] = True;
    
    // Do not(?) check the country for the EA, the Institute name is supposed to "say everything"
    if ($this->e_org){
      $e_inst_virtual = myOrg($this->e_org)->name();
      foreach ($this->rec_type_menuD as $k=>$v){
	if (strpos($k,'_org')!==False){
	  $this->rec_type_menu[$k]                        = str_replace('ORG',$e_inst_virtual,$v);
	  $this->rec_type_menu[str_replace('_org','',$k)] = str_replace('ORG',$e_inst_virtual,$v);
	}
      }
      $this->f->defineVariable(' ','e_inst_virtual');
      $this->f->formDB['e_inst_virtual'] = $e_inst_virtual;
      $this->f->isRO['e_inst'] = True;
      $this->f->isRO['e_inst_virtual'] = True;
      $this->f->textHidden('e_org',$this->e_org);
    }
    
    // Build the form  
    foreach(array_keys($this->list_quering) as $item){
      $this->dbg("item = $item",cnf_dev);

      if (empty($rec_type) && ($item != 'rec_type')) continue;
      if (!isset($this->ask_for_period)) $this->ask_for_period = (!empty($this->list_quering['lm_status']) && ($rec_type != '10_cp'));

      switch($item){

      case 'rec_type':
	$this->f->isRO['rec_type'] = !empty($rec_type);
	$this->f->selectMenu('rec_type',$this->rec_type_menu); 
	break;  
	
      case 'e_rate':
	if (!empty($this->e_org)){
	  if ($this->strict_list_of_positions($this->getMemberValue($lm_id,'lm_key'),
					      $this->getMemberValue($lm_id,'lm_status'))){
	    if ($e_rate) $this->f->isRO['e_rate'] = True;
	    $this->f->submitOnChange['e_rate'] = True;
	    $this->f->selectMenu('e_rate',$e_rates);  
	  }else{	
	    $this->f->textHidden('e_rate','');
	  }
	}
	break;

      case 'lm_value':       
	if (in_array($rec_type,self::$emp_groups[emp_education])){
	  // ???? What is this ????
	  $this->f->textHidden($item,$this->code2text($rec_type));
	  b_debug::traceBack(__METHOD__.", strange piece of code, rec_type='$rec_type'. YB");
	}else{
	  if ($this->strict_list_of_positions($this->getMemberValue($lm_id,'lm_key'),
					      $this->getMemberValue($lm_id,'lm_status'))){
	    // Due to historical reasons we inquire position as a pop-up menu (array of key=>value), 
	    // and in the database we store the value, not the key as one would expect
	    // Hence we need the following small gymnastics to extract the key from the value (see orgEmpTypes()->text2code($title)) 
	    $lm_value  = $this->getMemberValue($lm_id,'lm_value');
	    $lm_value = myOrg()->orgEmpTypes()->text2code($lm_value);
	    $titles = myOrg()->orgEmpTypes()->code2titles($this->f->getValue('e_rate'));
	    if (empty($titles[$lm_value])) $titles[$lm_value] = $lm_value;
	    $this->f->getValue_reset($item,$lm_value);
	    $this->f->selectMenu($item,$titles);
	  }else{
	    $this->f->textField($item); 
	  }
	}
	break;
	
      case 'e_cc':       
	if (empty($this->e_org))  $this->f->textFieldCountry($item);   
	break; 
	
      case 'lm_key':    
	if ($this->ask_for_period)$this->f->datePickerField2('lm_period','lm_key','lm_status');
	else                      $this->f->datePickerField($item); 
	break;
	
      case 'lm_status': 
	if(!$this->ask_for_period)$this->f->datePickerField($item); 
	break;
	
      case 'e_inst':
	$this->f->isRO['e_inst_virtual'] = True;
	if (empty($this->e_org)) $this->f->textField('e_inst');
	else                     $this->f->textField('e_inst_virtual');
	break;
	
      default:  
	$this->f->textField($item);
      }
    }	

    // Send lm_id
    $this->f->formDB['memberID'] = $this->member_ID;

    // Close the form
    $this->f->closeForm();
    $this->dbg(sprintf('$this->MODE="%s" $this->f->MODE="%s"',$this->MODE,$this->f->MODE));
    $this->MODE = $this->f->MODE;
  }

  /*
   * Add button "go to the personal data"
   */
  protected function draw_buttons(){
    static $dejaVu = 0;
    if ($reply = parent::draw_buttons()){
      $this->dbg('MODE = '.$this->MODE,cnf_dev);
      $this->dbg('$this->av->MODE = '.$this->av->MODE,cnf_dev);
      $this->dbg('from_bForm='.var_export(b_cnf::get('from_bForm'),True),cnf_dev);
      if (($this->MODE == 'RO') || 
	  (b_cnf::get('from_bForm')) ||
	  (b_cnf::get('from_bList') && ($this->av->MODE == 'RO'))){
      	// Print a button to update the parent Avatar record
	locateAndInclude('b_tabs');
	if ($this->av->isWritable() && ((b_cnf::get('from_bForm') || !b_tabs::$inside))){
	  $reply[] = b_btn::submit('update personal data',
				   b_url::same("?from_bList=0&mode=RO&form=Avatar_ea&av_id=".$this->av->ID."&id=".$this->av->ID));
	}
	
	// Let the record to be public                                                                                                                                                                       
	if (!myOrg()->eaMembers()->isMemberAccess_public($this->av->ID,True)){
	  if ($this->av->isE(True)){
	    myOrg()->eaMembers()->setMemberAccess($this->av->ID,EA_acl_public);
	  }elseif (!$dejaVu++){
	    $make_public = 'Make this information public';
	    MSG::INFO("The information on this page is password protected, i.e. only the authorized persons might see it. ".
			 "<br/>".
			 "You can make it public by pushing the button \"$make_public\"");
	    $reply[] = b_btn::submit($make_public,
				     b_url::same("?set_access_once=".EA_acl_public."&mode=RO&form=Avatar_ea&av_id=".$this->av->ID."&id=".$this->av->ID));
	  }
	}
      }
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   *
   */
  private function code2text($code){
    switch($code){
    case '30_h':    return 'Habilitation';
    case '34_phd':  return 'Ph.D degree';
    case '38_ms':   return 'Master degree';
    }
  }

  /*
   *
   */
  function getValue($name,$asIs=True,$description_MBE=True) {
    $lm_id = parent::getValue('lm_id',True,'isMBE');
    if ($name == 'lm_id') return $lm_id;

    $value = $valueP = ((strpos($name,'lm_') === 0) || (strpos($name,'e_') === 0)
			? $this->getMemberValue($lm_id,$name)
			: parent::getValue($name));
    switch($name){
      //    case 'lm_value':
      //      $value = $this->code2text($value);
      //      break;
    case 'lm_key':
      if (!empty($value) && is_numeric($value)) $value = date('Y-m-d',$value);
      break;
    case 'e_cc':
      $value = b_cc::getCC($value);
      break;
    default:
    }
    if ($value != $valueP) $this->dbg($value,$this->show_gv);
    return $value;
  }

  /*
   * Header for the employment records listing
   */
  protected function header($arg=Null){
    return $this->list_showing;
  }

  /*
   * Page header
   */
  protected function getFormHeader(){
    MSG::H1($this->av->name());
    return Null;
  }
  
  /*
   * Print employment records grouped by the employment type (major positions, awards, etc.) 
   * Skip the "education group", it is printed separately 
   */
  protected function getMembers_forListing($arg=Null){
    $reply = array();
    foreach(self::$emp_groups as $k=>$items){
      if ($k != emp_education){ // see 'function show' in this class
	foreach($this->getMembers() as $r){
	  if (in_array(@$r['rec_type'],$items)) $reply[] = $r;
	}
      }
    }
    return $reply;
  }
  
  /*
   * Create virtual 'birth record' in the EA format
   */
  private function birth_record(){
    $y = (string)$this->av->get_date();
    $d = (string)$this->av->get_date('av_ddate');
    $b = (string)$this->av->getValue('av_birthplace',True,True);
    $c = b_cc::getName($this->av->getValue('av_birthcountry',True,True));
    
    $rec_type = '0_b';
    if (!empty($b) || !empty($c) || !empty($y)){
      if (empty($d)){
	$lm_key = 'Born:';
      }else{
	$lm_key = '&nbsp;';
	$y .= " - $d";
	if (!empty($b) || !empty($c)){
	  $b = "Born: $b";
	  $this->t->css['e_inst']['align'] = 'align-right';
	}
      }
      $r = array('lm_key'   => $lm_key,
		 'e_inst'   => x('em',$b),
		 'e_cc'     => $c,
		 'lm_value' => x('em',$y),
		 'lm_id'    => 0,
		 'rec_type'   => $rec_type);
    }else{
      $r = array('lm_key'   => '',
		 'lm_id'    => 0,
		 'rec_type'   => $rec_type);
    }
    $reply[] = $r;
    
    // do not print the header line, set it as "already printed"
    self::$emp_groups[$rec_type] = array($rec_type);
    @$this->dejaVu[$rec_type]++;
    return $reply;
  }

  /*
   * Create virtual "Study record" in the EA format
   */
  private function get_studyRecord_fromAvatar($av_type){
    static $a2e = array('phd'=>'34_phd',
			'ms' =>'38_ms');
    $r = array();
    if (!empty($a2e[$av_type])){

      if ($this->av->getValue("av_${av_type}_year",True) == CONST_year_not_considered) return array();

      if ($i=$this->av->year2date("av_${av_type}_year","M Y"))     $r['lm_key'] = $i;
      if ($i=$this->av->getValue("av_${av_type}_country",True,1))  $r['e_cc']   = $i;
      if ($i=$this->av->getValue("av_${av_type}_institute",True,1))$r['e_inst'] = $i;
      if (!empty($r)){
	$r['lm_id']   = 1;
	$r['lm_value']= $this->code2text($rec_type=$a2e[$av_type]);
	$r['rec_type']  = $rec_type;
      }
    }
    return $r;
  }
  
  /*
   *
   */
  function get_action($code,$myGet=array()){
    $reply = parent::get_action($code,array('from_bList'=>1,'from_bForm'=>0));
    $this->dbg($reply,cnf_dev);
    return $reply;
  }
  
  /*
   *
   */
  function show($arg1=''){
    $this->dbg();
    
    if(!$this->av->isEA()) b_debug::internalError("$av is not EA");
    if (b_cnf::get('from_bForm')){
      $this->dbg('from_bForm',cnf_dev);
      //      $this->av->getForm();
      parent::show();
    }else{
      bTiming()->cpu(get_class($this).__FUNCTION__);
      
      $photo = $this->av->getPhoto();
      
      ob_start();
      $this->t = new b_table_zebra($this->header());
      $this->t->repetValuesCol1only = $this->repetValuesCol1only;
      $this->t->highlightRows = empty($photo); // works bad for the photo
      $this->t->getHeader();
      
      //
      // The very first (two) line(s) is the personal information with an optional photo
      //
      foreach($this->birth_record() as $n=>$this->r){
	$this->format();
	$this->r['edit_what'] = $n;
	if (($n==0) && !empty($photo)) $this->t->pendingObject = array($photo);
	$this->t->extraTD[] = ($this->isMemberEditable()
			       ? b_btn::link('ICON_edit',b_url::same("?from_bList=1&from_bForm=0&form=Avatar_ea&id=".$this->av->ID."&mode=RW&block_once=av_birth_block"))
			       : '');
	
	$this->t->extraTD[] = '';
	$this->t->prt($this->r);
      }
      
      
      //
      // Now show the "study group"
      //
      // First Habilitation
      foreach($this->getMembers() as $this->r){
	if (!empty($this->r) && in_array(@$this->r['rec_type'],self::$emp_groups[emp_education])){
	  $this->format();
	  $this->t->extraTD[] = $this->button_editListMember();
	  $this->t->extraTD[] = $this->button_deleteListMember();
	  $this->t->extraTD[]='';
	  $this->t->prt($this->r);
	}
      }
      
      // Then PhD & MS
      foreach(array('phd','ms') as $c){
	if ($this->r = $this->get_studyRecord_fromAvatar($c)){
	  $this->format();
	  if (!empty($this->r)){ // $this->format might clean the record
	    $this->r['edit_what'] = $c;
	    $this->t->extraTD[] = ($this->isMemberEditable()
				   ? b_btn::link('ICON_edit',
						 b_url::same("?from_bList=1&from_bForm=0&&form=Avatar_ea&id=".$this->av->ID."&mode=RW&block_once=edu_$c"))
				   : '');
	    $this->t->extraTD[]='';
	    $this->t->prt($this->r);
	  }
	}
      }
      
      // Save the groups as the b_tabs prefix
      $content = ob_get_contents();
      ob_end_clean();
      
      // Now print the rest, i.e. the "true" employment records.
      // The "Who am I" &  "study" groups are saved as b_tabs->body_prefix, and will be printed first
      parent::show($content);
      
      bTiming()->cpu();
    }
  }

  /*
   * Format for the employment record 
   */
  private $highestDegree = 0;
  protected function format(){

    // Adjust the dates if the person has passed away
    if (!empty($this->r['lm_status']) &&
	(b_time::is_valid($av_ddate = $this->av->getValue('av_ddate',True,True))>0) && 
	preg_match('/^\d\d\d\d-\d\d-\d\d$/',($was=$this->r['lm_status']))){
      $this->r['lm_status'] = $now = date('Y-m-d',min(b_time::txt2unix($this->r['lm_status']),$av_ddate));
      if (cnf_dev && ($was != $now)) $this->dbg($this->av->name()." lm_status: $was->$now");
    }
    
    $this->t->showLineCounter = False;
    $this->t->class['favicon'] = 'b_transparent';

    // Find the appropriate "type of employment"
    foreach(self::$emp_groups as $current_emp_group=>$items){
      if (in_array($this->r['rec_type'],$items)) $title = $current_emp_group;
    }
    if (!@$this->dejaVu[$title]++){
      $this->t->tro();
      $this->t->th($title,"colspan=6 class='ea_btitle'");
      $this->t->trc();
    }

    // Unpack the packed data, massage a bit the record 
    switch($this->r['rec_type']){
    case '0_b':
      $this->r['_emp_period'] = $this->r['lm_key'];
      break;
    case '50_ah':
      $this->t->colspan['lm_value'] = 2;
      if (stripos($this->r['lm_value'],'nobel') !== False){
	unset($this->r['e_cc']);
	$this->r['favicon'] = bIcons()->get('i-nobel');
      }
    case '30_h':
      $this->r['_emp_period'] = date('M Y',b_time::txt2unix($this->r['lm_key']));
    case '34_phd': 
    case '38_ms':
      if (@$this->r['lm_key'] == CONST_year_not_considered) { 
	$this->r = array();
	return; 
      }
      if (!isset($this->r['_emp_period']) && isset($this->r['lm_key']))$this->r['_emp_period'] = $this->r['lm_key']; 
      if  (empty($this->r['favicon'])) if (!$this->highestDegree++)   $this->r['favicon'] = bIcons()->get('i-oxford');
      break;

    default:
      if ($this->r['lm_key'] > date('Y-m-d',time())){
	$this->r['_emp_period'] = 'from '.date('M Y',b_time::txt2unix($this->r['lm_key']));
      }elseif ($this->r['lm_status'] > date('Y-m-d',time())){
	$this->r['_emp_period'] = date('M Y',b_time::txt2unix($this->r['lm_key'])).' -> present'; 
      }else{
	$this->r['_emp_period'] = b_time::period(b_time::txt2unix($this->r['lm_key']), 
						 b_time::txt2unix($this->r['lm_status']),True);
      }
    }
    if (cnf_show_dp && !empty($this->r['_emp_period'])){
      $this->dbg($this->r['lm_key'].', '.$this->r['lm_status'].' ===>>  '.$this->r['_emp_period']);
    }

    // format the fields
    $this->r['lm_value'] = x('em',str_replace('award','',@$this->r['lm_value']));
    if (!empty($this->r['e_cc'])) $this->r['e_cc'] = b_cc::getName(@$this->r['e_cc']);

    // put the institute logo in front if there is an affiliation
    if ($e_org = @$this->r['e_org']){
      $this->r['e_cc'] = b_fmt::redText(myOrg($e_org)->name(),'bold-text italic-text');
      $this->r['favicon'] = myOrg($e_org)->getFavicon();
      $this->t->colspan['lm_value'] = 2;
    }
  }
}


