<?php
/*
 *
 * Class to keep the reimbursement rates - travel, subsistence, etc.
 *
 * The "reimbursementRates" parent might be either Event or Organization.
 *
 * The very first "reimbursementRates" gets the default values from the config.inc,
 * the consequet classes for Events get the default values from the Organization class.
 *
 * This lets to modify the default rates without changing the sourse code.
 */
locateAndInclude('bList_vm');
class bList_vm_reimbursementRates extends bList_vm{

  var $list_quering = array('lm_key'  =>'What',
			    'lm_value'=> '',
                            's_rate'  =>'max rate');
  var $list_showing = array('_lm_key'  => '',
			    '_lm_value'=> '',
			    '_s_rate'  =>'Max rate',
			    '_s_plc'   =>'Affected policy');
  protected $packed_items = array('s_cc','s_plc','s_rate',); // countries list & policy
  protected $isReady = True;
  protected $orderBy ='lm_key';
  protected $agenda  = Null;
  public    $default_source = 'Nordita';
  private   $e_id = Null;

  /*
   * The class parent might be either Event or Organization
   */
  function __construct($parent){

    parent::__construct($parent);
    
    // Workaround - older php versions can't have string concatination in "static"    
    $this->list_quering['s_rate'] .= ' ('.trim(CONST_currency).')';

    // Initialise if needed
    if ( @$_GET['drop_list_once'] == $this->ID) bList::deleteList($this->ID,True);
    if ((@$_GET['init_list_once'] === 'yes') || $this->parent_ID == myOrg_ID) $this->init();
    
    // Set flag "Recalculate the travel estimate if the rates are changed" 
    if ($this->myPost_bList()) $this->set_flagToBeUpdated();

    // Sanity
    if ($this->parent_ID != myOrg_ID) $this->list_showing['_s_plc'] = 'Yes / No';
  }

  /*
   *
   */
  private function init(){
    
    if ($this->getMembers()) return;
    if ($this->parent_ID == myOrg_ID){ 
      $this->dbg("Initialise ".myOrg_name);
      // The Organizational Rates are not yet defined...	  

      // Initially set default subsistence rate as zero
      // This might be changed later by the administrator
      foreach(VM_reimbursable_visits() as $v_type){
	foreach(VM::$reimbursements as $s_plc=>$d){
	  if (@$d['t'] === auto_maintained){
	    myPear::MESSAGE("Skip auto-maintained ".x('em',$d['d']));
	  }else{
	    $this->addMember(array('lm_status'=>'ok',
				   'lm_key'   => $d['d'],
				   'lm_value' => $v_type,
				   's_rate'   =>0,
				   's_plc'    =>$s_plc));
	  }
	}
      }
      
      // set trip rates
      foreach(VM::$travel_zones as $zone=>$a) $this->addMember(array('lm_status'=>'ok',
								     'lm_key'  =>'travel',
								     'lm_value'=>$zone,
								     's_rate'  =>$a['p'],
								     's_cc'    =>join(',',$a['n'])));
      $this->set_flagToBeUpdated();
    }else{
      // The event rates are missing, copy them from the Organization
      // However, the rates are not created for the Events without the budget code.
      // (the budget code might be missing for the "out of scope" events or when it was not yet assigned)
      //
      if (!$this->e->budgetSource()->get_budgetSource()){
	myPear::ERROR("!! No budget for \"".$this->e->name()."\"");
      }else{
	foreach(bList::getListInstance(myOrg_ID,get_class($this))->getMembers() as $lm_id=>$r){
	  if (in_array($r['lm_value'],array(VISIT_TYPE_COLLABORATION,
					    VISIT_TYPE_OTHER,
					    VM::$reimbursable_visits[VISIT_TYPE_COLLABORATION]['d'],
					    VM::$reimbursable_visits[VISIT_TYPE_OTHER]['d'],
					    ))) continue;
	  $this->addMember(array('lm_status'=>'ok',
				 'lm_key'   =>@$r['lm_key'],
				 'lm_value' =>@$r['lm_value'],
				 's_rate'   =>@$r['s_rate'],
				 's_plc'    =>@$r['s_plc'],
				 's_cc'     =>@$r['s_cc']));
	}
	$this->set_flagToBeUpdated();
      }
      // Process this newly created bList
      self::process_updates();
    }
    $this->cache_bList_getMembers = array();
  }
  
