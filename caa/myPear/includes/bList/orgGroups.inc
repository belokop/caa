<?php
locateAndInclude('bList');

class bList_orgGroups extends bList{
  
  var $debug = False;
  protected $packed_items = array('g_name');
  var $list_quering = array('lm_key'  =>'Code of group/department',
			    'g_name'  =>'Name');
  var $list_showing = array('_lm_key'  =>'Code of group/department',
			    'g_name'  =>'Name',
			    '_n'      =>' ');
  
  protected $isReady  = True;

  // The "administration" group is always there. Fill free to assign the name
  //  private $compalsary_groups = array(EA_group_adm);
  private $compalsary_groups = array();

  /*
   * parent is the current organization
   */
  function __construct($org_id=Null){
    if (empty($org_id)) $org_id = myOrg_ID;
    parent::__construct($org_id);

    // Make sure that the default groups are there
    foreach($this->compalsary_groups as $group) $this->addMember(array('lm_key'=>$group));
  }

  function dump($r=Null){
    if (empty($r))  $r = $this->formDB_member;
    return sprintf("%10s - %10s %s, %s",
		   $r['lm_key'],
		   $r['g_name']);
  }

  /*
   * EA is an optional module
   */
  function isManager(){
    myPear_access(EA_MODULE);
    return EA_ORG_manager_here;
  }

  /*
   *
   */
  protected function isMemberDeletable(){
    static $noname = 'no group';
    $nMembers = count($this->get_groupMembers($this->r['lm_key'],True));
    if (empty($this->r['g_name'])) $this->r['g_name'] = $noname;
    $compalsary_groups = in_array($this->r['lm_key'],$this->compalsary_groups);
    //    if ($nMembers && $this->isWritable()) myPear::MESSAGE("$nMembers members in \"".$this->r['g_name']."\"");
    return b_cnf::check_conditions(array(True =>array('empty' => empty($this->r['lm_key'])),      
					 False=>array('ROx'   => $compalsary_groups,
						      'RO'    => !$this->isWritable(),
						      '!empty'=>($nMembers > 0)),
					 'default' => True),
				   $this);
  }
  
  protected function isMemberEditable(){
    return $this->isWritable();
  }

  /*
   * Check the sanity
   */
  function getFormAdd_OK(){
    $reply = parent::getFormAdd_OK();
    $lm_id = $this->getValue('lm_id');
    $code = $this->getMemberValue($lm_id,'lm_key');
    $this->get_groups();
    //b_debug::print_r($this->formDB_member);
    if (($id = @$this->all_groups_IDs[$code]) && ($id != @$this->formDB_member['lm_id'])) {
      $msg = "Group code \"$code\" is already used. Please chouse another one";
      if (is_object($this->f)) $this->f->errorsInTheForm['lm_key'] = $msg;
      $this->dbg($msg);
      $reply = False;
    }
    return $reply;
  }

  /*
   * Input form
   */
  protected function getFormAdd(){
    myPear::H1('Updating groups/departments');

    // Create the record if it is empty
    if (empty($this->member_ID))      $this->updateMember();
    
    $this->bForm($this->list_quering,$inverted=True);
    if ((@$_GET['act_once'] || $this->getFormAdd_OK()) && $this->f->getValue('lm_key',True)) $this->f->isRO['lm_key'] = True;
    $this->f->isRO['lm_value'] = True;  // not used for the time being
    foreach($this->list_quering as $k=>$v){
      $this->f->textField($k);
    }
    $this->f->closeForm();
  }

  /*
   * Get the array of org_units
   * @param $reply = array(code=>title)
   */
  private $all_groups = Null;
  private $all_groups_IDs = Null;
  function get_groups($key=Null){
    if ($this->all_groups === Null){
      $this->all_groups_IDs = $this->all_groups = array();
      foreach($this->getMembers() as $lm_id=>$r){
	if (empty($r['g_name']) && !empty($r['lm_key'])){
	  $this->updateMember(array('g_name'=>($r['g_name'] = $r['lm_key'])),$r['lm_id']);
	}
	$this->all_groups[$r['lm_key']] = @$r['g_name'];
	$this->all_groups_IDs[$r['lm_key']] = $r['lm_id'];
      }
      ksort($this->all_groups);
    }
    $reply = (empty($key)
	      ? $this->all_groups
	      : (empty($this->all_groups[$key]) ? 'unknown' : $this->all_groups[$key]));
    $this->dbg($reply,True);
    return $reply;
  }
  
