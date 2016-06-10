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
  public $tips_byMode = array('RW'=>array('lm_period' => "If you don't know the future end date of the period, type '2099-01-01'",
					  'emp_period'=> "If you don't know the future end date of the period, type '2099-01-01'",
					  ));
  var $e_type_menu = array();
  var $e_type_menuD= array('separator00'=> '',
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
  protected $packed_items = array('e_cc',
				  'e_gid',
				  'e_inst',
				  'e_off',
				  'e_org',
				  'e_rate',
				  'e_type',
				  );

  var $list_quering= array('e_type'     =>'Type of entry',
			   'e_inst'     =>'Institute',
			   'e_cc'       =>'Country',
			   'lm_value'   =>'Position',
			   'lm_key'     =>'Start of position',
			   'lm_status'  =>'End of position');

  var $isMBE       = array('e_inst'     =>'Institute',
			   'e_cc'       =>'Country',
			   'lm_status'  =>'End of position');
 
  var $list_showing= array('favicon'    => ' ',
			   'emp_period' => ' ',
			   'lm_value'   => ' ',
			   'e_inst'     => ' ',
			   'e_cc'       => ' ');
  // The order of the blocks
  var $emp_groups   = array(emp_education  => array('30_h','34_phd','38_ms'),
			    emp_major_pos  => array('20_mp'),
			    emp_lt_visits  => array('40_va'),
			    emp_awards     => array('50_ah'),
			    emp_other      => array('22_op'),
			    );
  var $av    = Null;
  var $MODE  = 'RO';

  protected $orderBy    = 'lm_key';
  protected $orderByDir = 'DESC';
  private   $dejaVu = array();

  /*
   * Check is the given person affiliated to the organization
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
   * The parent is an Avatar, the list name is
   */
  function __construct($parent_ID){
    
    // Hook the parent Avatar
    if ($parent_ID instanceof bForm_Avatar){
      $this->av = $parent_ID;
      if ($this->av->debug) $this->debug = True;
    }elseif((int)$parent_ID){
      $this->av = new bForm_Avatar_ea($parent_ID);
    }
    
    // Replace the virtual employment type by the real one
    switch(@$_POST['e_type']){
    case '20_mp_org':
    case '22_op_org':
    case '40_va_org':
      // "Position within organization" = "Major position" + "Organization number" 
      $this->getValue_reset('e_type',str_replace('_org','',$_POST['e_type']));
      $this->getValue_reset('e_org', myOrg_ID);
      break;
    case '10_cp':
      // "Current position" is an endless "Major position"
      $this->getValue_reset('e_type','20_mp');
      $this->getValue_reset('lm_status',CONST_eternity);
      //      $_POST['e_type'] = '20_mp';
      //      $_POST['lm_status'] = CONST_eternity;
      break;
    default:
    }

    
    // Adjust the pop-up menu to the organization name
    foreach ($this->e_type_menuD as $k=>$v) $this->e_type_menu[$k] = str_replace('ORG',myOrg_name,$v);
    
    // Complete the class
    parent::__construct($this->av->ID);
    $this->dbg($this->av->name());
  }
  
  /*
   *
   */
  protected function unpack($buffer=array(), $updates=array()){
    if ($buffer = parent::unpack($buffer,$updates)){

      // Capitalize the position & Institute
      foreach(array('lm_value','e_inst') as $k){
	if (!empty($buffer[$k])) $buffer[$k] = b_fmt::ucwords($buffer[$k]); 
      }
      
      if (!empty($buffer['e_org'])){
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
   * Extract active records of the types "Major positions" & "Long Time Visits".
   * The EA records are first in the list
   */
  function get_currentEmploementRecords($args=array(),$limit=99,$av_ddate=Null){
    $reply = array();
    $e_end = ($av_ddate // end of employment period
	      ? $av_ddate-30*24*3600 
	      : time());
    foreach($this->getMembers($args) as $r){
      if (empty($r['e_type']))                                     continue; 
      if (($lm_status=b_time::txt2unix($r['lm_status'])) < $e_end) continue;
      if (!in_array($r['e_type'],array_merge($this->emp_groups[emp_major_pos],
					     $this->emp_groups[emp_lt_visits]))) continue;
      if ($av_ddate && ($lm_status > $av_ddate)) $r['lm_status'] = date('Y-m-d',min($av_ddate,$lm_status)); 
      if (empty($r['e_org'])){
	$reply[] = $r;
      }else{
	$r['e_cc'] = myOrg($r['e_org'])->getValue('org_country',True,True);
	array_unshift($reply,$r);
      }
      if (count($reply) >= $limit) break;
    }
    return $reply;
  }
  
  /*
   * Return the current emp. record item.
   * In case of multiple "current records" return a comma-separated list of items. 
   */
  function get_currentItem($what='e_inst',$args=array('e_org'=>myOrg_ID),$limit=999,$av_ddate=Null){
    $reply_a = array();
    foreach($this->get_currentEmploementRecords($args,$limit,$av_ddate) as $r){
      if ($v = @$r[$what])            $reply_a[] = trim($v);
      if ($v = @$r[$what.'_virtual']) $reply_a[] = trim($v);
    } 
    $reply = join(',',array_unique($reply_a));
    $this->dbg($reply);
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
      if (empty($r['e_type'])) $r['e_type'] = '?? type';
      $a[] = sprintf("%6d: %10s - %10s %-6s %s @ %s",
		     $r['lm_id'],
		     (string)@$r['lm_key'],
		     (string)@$r['lm_status'],
		     (string)@$r['e_type'],
		     (string)@$r['lm_value'],
		     (string)@$r['e_inst']);
      // Array as is
      ksort($r);
      $e_type = $r['e_type'];
      foreach(array('e_type','l_locale','l_timestamp',
		    'lm_locale','lm_lid','lm_option',) as $f) unset($r[$f]);
      $l[] = x('td',b_debug::print_r($r,$e_type,$doNotPrint=True));
    }
    myPear::DEBUG(x("table class='align_left b_table' ",x('tr',join('',$l))));
  }
  
  /*
   *
   */
  protected function sanity_action($w){
    $q = myPear_db()->qquery("DELETE  FROM ".$this->TABLE_member." WHERE ".join(' AND ',$w),True);
    myPear::ERROR(myPear_db()->num_rows($q).' deleted');
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
    $reply = $this->isWritable();
    return $reply;
  }
  
  /*
   *
   */
  protected function isMemberEditable(){
    if ($reply = $this->isWritable()){
      $what = @$this->r['edit_what'];
      if     ($what === 'phd') $reply = $this->av->askPhD();
      elseif ($what ===  'ms') $reply = $this->av->askMS();
      elseif ((int)$what)      $reply = False; 
    }
    return $reply;
  }

  /*
   *
   */
  protected function message_createList(){
    if (!cnf_CLI)  myPear::MESSAGE("Start ".$this->name(True)." for ".$this->av->name());
  }

  /*
   *
   */
  protected function message_addMember($r){
    myPear::MESSAGE("New ".$this->name()." for ".$this->av->name());
  }

  /*
   *
   */
  protected function message_deleteMember($args){
    myPear::WARNING_T("Clean duplicated employment record");
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
      if (empty($r['e_type']) && empty($_POST['e_type']))  $toRemove[] = $r_id;
    }
    
    foreach($toRemove as $member_ID) {
      $this->message_deleteMember_custom = 'empty "e_type"';
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
    if (!myPear_db()->num_rows(myPear_db()->qquery("SELECT lm_id FROM $this->TABLE_member ".$this->hookJOIN()." WHERE ".$this->hookParent(),$this->debug))){
      myPear::INFO("Start ".$this->av->name()."'s employment history");
      myPear_logs()->add('ea',$this->av->ID);
    }
  }
  

  /*
   * setup list_quering
   */
  private function adjust_list_quering(){
    $was = $this->list_quering;
    switch($this->getMemberValue(0,'e_type')){
    case '50_ah':
      $this->list_quering['lm_value'] = 'Award / Honor';
      unset($this->list_quering['e_inst']);
    case '30_h':
    case '34_phd':
    case '38_ms':
      unset($this->list_quering['lm_status']);
      $this->list_quering['lm_key'] = 'Awarded';
      break;
      
    case '10_cp':
      unset($this->list_quering['lm_status']);
    }
    
    if ($this->getMemberValue(0,'e_org')){
      unset($this->list_quering['e_cc']);
      unset($this->list_quering['e_inst']);
    }
    if ($was !== $this->list_quering) b_debug::xxx($this->list_quering);
  }

  function getFormAdd_OK(){
    if ($reply = parent::getFormAdd_OK()){
      if (!$this->getValue('lm_key') || 
	  !$this->getValue('lm_value') || 
	  !$this->getValue('lm_status')) $reply = False;
    }
    return $reply;
  }

  /*
   * Input form
   */
  function getFormAdd(){
    // Create the record if it is empty
    if (empty($this->member_ID)){
      $this->logIt();
      $this->updateMember();
    }

    $lm_id = $this->getValue('lm_id');

    $this->adjust_list_quering();
    
    switch($e_type= $this->getMemberValue($lm_id,'e_type')){

      /*
       * Jump to the avatar study dialog for PhD / MS 
       * since those 2 blocks are stored in the zzz_avatars, and not in zzz_list_members
       */
    case '34_phd':  
    case '38_ms':
      $this->deleteMember($_POST['lm_id']);

      list($x,$degree) = explode('_',$e_type);
      // Break the processing and go directly to editing the Avatar class
      b_url::header(b_url::same("?form=Avatar_ea&id=".$this->av->ID."&mode=RW&block_once=edu_$degree"));
      break;
      
      /*
       * Rise standard bForm_List dialog for the other items
       */
    default:
      $this->bForm($this->list_quering,$inversed=True);
      $this->f->defineVariable('Period of stay','lm_period'); 
      //      $this->f->displayErrors_NO = True;
      $this->f->submitOnChange['e_type'] = True;
      $this->f->isRO['e_rate'] = True;
      $this->f->isRO['lm_id'] = True;
      $this->f->formDB = $this->formDB_member;
      
      // Do not(?) check the country for the EA, the Institute name is supposed to "say everything"
      if ($e_org = $this->getMemberValue($lm_id,'e_org')){
	$e_inst_virtual = myOrg($e_org)->name();
	foreach ($this->e_type_menuD as $k=>$v){
	  if (strpos($k,'_org')!==False){
	    $this->e_type_menu[$k]                        = str_replace('ORG',$e_inst_virtual,$v);
	    $this->e_type_menu[str_replace('_org','',$k)] = str_replace('ORG',$e_inst_virtual,$v);
	  }
	}
	$this->f->formDB['e_inst_virtual'] = $e_inst_virtual;
	$this->f->isRO['e_inst'] = True;
	$this->f->isRO['e_inst_virtual'] = True;
	$this->f->textHidden('e_org',$e_org);
      }
      
      foreach(array_keys($this->list_quering) as $item){
	if (empty($e_type) && $item != 'e_type') continue;
	$ask_for_period = (!empty($this->list_quering['lm_status']) && ($e_type != '10_cp'));
	switch($item){
	case 'e_type':
	  if (in_array($e_type,array('20_mp'))){
	    $this->f->selectMenu('e_type',array('20_mp' => $this->e_type_menu['20_mp'],
						'40_va' => $this->e_type_menu['40_va'],
						'22_op' => $this->e_type_menu['22_op']));
	  }else{
	    if (!($this->f->isRO['e_type'] = !empty($e_type)))  $this->f->keepRW[] = 'e_type';
	    $this->f->selectMenu('e_type',$this->e_type_menu); 
	  }
	  break;  
	  
	case 'lm_value':       
	  if (in_array($e_type,$this->emp_groups[emp_education])) $this->f->textHidden($item,$this->code2text($e_type));
	  else                                                    $this->f->textField($item); 
	  break;
	  
	case 'e_cc':       
	  if (empty($e_org))       $this->f->textFieldCountry($item);   
	  break; 
	  
	case 'lm_key':    
	  if ($ask_for_period)     $this->f->datePickerField2('lm_period','lm_key','lm_status');
	  else                     $this->f->datePickerField($item); 
	  break;
	  
	case 'lm_status': 
	  if(!$ask_for_period)     $this->f->datePickerField($item); 
	  break;
	  
	case 'e_inst':
	  $this->f->isRO['e_inst_virtual'] = True;
	  if (empty($e_org))       $this->f->textField('e_inst');
	  else                     $this->f->textField('e_inst_virtual');
	  break;
	
	default:  
	  $this->f->textField($item);
	}
      }	
      if (cnf_dev){
	$this->f->isRO['memberID'] = True;
	$this->f->formDB['memberID'] = $this->member_ID;
	$this->f->textField('memberID');
      }
      $this->f->closeForm();
      $this->MODE = $this->f->MODE;
    }
  }

  /*
   * Add button "go to the personal data"
   */
  protected function draw_buttons(){
    static $dejaVu = 0;
    if ($reply = parent::draw_buttons()){
      if ($this->MODE == 'RO'){
	// Print a button to update the parent Avatar record
	locateAndInclude('b_tabs');
	if ($this->av->isWritable() && ! b_tabs::$inside){
	  $reply[] = b_btn::submit('update personal data',
				   b_url::same("?see_myinfo=yes&mode=RO&form=Avatar_ea&av_id=".$this->av->ID."&id=".$this->av->ID));
	}
	
	// Let the record to be public                                                                                                                                                                       
	if (!myOrg()->eaMembers()->isMemberAccess_public($this->av->ID,True)){
	  if ($this->av->isE(True)){
	    myOrg()->eaMembers()->setMemberAccess($this->av->ID,EA_acl_public);
	  }elseif (!$dejaVu++){
	    $msg = 'Make this information public';
	    myPear::INFO("The information on this page is password protected, i.e. only the authorized persons might see it. ".
			 "<br/>".
			 "You can make it public by pushing the button \"$msg\"");
	    $reply[] = b_btn::submit($msg,
				     b_url::same("?set_access_once=".EA_acl_public."&mode=RO&form=Avatar_ea&av_id=".$this->av->ID."&id=".$this->av->ID));
	  }
	}
      }
    }
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
    $lm_id = parent::getValue('lm_id');
    if ($name == 'lm_id') return $lm_id;

    $value = $valueP = ((strpos($name,'lm_') === 0) || (strpos($name,'e_') === 0)
			? $this->getMemberValue($lm_id,$name)
			: parent::getValue($name));
    switch($name){
    case 'lm_key':
      if (!empty($value) && is_numeric($value)) $value = date('Y-m-d',$value);
      break;
    case 'e_cc':
      $value = b_cc::getCC($value);
      break;
    default:
    }
    if ($value != $valueP) $this->dbg($value,(bool)($this->debug||$this->show_gv));
    return $value;
  }

  /*
   * Header for the employment records listing
   */
  protected function header(){
    return $this->list_showing;
  }

  /*
   * Page header
   */
  protected function getFormHeader(){
    myPear::H1($this->av->name());
    return Null;
  }
  
  /*
   * Print employment records grouped by the employment type (major positions, awards, etc.) 
   * Skip the "education group", it is printed separately 
   */
  protected function getMembers_forListing($arg=Null){
    $reply = array();
    foreach($this->emp_groups as $k=>$items){
      if ($k != emp_education){ // see 'function show' in this class
	foreach($this->getMembers() as $r){
	  if (in_array(@$r['e_type'],$items)) $reply[] = $r;
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
    
    $e_type = '0_b';
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
		 'e_type'   => $e_type);
    }else{
      $r = array('lm_key'   => '',
		 'lm_id'    => 0,
		 'e_type'   => $e_type);
    }
    $reply[] = $r;
    
    // do not print the header line, set it as "already printed"
    $this->emp_groups[$e_type] = array($e_type);
   @$this->dejaVu[$e_type]++;
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
      if ($i=$this->av->year2date("av_${av_type}_year","M Y"))     $r['lm_key'] = $i;
      if ($i=$this->av->getValue("av_${av_type}_country",True,1))  $r['e_cc']   = $i;
      if ($i=$this->av->getValue("av_${av_type}_institute",True,1))$r['e_inst'] = $i;
      if (!empty($r)){
	$r['lm_id']   = 1;
	$r['lm_value']= $this->code2text($e_type=$a2e[$av_type]);
	$r['e_type']  = $e_type;
      }
    }
    return $r;
  }

  /*
   *
   */
  function show($arg1=''){
    $this->dbg();
    locateAndInclude('b_tabs');

    if(!$this->av->isEA()) b_debug::internalError("$av is not EA");
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
			     ? b_btn::link('ICON_edit',
					   b_url::same("?form=Avatar_ea&id=".$this->av->ID."&mode=RW&block_once=av_birth_block"))
			     : '');
      
      $this->t->extraTD[] = '';
      $this->t->prt($this->r);
    }
    
    
    //
    // Now show the "study group"
    //
    // First Habilitation
    foreach($this->getMembers() as $this->r){
      if (!empty($this->r)){
	if (in_array(@$this->r['e_type'],$this->emp_groups[emp_education])){
	  $this->format();
	  $this->t->prt($this->r);
	}
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
					       b_url::same("?form=Avatar_ea&id=".$this->av->ID."&mode=RW&block_once=edu_$c"))
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
      if (cnf_dev && ($was != $now)) b_debug::xxx($this->av->name()." lm_status: $was->$now");
    }
    
    $this->t->showLineCounter = False;
    $this->t->class['favicon'] = 'b_transparent';

    // Find the appropriate "type of employment"
    foreach($this->emp_groups as $current_emp_group=>$items){
      if (in_array($this->r['e_type'],$items)) $title = $current_emp_group;
    }
    if (!@$this->dejaVu[$title]++){
      $this->t->tro();
      $this->t->th($title,"colspan=6 class='ea_btitle'");
      $this->t->trc();
    }

    // Unpack the packed data, massage a bit the record 
    switch($this->r['e_type']){
    case '0_b':
      $this->r['emp_period'] = $this->r['lm_key'];
      break;
    case '50_ah':
      $this->t->colspan['lm_value'] = 2;
      if (stripos($this->r['lm_value'],'nobel') !== False){
	unset($this->r['e_cc']);
	$this->r['favicon'] = bIcons()->get('i-nobel');
      }
    case '30_h':
      $this->r['emp_period'] = date('M Y',b_time::txt2unix($this->r['lm_key']));
    case '34_phd': 
    case '38_ms':
      if (@$this->r['lm_key'] == CONST_year_not_considered) { 
	$this->r = array();
	return; 
      }
      if (!isset($this->r['emp_period']) && isset($this->r['lm_key']))$this->r['emp_period'] = $this->r['lm_key']; 
      if  (empty($this->r['favicon'])) if (!$this->highestDegree++)   $this->r['favicon'] = bIcons()->get('i-oxford');
      break;

    default:
      if ($this->r['lm_key'] > date('Y-m-d',time())){
	$this->r['emp_period'] = 'from '.date('M Y',b_time::txt2unix($this->r['lm_key']));
      }elseif ($this->r['lm_status'] > date('Y-m-d',time())){
	$this->r['emp_period'] = date('M Y',b_time::txt2unix($this->r['lm_key'])).' -> present'; 
      }else{
	$this->r['emp_period'] = b_time::period(b_time::txt2unix($this->r['lm_key']), 
						b_time::txt2unix($this->r['lm_status']),True);
      }
    }
    if (cnf_show_dp && !empty($this->r['emp_period'])){
      b_debug::xxx($this->r['lm_key'].', '.$this->r['lm_status'].' ===>>  '.$this->r['emp_period']);
    }

    // format the fields
    $this->r['lm_value'] = x('em',str_replace('award','',@$this->r['lm_value']));
    if (!empty($this->r['e_cc'])) $this->r['e_cc'] = b_cc::getName(@$this->r['e_cc']);

    // put the institute logo in front if there is an affiliation
    if ($e_org = @$this->r['e_org']){
      $this->r['e_cc'] = b_fmt::redText(myOrg($e_org)->name(),'boldText italicText');
      $this->r['favicon'] = myOrg($e_org)->getFavicon();
      $this->t->colspan['lm_value'] = 2;
    }
  }
}