  /*
   *
   */
  private function set_flagToBeUpdated(){
      if (empty($_SESSION['bList_vm_reimbursementRates_rates_updated'])) $_SESSION['bList_vm_reimbursementRates_rates_updated'] = array();
      $_SESSION['bList_vm_reimbursementRates_rates_updated'][$this->parent_ID] = True;
  }
  
  /*
   * Update the rate estimates after changing the event rates
   */
  static function process_updates($args=array()){

    if (empty($_SESSION['bList_vm_reimbursementRates_rates_updated'])) return;
    
    // recalculate travel/subsistence reimbursement estimates
    foreach(array_keys($_SESSION['bList_vm_reimbursementRates_rates_updated']) as $parent_ID){
      b_debug::xxx("parent $parent_ID");
      locateAndInclude('bForm_vm_Expenses');
      bForm_vm_Expenses::_updateEstimates($parent_ID);                                                                                                                           
      
      
      //
      // Integrate the change of reimbursement policy into Event and/or Ogranization
      //
      if ($parent_ID == myOrg_ID){
	$e = Null;
	$reimbursementRates = bList::getListInstance(myOrg_ID,'bList_vm_reimbursementRates');
	$visit_types = VM_reimbursable_visits();
      }else{
	$e = myPear::getInstance_new('bForm_vm_Event',$parent_ID,'fatal');
	$reimbursementRates = $e->reimbursementRates();
	$visit_types = array(VISIT_TYPE_PROGRAM);
      }
      
      // look for updates
      foreach($visit_types as $type){
	$was_p = $now_p = array();
	$now = $was = VM_visit_policies($type,$e);
	foreach($reimbursementRates->get_policies($type) as $policy=>$is_ON){
	  if ($is_ON && !in_array($policy,$was)) $now[] = $policy;
	  if(!$is_ON &&  in_array($policy,$was)) $now = array_diff($now,array($policy));
	}
	
	foreach($was as $p) $was_p[] = x('li',VM::$v_policies[$p]['d']);
	b_debug::xxx(strip_tags(join(', ',$was_p)));
	
	// Are there any changes?
	if ($was !== $now){
	  
	  // Save the updated policies in the database
	  sort($now);  // kill association
	  VM_visit_policies($type,$now,$e);
	  
	  // Print a message
	  foreach($now as $p) $now_p[] = x('li',VM::$v_policies[$p]['d']);
	  
	  if ($add = array_diff($now_p,$was_p)) $changes[] = "Added:".x('ul',  join("\n",$add));
	  if ($rem = array_diff($was_p,$now_p)) $changes[] = "Removed:".x('ul',join("\n",$rem));
	  myPear::INFO($changes,VM::$description[$type]['d'].' visits policy is changed for '.$reimbursementRates->title);
	}
      }
    }
    unset($_SESSION['bList_vm_reimbursementRates_rates_updated']);
  }

  /*
   *
   */
  protected function message_addMember($r){
    if (cnf_dev) return parent::message_addMember($r);
  }
  
  /*
   *
   */
  protected function message_addMember_name($r){
    return $r['lm_key'];
  }
  
  /*
   *
   */
  protected function message_createList(){
    if ($this->name(True)  == 'list Null') b_debug::internalError('list Null');
    myPear::MESSAGE("Creating ".$this->name(True).' for '.$this->title);
  }

  /*
   *
   */
  protected function draw_buttons(){
    $reply = array();
    if ($this->isWritable()){
      if (!$this->getMembers()){
	$reply[] = b_btn::submit('initialize reimbursment rates for '.
				 ($this->parent_ID == myOrg_ID ? myOrg_name : 'your conference'),b_url::same("?init_list_once=yes&resetcache_once=1"));
      }else{
	$reply[] = b_btn::submit('wipe all rates',b_url::same("?&drop_list_once=".$this->ID));
	if ($this->parent_ID!=myOrg_ID) $reply[] = b_btn::submit('back to the budget estimation',b_url::same("?tab_force=tab_vm_ogz_budget"));
      }
    }
    return array_merge($reply,$this->extra_buttons);
  }
		