  /*
   * Get list of department members
   */
  public $groupMembers = array();
  function get_groupMembers($org_unit=Null,$for_HTML=False){
    locateAndInclude('bList_eaEmpRecords');
    static $unk = '??';
    if (empty($org_unit)) $org_unit = $unk;

    // How to show the avatar with errors - as modal or not?
    // Someone hates the modal mode
    $modal = !$this->isWritable();;
    
    // Save in cache the membership
    if(@$this->groupMembers[(int)$for_HTML] === Null){
      bTiming()->cpu(__FUNCTION__);
      $this->groupMembers[(int)$for_HTML] = array();
      $this->get_groups();
      
      // Scan all the emploeers records to check for the departments
      foreach(myOrg($this->parent_ID)->eaMembers()->getMembers_byStatus(array(EA_role_employee,
									      EA_role_lt_visitor,
									      )) as $av_id=>$r){
	if (isset($r['av_lastname'])) $name = bForm_Avatar::_fmtName('Lf',$r);
      	else                          $name = $r['sfl'];
	$url  = b_url::same(array('form' => 'bForm_Avatar_ea',
				  'id'   => $av_id,
				  'mode' =>'RW',
				  'block_once'=>'block_researchGroup'),
			    b_url::get_GET('^(act_|ed_bList)',True));
	
	$not_defined = True;
        $EmpRec_class = bList::getListInstance($av_id,'bList_eaEmpRecords');
	foreach($EmpRec_class->getMembers() as $er_id=>$er){
	  $groups = (empty($er['e_gid'])
		     ? array()
		     : explode(',',$er['e_gid']));
	  if (!$for_HTML && !in_array($org_unit,$groups)) continue;

	  foreach($groups as $gid){
	    if (in_array($gid,array_keys($this->all_groups))){
	      $this->groupMembers[(int)$for_HTML][$gid][$av_id] = $name;
	    }elseif($for_HTML){
	      $this->groupMembers[(int)$for_HTML][$unk][$av_id] = $name;
	      if ($this->isWritable()){
		$msg = "Unknown group \"$gid\" for ";
		// Modal or not? Some people hate modal windows...
		if($modal)myPear::WARNING($msg . bJS()->modal_iframe($url," [ $name ]"));
		else      myPear::WARNING($msg . b_btn::link($name,b_url::same($url)));
	      }
	    }
	    $not_defined = False;
	  }
	}
	if ($for_HTML && $not_defined){
	  $this->groupMembers[(int)$for_HTML][$unk][$av_id] = $name;
	  if ($this->isWritable()){
	    $msg = "Group is not set for ";
	    if($modal)myPear::WARNING($msg . bJS()->modal_iframe($url," [ $name ]"));
	    else      myPear::WARNING($msg . b_btn::link($name,b_url::same($url)));
	  }
	}
      }
      ksort($this->groupMembers[(int)$for_HTML]);
      bTiming()->cpu();
    }

    if (empty($this->groupMembers[(int)$for_HTML][$org_unit])) $this->groupMembers[(int)$for_HTML][$org_unit] = array();
    $reply = $this->groupMembers[(int)$for_HTML][$org_unit];
    return $reply;
  }

  /*
   * Writable by the EA managers
   */  
  private $isWritable = Null;
  function isWritable(){
    if ($this->isWritable === Null) $this->isWritable = (bool)b_cnf::check_conditions(array(False=>(array('not EA' => (b_reg::$current_module != EA_MODULE))),
											    'default' => parent::isWritable()),
										      $this);
    return $this->isWritable;
  }

  /*
   * Header for the department records listing
   */
  protected function header($arg=Null){
    return $this->list_showing;
  }


  /*
   * Formatting for the department record listing
   */
  protected function format(){
    parent::format();
    $this->r['_n'] = (($nMembers = count($this->get_groupMembers($this->r['lm_key'],True))) > 0
		      ? "$nMembers members"
		      : "");
    $this->r['_lm_key'] = x('strong',$this->r['lm_key']);
    unset($this->r['lm_value']);
  }

  /*
   * Listing of orgGroup members as b_tab
   */
  static function b_tab_groupMembers($org_id){
    
    // Open the tabs
    locateAndInclude('b_tabs');
    $b_tabs = new b_tabs(array_values(myOrg($org_id)->orgGroups()->get_groups()));
    
    // Fill the tab
    locateAndInclude('bHolder_Avatar_ea');
    foreach(myOrg($org_id)->orgGroups()->get_groups() as $group_code=>$group_name){
      if ($b_tabs->body()){
	// List the group members, but instead of SQL query
	// provide the list of av_id to be listed
	$holder = new bHolder_Avatar_ea();
	$holder->show_icons = superUser_here || (defined('EA_ORG_manager_here') && EA_ORG_manager_here);
	$holder->custom_header = array('icon_nosort_afirst' => ' ',
				       'name'               => 'Name',
				       'av_email'           => 'Email',
				       'av_institute'       => "Affiliation",
				       'av_lastlogin'       => 'login');
	if ($holder->av_ids = array_keys(myOrg($org_id)->orgGroups()->get_groupMembers($group_code,True))){
	  $holder->listEntries();
	}
      }
    }
    $b_tabs->close();
  }  
}