  /*
   *
   */    
  function get_policies($v_type){
    bTiming()->cpu(__FUNCTION__);
    $reply = array();
    foreach($this->getMembers() as $lm_id=>$r){
      if ($s_plc = @$r['s_plc']) $reply[$s_plc] = ((int)$r['s_rate'] > 0) || bForm::_checkBoxIsOn('s_rate',$r);
    }
    $this->dbg($reply,cnf_dev);
    bTiming()->cpu();
    return $reply;
  }

  /*
   * @param return array(subsistence,trip_reimbursement)
   */
  public $tripCostFromResidentshipCountry = array();
  function get_rates($rec){

    // Travel estimate
    if (!empty($rec['av_residentship'])){
      $this->tripCostFromResidentshipCountry = $this->get_tripCostForCountry($rec['av_residentship']);
    }elseif (bForm_vm_Visit::_getPolicy(VM_V_payTrip,$rec)){
      $this->tripCostFromResidentshipCountry = array('country_name' => 'Unknown country',
						     'zone'         => 'Unknown country',
						     'pay'          => '');
      // myPear::ERROR("Missing residentship for ".bForm_Avatar::_fmtName('fL',$rec).", can't estimate the travel cost.");
    }
    
    $reply = empty($rec['v_type'])
      ? array(0,0)
      : array(VM_wordPerdiem=>(int) $this->get_allowedPerdiem($rec['v_type']),
	      'trip'        =>(int)@$this->tripCostFromResidentshipCountry['pay']);
    return array_values($reply);
  }
  
  /*
   *
   */  
  private function get_allowedPerdiem($v_type){
    $reply = $this->getMemberValue(array('lm_key'  =>VM_wordPerdiem,
					 'lm_value'=>$v_type),'s_rate');
    $this->dbg($reply);
    return $reply;
  }

  /*
   * @param 
   */
  function get_tripCostForCountry($cc){
    static $nowhere = array('country_name' => 'Unknown country',
			    'zone'         => 'Unknown region',
			    'pay'          => '');
    static $members = Null;
    if ($members === Null){
      $this->init();
      $members = $this->getMembers(array('lm_key'=>'travel'));
    }

    $reply = $nowhere;
    if (!empty($cc)){
      foreach(array(1,2) as $pass){
	foreach ($members as $id=>$r){
	  $code = ($pass == 1
		   ? $cc
		   : b_cc::getContinent($cc));
	  $countries = explode(',',$r['s_cc']);
	  if (in_array($code,$countries)){
	    if (empty($r['s_rate'])) myPear::WARNING_T("Missing rates for $cc / ".$r['lm_value']);
	    $reply = array('country_name' => b_cc::getName($cc),
			   'zone'         => $r['lm_value'],
			   'pay'          => $r['s_rate']);
	    break 2;
	  }
	}
      }
      if ($reply === $nowhere) myPear::WARNING_T("Unknown country name/code \"$cc\"");
    }      
    return $reply;
  }
  
  /*
   *
   */
  function isWritable(){
    $reply = superUser_here || (VM_manager_here && ($this->parent_ID==myOrg_ID)) || $this->e->isOrganizer();
    return $reply;
  }

  /*
   * Possible to add a new member?
   */
  function isAddable(){
    return VM_manager_here;
  }

  protected function isMemberDeletable(){
    return False;
  }

  protected function isMemberEditable(){
    if ($reply = parent::isMemberEditable()){
      $v_type = $this->getMemberValue($this->r['lm_id'],'lm_value');
      if (in_array($v_type,VM_reimbursable_visits())){
	$s_plc = $this->getMemberValue($this->r['lm_id'],'s_plc');
	switch($c=@VM::$reimbursable_visits[$v_type][$s_plc]){
	case 'yes':
	case 'maybe':
	  $reply = True;
	  break;
	case 'no':
	  $reply = False;
	}
      }
    }
    $this->dbg($reply,True);
    return $reply;
  }
  
  protected function getFormAdd(){
    //
    //    self::process_updates();

    $lm_id = $this->getValue('lm_id');
    $args = array('lm_key'  => ' ',
		  'lm_value'=> ' ',//Trip distance',
		  's_rate'  => $this->list_quering['s_rate'],
		  's_cc'    => 'countries/continents',
		  's_plc'   => '');

    $s_plc = $this->getMemberValue($lm_id,'s_plc');
    if (!$this->getMemberValue($lm_id,'lm_value')) unset($args['lm_value']);
   
    $this->bForm($args,True);
    if ($checkbox = in_array($s_plc,VM::$reimbursable_visits['checkbox'])){
      $this->f->defineVariable(array('s_rate'=>'Yes? / No?'),True);
    }
    $this->f->isRO['lm_key']= True;
    $this->f->isRO['lm_value']= True;
    $this->f->isRO['s_cc']  = !VM_manager_here;
    $this->f->textField('lm_key');
    if ($d = @VM::$reimbursable_visits[$this->getMemberValue($lm_id,'lm_value')]['d']){
      $this->f->formDB['lm_value'] = $d;
    }
    $this->f->textField('lm_value');
    if ($this->getMemberValue($lm_id,'lm_key') == 'travel') $this->f->textField('s_cc'); 
    if ($checkbox) $this->f->checkbox('s_rate');
    else           $this->f->textField('s_rate');
    $this->f->closeForm();
  }

  function getFormAdd_OK(){
    $reply = (@$_GET['act_code'] !== 'edit');
    if ($this->myPost_bList()){
      //      if (isset($_POST['s_rate']) && empty($_POST['s_rate'])) $_POST['s_rate'] = 'no';
      //      if (@$_POST['s_rate'] === 'on' ) $_POST['s_rate'] = 1;
      $reply = parent::getFormAdd_OK();
      
      if ($_POST['lm_key'] == VM_wordPerdiem){
	if (@$_POST['s_rate']>VM_maxPerdiemAmount){
	  $reply = False;
	  myPear::ERROR("Per Diem can't be more then ".VM_maxPerdiemAmount.CONST_currency);
	}elseif(@$_POST['s_rate'] <= 0){
	  $reply = True;
	  $_POST['s_rate'] = 0;
	} 
      }
    }
    $this->dbg($reply);
    return $reply;
  }
  
  protected function format(){
    static $lm_key_was = '?';

    $forOrg = ($this->parent_ID == myOrg_ID);

    $this->t->showLineCounter = False;
    //    foreach(preg_grep('/^(l_|lm_option)/',array_keys($this->r)) as $l)unset($this->r[$l]);    b_debug::print_r($this->r);

    // Visit type / Trip distance
    if ( $this->r['lm_value'] == VISIT_TYPE_PROGRAM && !$forOrg) $this->r['_lm_value'] = '';
    else $this->r['_lm_value'] = (($d = @VM::$reimbursable_visits[$this->r['lm_value']]['d'])
				  ? $d
				  : trim($this->r['lm_value'] .' '.@$r['s_cc']));
    
    // Rate
    $this->r['_s_rate'] = ((int)@$this->r['s_rate'] <= 0
			   ? ''
			   : x('strong',b_fmt::money($this->r['s_rate'])));
    // reimbursement
    $this->r['_lm_key'] = ($this->r['lm_key'] == $lm_key_was
			   ? ''
			   : x('strong',ucfirst($this->r['lm_key'])));

    // Policy (yes / no)
    $this->r['_s_plc'] = (empty($this->r['s_plc'])
			  ? ''
			  : (bIcons()->get(((int)$this->r['s_rate']>0) || bForm::_checkBoxIsOn('s_rate',$this->r) 
					   ? 'i-finger_up-small' 
					   : 'i-finger_down-small').($forOrg?VM::$v_policies[$this->r['s_plc']]['d']:'')));
    //
    if ($this->r['lm_key'] != $lm_key_was) print '<tr><td></td></tr>';
    $lm_key_was = $this->r['lm_key'];
  }
}
