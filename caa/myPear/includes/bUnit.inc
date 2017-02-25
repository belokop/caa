<?php

define('use_memberInfo_SESSION',False);
if (cnf_show_acl) myPear::DEBUG('bUnit_memberInfo_SESSION (old slow cache): '.(use_memberInfo_SESSION ? 'YES' : 'NO'));

/*
 * Unit is a list of avatars united for a dedicated task
 */
locateAndInclude('bList');
locateAndInclude('bForm_Avatar');
class bUnit extends bList{

  var $ID        = 0;
  var $query     = array(); 
  var $MODE      = 'RW';
  var $statuses  = array();

  protected $orderBy    = 'av_lastname';
  protected $orderByDir = 'ASC';
  protected $b_tabs = Null;

  /*
   * See bList
   */
  var $TABLE           = 'zzz_units';
  var $TABLE_member    = 'zzz_unit_members';
  var $TABLEID         = 'u_id';
  var $TABLEID_member  = 'um_id';
  var $TABLEID_member_p= 'um_uid';

  // see bList
  protected $serialized= 'um_option';
  protected $keys      = array('id'     => 'u_id',
			       'parent' => 'u_parent',
			       'rank'   => 'u_rank',
			       'class'  => 'u_class',
			       'name'   => 'u_name',
			       'memberT'=> 'u_member_title');

  /*
   * Present the units for the module as tabs.
   * The method is designed to show bUnits from mupliple modules in a single invocation.
   * However, this is useful mainly for debugging
   */
  public static function b_tabs_bUnits($modules=MyPear_MODULE,$parent_ID=myOrg_ID,$exclude=array(),$include='all'){
    locateAndInclude('b_tabs');

    // Strip out modules where this parentID does not present 
    $modules = (empty($modules)
		? array_keys(b_reg_bUnit::get_description())
		: array($modules));
    foreach($modules as $module){
      $rank_used[$module] = $rank_unused[$module] = array();
      foreach(b_reg_bUnit::get_description(Null,$module) as $rank=>$d){
	$where = array("u_rank = $rank");
	if ($parent_ID) $where[] = "u_parent = '$parent_ID'";
	$q = myPear_db()->query("SELECT u_parent FROM zzz_units WHERE ".join(' AND ',$where)." LIMIT 1");
	if (myPear_db()->num_rows($q)) $rank_used[$module][] = $rank;
	else                           $rank_unused[$module][] = $rank;
      }
      if (empty($rank_used[$module])) {
	$parent_name = ($parent_ID == myOrg_ID
		      ? ' ('.myOrg_name.')'
		      : '');
	myPear::WARNING("module \"$module\" does not have any bUnit for parent_ID=$parent_ID $parent_name");
	b_debug::print_r($rank_unused,'rank_unused');
	$modules = array_diff($modules,array($module));
      }
    }

    if (count($modules) > 1){
      print x('h2',"All units for parent=\"$parent_ID\"");
      locateAndInclude('b_tabs');
      $tabs_modules = new b_tabs_myPear($modules);
    }
    
    foreach($modules as $module){
      if (isset($tabs_modules) && !$tabs_modules->body()) continue;

      // Get all the wanted units for the module
      $list = array();
      foreach(b_reg_bUnit::get_description(Null,$module) as $rank=>$d){
	if (in_array($rank,$exclude))                                      continue;
	if (($include !== 'all') && !in_array($rank,array_keys($include))) continue;
	if (isset($d['class'])) $list[$rank] = $d;
      }
      
      $class_for_ranks = (False           // count($modules) > 1
			  ? 'b_tabs_JQ'
			  : 'b_tabs_myPear');
      
      // Show ranks in the tabs from the "most important" to the "least important"
      krsort($list);
      foreach($list as $rank=>$d){
	if (superUser_here || ($rank != RANK__superuser)) $tabbs[$rank] = $d['d'];
      }
      $tabs_ranks = new $class_for_ranks(array_values($tabbs));
      foreach(array_keys($tabbs) as $rank){
	if ($tabs_ranks->body()){
	  locateAndInclude($class = $list[$rank]['class']);
	  $unit = new $class(($parent_ID ? $parent_ID : $include[$rank]),$rank);
	  $unit->getForm();
	}
      }
      $tabs_ranks->close();
    }
    if (isset($tabs_modules)) $tabs_modules->close();
  }
  
  /*
   * Delete the unit and it members
   */
  static public function deleteUnit($u_id,$membersOnly=False){
    $debug = (cnf_dev || cnf_CLI || cnf_show_db);
    myPear_db()->qquery("DELETE FROM zzz_unit_members WHERE um_uid = '$u_id'",$debug);
    if (!$membersOnly) myPear_db()->qquery("DELETE FROM zzz_units WHERE u_id = '$u_id'",$debug);
  }

  /*
   * Return list of known units
   */
  static public function getUnits($rank=False,$parent=False){
    $list  = array();
    $where = array(1);
    if ($parent) $where[] = "u_parent= '$parent'";
    if ($rank)   $where[] = "u_rank  = '$rank'";
    $orderBy = 'u_parent,u_rank';
    $q = myPear_db()->query("SELECT * FROM zzz_units WHERE ".join(' AND ',$where)." ORDER BY $orderBy DESC");
    while ($r = myPear_db()->next_record($q)) $list[$r['u_id']] = $r;
    return $list;
  }

  /*
   * Get back the units where av_id is member of
   *
   * @return array[av_id][] = array('u_rank',
   *                                'u_parent',
   *                                'u_class')
   */
  static public function get_unitsMembership($parent=Null,$rank=Null,$av_id=Null,$class=Null){

    // There might be many ranks in the query
    if (empty($rank))     $rank = array();
    if (!is_array($rank)) $rank = array($rank);
    foreach($rank as $r){
      if (!empty($r))  $where_rank[] = myPear_db()->quote($r, ((int)$r ? True : 'R'), 'u_rank');
    }

    // There might be many parents searched
    if (empty($parent))     $parent = array();
    if (!is_array($parent)) $parent = array($parent);
    foreach($parent as $r){
      if (!empty($r))  $where_parent[] = myPear_db()->quote($r, ((int)$r ? True : 'R'), 'u_parent');
    }

    
    // Build "where"
    if (!empty($av_id))        $where[] = myPear_db()->quote($av_id,  1, 'um_avid');
    if (!empty($class))        $where[] = myPear_db()->quote($class,  1, 'u_class');
    if (!empty($where_rank))   $where[] = '('.join(' OR ',$where_rank).')';
    if (!empty($where_parent)) $where[] = '('.join(' OR ',$where_parent).')';
    if ( empty($where))        $where[] = 0;

    $q = myPear_db()->query("SELECT um_avid,u_rank,u_parent,u_class FROM zzz_unit_members ".
			    " LEFT JOIN zzz_units ON um_uid = u_id ".
			    " WHERE ".join(' AND ',$where)." GROUP BY u_id ORDER BY u_rank DESC");
    
    $reply = array();
    while($r=myPear_db()->next_record($q)){
      $reply[$r['um_avid']][] = array('u_parent'=>$r['u_parent'],
				      'u_rank'  =>$r['u_rank'],
				      'u_class' =>$r['u_class']);
    }
    return $reply;
  }

  /*
   *
   */  
  public function getEmails($status=Null){
    $reply = array();
    foreach ($this->getUnitMemberIDs() as $av_id){
      $r = $this->getUnitMember($av_id);
      if (empty($r['av_email'])){
	if (cnf_dev) b_debug::var_dump($r,__METHOD__.': empty $r[av_email]');
      }else{
	if (($status===Null) || ($status === @$r['um_status'])) $reply[bForm_Avatar::_fmtName('Lf',$r,False)] = $r['av_email'];
      }
    }
    return $reply;
  }

  /*
   * bUnit generic constructor
   */
  public function __construct($parent_ID, $rank){
    // Say "I am bUnit, not bList";
    $this->rank = $rank;

    // Check for the special case - superuser
    if ($rank == RANK__superuser) $parent_ID = myPear_MODULE;

    // And go on
    parent::__construct($parent_ID,$rank);
  }

  /*
   * By default update only the packed items of bUnit
   */
  protected function edit_bList(){
    if ($this->myPost_bUnit()){
      $updates = array();
      foreach($this->packed_items as $item){
	if ($v = $this->getValue($item)) $updates[$item] = $v;
      }
      if ($updates) $this->updateMember($updates,$_POST['um_id']);
    }
  }

  
  /*
   *
   */
  protected function myPost($arg=array()){
    return $this->myPost_bUnit($arg);
  }

  /*
   *
   */
  function myPost_bUnit($r=array()){
    $reply = $this->myPost_bList($r) && isset($_POST['um_id']);
    return $reply;
  }

  /*
   *
   */
  function unitMemberIcon($av_id=Null){
    return b_reg_bUnit::get_icon($this->rank);
  }

  function unitMemberTitle($av_id=Null){
    return $this->memberTitle();
  }

  /*
   *
   */
  protected $bUnit_buttons_row = array();
  function getForm(){

    if ($name=$this->name()) print x('h3',$name);

    // Modify the unit membership if requested
    if     ($av_id = @$_GET['del_m_once']) $wait=$this->remove_unitMember($av_id,True);
    elseif ($av_id = @$_GET['edi_m_once']) $wait=$this->edit_unitMember($av_id);
    elseif ($av_id = @$_GET['add_m_once'])       $this->add_unitMember($av_id);

    if (empty($av_id)) $memberTitle = 'member';
    else               $memberTitle = $this->unitMemberTitle($av_id);
    
    // Switch to the list unit members when add / delete / remove is done    
    if  (empty($wait)){
      $url_on_success = join(',',array("add_m_once=AV_ID",
				       "ed_bList=".get_class($this),
				       (($t=@$_GET['b_tab']) ? ",b_tab=$t" : "").
				       "arg1_once=$this->parent_ID",
				       "arg2_once=$this->rank"));
      
      $create_url = b_url::same(array('id'             => 'new',
				      'form'           => bForm_Avatar::$bForm_Avatar, 
				      'resetcache_once'=> 1,
				      '_button'        => "appoint as a $memberTitle",
				      '_after_create'  => $url_on_success),
				b_url::get_GET('^ed_bList()',True));
      
      $search_url = b_url::same(array('resetcache_once' => 1,
				      'form'            => 'bForm_Avatar_Search', 
				      '_button'         => "appoint as a $memberTitle",
				      '_after_search'   => $url_on_success),
				b_url::get_GET('^ed_bList()',True));
      
      
      $this->show();
      if($this->isWritable()){
	$this->bUnit_buttons_row[] = b_btn::submit("add new $memberTitle from the database",$search_url);
	$this->bUnit_buttons_row[] = b_btn::submit("\"create\" a new $memberTitle",         $create_url);
	print b_btn::UL_buttons($this->bUnit_buttons_row);
      }
    }
  }
  
  
  /*
   *
   */
  protected function header(){
    return array('name'=>'', 'av_institute'=>'');
  }

  /*
   *
   */
  protected function format(){}
  
  protected function all_members_tidy_name(){
    //    if ($object=myPear::getClassFromID($this->parent_ID,'class'))
    //      $p = $object->name();
    //    else   
    $p = '';
    return "$p ".$this->unitMemberTitle().'s';
  }


  /*
   * See is the member record publicly readably.
   * By default the authentication is required.
   */
  function isMemberAccess_public($av_id){
    return is_object(bAuth::$av);
  }

  function isOwner(){
    return bAuth::authenticated() && (bAuth::$av->ID == @$this->rec['um_avid']);
  }

  /*
   * Detect which tab should be selected when the page is shown.
   * Select the tab with the new member if the latter was just added to the unit.
   */
  protected $av_justAdded = False;
  protected function detect_selectedTab($tabs){
    $selected = 0;
    if ((count($tabs) > 1) && is_object($this->av_justAdded)){
      $name_letter1 = substr($this->av_justAdded->getValue('av_lastname',1,1),0,1);
      foreach ($tabs as $tab_number=>$tab) if (stripos($tab,$name_letter1) !== False) $selected = $tab_number;
    }
    return $selected;
  }

  /*
   * Decide what should be the tabs structure to present the listing
   */
  protected function build_tabs_for_show(){
    //    
    // Collect the first letters of the last-names and count them
    //
    $c = array();
    $total = 0;
    foreach($this->get_memberInfo() as $av_id=>$r){
      if (!$this->accept_for_tab($r)) continue;
      $f_letter = mb_strToUpper(mb_substr(@$r['av_lastname'],0,1,'UTF-8'),'UTF-8');
      if (empty($c[$f_letter])) $c[$f_letter] = 0;
      $c[$f_letter]++;
      $total++;
    }
    ksort($c,SORT_STRING);

    //
    // Analise the result and decide how many tabs we will need to present the list
    //
    $tabs = array();
    switch($total < $this->maxPerPage){
    case True:
      // Good. Not too many members, the list will fit in 1 page (in one tab)
      $tabs['all'] = 'all';
      break;

    case False:
      // Break the members by number of people "per letter"
      if ($total < 2*$this->maxPerPage){
	$this->maxPerPage = (int)($total / 2. * 1.3);
      }
      $current_size = 0;
      foreach($c as $l=>$n){
	if ($current_size > 0){
	  if($current_size + $n < $this->maxPerPage){
	    $current_size += $n;
	    $current_tabTitle[] = $l;
	  }else{
	    $current_size = 0;
	    $tab_human = $tab_for_search = join('-',$current_tabTitle);
	    if (($n=count($current_tabTitle)) > 2){
	      $tab_human = $current_tabTitle[0].'-'.$current_tabTitle[$n-1];
	    }
	    $tabs[$tab_for_search] = $tab_human;
	  }
	}
	
	if ($current_size == 0){
	  $current_size = $n;
	  $current_tabTitle = array($l);
	}
      }
      $tab_human = $tab_for_search = join('-',$current_tabTitle);
      if (($n=count($current_tabTitle)) > 2){
	$tab_human = $current_tabTitle[0].'-'.$current_tabTitle[$n-1];
      }
      $tabs[$tab_for_search] = $tab_human;
    }
    return $tabs;
  }
  
  /*
   * Egg & Chicken...
   */
  protected function accept_for_tab(&$rec,$tabs=Null,$tab=''){

    if (($tabs === Null) || count($tabs) == 1){
      // We are creating the tabs
      $reply = True;
    }elseif (empty($rec)){
      $reply = False;
    }elseif(empty($rec['av_lastname'])){
      if (cnf_dev) b_debug::print_r($rec,sprintf("%s(%s)->%s() - %s",get_class($this),$this->parent_ID,__FUNCTION__,'bogus record, missing "av_lastname"')); 
      if (!empty($rec['um_id'])) $this->deleteMember($rec['um_id'],cnf_dev);
      $reply = False;
    }else{
      // The tabs are known, and we are populating them
      $reply = True;
      $options = explode('-',mb_strToLower($tab,'UTF-8'));
      if ((count($options) > 1) || (mb_strlen($tab,'UTF-8') == 1)){
	$reply = in_array(mb_substr(mb_strToLower($rec['av_lastname'],'UTF-8'),0,1),$options);
      }
    }
    return $reply;
  }

  function deleteMember($args=array(),$verbose=True){
    parent::deleteMember($args,$verbose);
    $this->get_memberInfo(Null,Null,'reset');
  }
  
  protected function show_set_css() {}

  /*
   * Show the unit members, arrange them by tabs if there are too many members
   */
  function show($onlyNames=False){
    locateAndInclude(bForm_Avatar::$bForm_Avatar);

    // Get the tabs by the start letters of surnames
    $tabs = $this->build_tabs_for_show();
    if ($all_in_one=@$tabs['all']) $tabs['all'] = $this->all_members_tidy_name();
    
    // Open the preselected tab
    $selected = $this->detect_selectedTab($tabs);
    locateAndInclude('b_tabs');
    $this->b_tabs = new b_tabs(array_values($tabs),$selected);
    
    // Fill the entries
    foreach ($tabs as $tab_search=>$tab){
      if (!$this->b_tabs->body()) continue;

      // Show the members list      
      if ($onlyNames) $header = array('name'=>'');
      else            $header = $this->header();
      locateAndInclude('b_table');
      $this->t = new b_table_zebra($header);
      $this->t->noSort = True;
      $this->show_set_css();
      
      foreach($this->get_memberInfo() as $av_id=>$this->rec){
	$this->rec['av_id'] = $av_id;
	if (!$this->accept_for_tab($this->rec,$tabs,$tab_search)) continue;
	
	// Get the unit member name.
	// Depending on the access rights that might jeust the name, or a link to "edit page"
	$this->rec['name'] = $this->show_getAnker();
	
	if (!$onlyNames){
	  $this->t->extraTD[] = $this->showStatus($this->rec);
	  $this->t->extraTD[] = $this->button_edit_unitMember();
	  $this->t->extraTD[] = $this->button_deleteUnitMember();
	}
	$this->format();
	$this->t->prt($this->rec);
      }
      $this->t->close(); 
    }
    $this->b_tabs->close();
  }

  /*
   *
   */
  protected function show_getAnker(){
    // Show only the photo for non-authenticated users and 
    // the full info for the authenticated ones
    $name = bForm_Avatar::_fmtName('Lf',$this->rec);
    if ($this->isMemberAccess_public($this->rec['av_id'])){
      bJS()->width = '75%';
      b_url::$no_crypt = True;
      $url = b_url::same("?mode_once=RO&id=".$this->rec['av_id']."&form=".bForm_Avatar::$bForm_Avatar,
			 b_url::get_GET('^(ed_bList|arg[1234])',True));
      $reply = bJS()->modal_ajax($url,$name,Null,True);
    }elseif(is_file($photo = bForm_Avatar::photo_repo(@$this->rec['av_photo']))){
      $reply = bJS()->modal_image_file($name,$photo,$name);
    }else{
      $reply = $name;
    }
    return $reply;
  }

  private function show_getAnker_public(){}  

  /*
   *
   */
  protected function button_deleteUnitMember(){
    $name = bForm_Avatar::_fmtName('Lf',$this->rec);
    return ($this->isUnitMemberDeletable()
	    ? bIcons()->getButton(array('X'=>1,
					'l'=>b_url::same(array('del_m_once'=>$this->rec['av_id'],
							       'clean_once'=> 'act_code,act_id',
							       'ed_bList'  => get_class($this),
							       'arg1_once' => $this->parent_ID,
							       'arg2_once' => $this->rank),
							 array()),			
					'd'=>"remove $name from unit"),
				  'i-drop') 
	    : '');
  }

  /*
   *
   */
  protected function button_edit_unitMember(){
    $name = bForm_Avatar::_fmtName('Lf',$this->rec);
    return ($this->isMemberEditable()
	    ? bIcons()->getButton(array('X'=>1,
					'l'=>b_url::same(array('edi_m_once'=>$this->rec['av_id'],
							       'ed_bList'  => get_class($this),
							       'arg1_once' => $this->parent_ID,
							       'arg2_once' => $this->rank,
							       'clean_once'=> 'act_code,act_id',
							       ),
							 array()),			
					'd'=>"modify $name's membership"),
				  'i-edit') 
	    : '');
  }
  
  /*
   *
   */
  protected function showStatus(){
    return '';
  }
  
  /*
   * Unit name
  function name($arg=Null){ // ,$max_length=99){
    $reply = $this->getValue('u_name');
    if (empty($reply)) $reply = '';
    return $reply;
  }
   */
  
  /*
   * return list for <select> menu
   */
  function get(){
    $list = array();
    $q = myPear_db()->query("SELECT * FROM zzz_avatars ".
			    " LEFT JOIN ".$this->TABLE_member." ON av_id = um_avid ".
			    " WHERE um_uid = " . $this->ID . myPear_db()->orderBy());
    while ($r=myPear_db()->next_record($q)){
      $list[$r['um_avid']] = b_fmt::unEscape(bForm_Avatar::_fmtName('Lf',$r,$escape=False));
    }
    $this->dbg(b_fmt::joinX(', ',$list));
    return $list;
  }

  protected function hookJOIN(){
    return join(' ',array(' LEFT JOIN zzz_units ON u_id = um_uid ',
			  ' LEFT JOIN zzz_avatars ON av_id = um_avid '));
  }

  /*
   * Get the complete record of the member
   */
  function getUnitMember($av){
    if(is_object($av)) $av_id = $av->ID;
    else               $av_id = $av; 
    $reply = array();
    foreach($this->getMembers(array('um_avid'=>$av_id)) as $k=>$r)  $reply = $r;
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Check the membership
   */
  function isMember($av=Null){
    if(empty($av))         $av_id = @bAuth::$av->ID;
    elseif(is_object($av)) $av_id = $av->ID;
    elseif((int)$av)       $av_id = $av; 
    $reply = (empty($av_id)
	      ? False
	      : !myPear::is_empty($this->get_memberInfo($av_id))); // Check against the cache    
    return $reply;
  }
  
  /*
   * Check the member status.
   */
  function getUnitMemberStatus($av=Null){
    if(is_object($av)) $av_id = $av->ID;
    else               $av_id = $av; 
    $reply = $this->get_memberInfo($av_id,'um_status');
    return $reply;
  }

  function getOption($av){
    if(empty($av))         $av_id = @bAuth::$av->ID;
    elseif(is_object($av)) $av_id = $av->ID;
    else                   $av_id = $av; 
    if ($this->isMember($av_id)){
      $q = myPear_db()->query("SELECT um_option FROM zzz_unit_members WHERE um_id = ".$this->get_memberInfo($av_id,'um_id'));
      while($r = myPear_db()->next_record($q)) $reply = $r['um_option'];
    }
    return $reply;
  }

  /*
   *
   */
  function setOption($av,$um_option){
    if(empty($av))         $av_id = @bAuth::$av->ID;
    elseif(is_object($av)) $av_id = $av->ID;
    else                   $av_id = $av; 
    if ($this->isMember($av_id)){
      myPear_db()->query("UPDATE zzz_unit_members SET um_option = '$um_option' WHERE um_id = ".$this->get_memberInfo($av_id,'um_id'));
    }
  }

  /*
   * @param - return False if status was not changed, otherwise True
   */
  function setStatus($av,$status,$verbose=True){
    if(is_object($av))  $av_id = $av->ID;
    elseif((int)$av)    $av_id = $av; 
    else  b_debug::internalError("Can't parse argument '$av'");
    
    // Go home if it is not the unit member
    if (!$this->isMember($av_id))                         return False;
    
    // Go home if the status is not changed
    $current_role   = ($this->av_justAdded 
		       ? 'av_justAdded'
		       : $this->unitMemberTitle($av_id)); 
    $current_status = ($this->av_justAdded 
		       ? 'av_justAdded'
		       : $this->getUnitMemberStatus($av_id));
    if ($current_status  === $status)                     return False;

    // Update the status
    if (!empty($status)){
      myPear_db()->query("UPDATE zzz_unit_members SET um_status = '$status' WHERE um_id = ".$this->get_memberInfo($av_id,'um_id'));
      $this->get_memberInfo($av_id, array('um_status' => $status),'update');
    }
    
    // Print a message if required
    $this->setStatus_message($av_id,$current_role,($verbose && !empty($current_status) && !empty($status)));
    return True;
  }
  
  /*
   * Helper for setStatus, print the "status changed" message
   */
  private function setStatus_message($av_id,$current_role,$verbose=True){
    if (empty($av_id)){
      debug_print_backtrace();
      return;
    }
    if ($verbose){
      $role = $this->unitMemberTitle($av_id); 
      $name = x('em',$this->get_memberInfo($av_id,'sfl'));
      switch($current_role){
      case 'dejaVu':
	$unit = $this->name();
	myPear::MESSAGE(trim("$name is already a member of $unit ".$this->message_url($av_id)));
	break;

      case 'av_justAdded':
	$current_role = '';
      default:
	if ($current_role) $current_role = " (previously $current_role)"; 
	myPear::WARNING_T(trim("Appointing $name as $role $current_role ".$this->message_url($av_id)));
	break;
      }
    }
  }
  
  /*
   * Helper. Optionally add a click-able URL to the unit member record
   */
  protected function message_url($av_id){
    return '';
  }
  
  /*
   *
   */
  public function add_unitMember($av_id,$verbose=True,$um_status=Null){
    if ($av_id instanceof bForm_Avatar){
      $av = $av_id;
    }elseif(!(int)$av_id){
      return Null;
    }

    // Create the Avatar object
    if(!is_object(@$av)){
      $av = new bForm_Avatar($av_id);
      if ($av->ID != $av_id) b_debug::internalError("???? av->ID='$av->ID' !== av_id='$av_id'");
    }

    $av->defineVariables();
    if ($this->isMember($av)){
      if ($verbose === 'if_new') $verbose = False;
      $this->setStatus_message($av->ID,'dejaVu',$verbose);
      if (!empty($um_status))  $this->setStatus($av->ID,$um_status,$verbose);
      $this->add_unitMember_postFly($av->ID,True);
    }else{
      if ($verbose === 'if_new') $verbose = True;
      $this->av_justAdded = $av;
      $updates = array('um_id'   => bMaster()->reserve_masterCounter(),
		       'um_uid'  => $this->ID,
		       'um_avid' => $av->ID);
      $av->defineVariable($updates,True);

      myPear_db()->query("INSERT INTO ".$this->TABLE_member." SET ".b_fmt::joinX(',',$updates));
      myPear_logs()->add('newcomer',$av->ID,str_ireplace(myOrg_name,'',$this->name())); // (($c=get_class($this))=='bUnit' ? $this->name() : str_ireplace('bUnit_','',$c)));

      // Update the cache
      foreach($this->membersInfo_toSelect(True) as $k){
	if (!isset($av->vars[$k])) $av->defineVariable($k,$k);
	if  ($k == 'sfl')                      $updates[$k] = $av->fmtName('sfl');
	elseif($v=$av->getValue($k,True,True)) $updates[$k] = $v;
      }
      $this->get_memberInfo($av->ID,$updates,'update');

      // Announce the new member
      $this->setStatus($av->ID,$um_status,$verbose);
      $this->add_unitMember_postFly($av->ID,False);
    }
    return $this->get_memberInfo($av->ID,'um_id');
  }

  /*
   *
   */
  protected function add_unitMember_postFly($av_id,$alreadyThere){}

  /*
   *
   */
  protected function edit_unitMember($av_id){
    locateAndInclude('b_tabs');
    b_tabs::$jQueryTabs_depth = 1;

    $av = new bForm_Avatar::$bForm_Avatar($av_id);
    $av->getForm();
    return 'busy, wait please..';
  }

  /*
   *
   */
  private function remove_unitMember($av_id,$toConfirm=False){
    $this->dbg();
    if ($this->isMember($av_id)){
      $av = new bForm_Avatar($av_id);
      $av->defineVariables();
      $remove_msg = "remove <em>".$av->name().'</em> from '.$this->getValue('u_name');
      if ($toConfirm)  $answer = b_btn::confirm();
      else             $answer = 'yes';
      if ($answer == 'yes'){
	myPear_db()->query("DELETE FROM ".$this->TABLE_member." WHERE um_avid='$av_id' AND um_uid='$this->ID'");
	myPear::MESSAGE($remove_msg);
	$this->get_memberInfo($av_id,Null,'reset');
	myPear_logs()->add('rem_member',$av_id,str_ireplace(myOrg_name,'',$this->name())); 
     }elseif(empty($answer)){
	b_btn::confirm("Are you sure you want to $remove_msg?");
	return True;
      }
    }
    return False;
  }

  /*
   * Update the cache also
   */
  function updateMember($updates=array(),$member_ID=Null,$set_as_current_member=True){
    $um_id = parent::updateMember($updates,$member_ID,$set_as_current_member);
    $r = $this->getMember($um_id);
    $this->get_memberInfo($r['um_avid'],$updates,'update');
    return $um_id;
  }
  
  /*
   * Get the unit member information (name, status, etc)
   * @param $av_id
   *        Avatar ID to get
   * @param $column
   *        Database column to get (um_status, um_id, etc.)
   * @param $action:
   *        - 'update' update the cache with new values
   *        - 'reset'  reset (empty) the cache
   *        - 'get'    just get a value
   *        - 'get_byStatus' get back array of $av_id's having status(es) in $column
   */
  public static $counterU = array();

  private function id_unique($id){
    return join('_',array($id,
			  get_class($this), 
			  $this->ID,
			  $this->parent_ID,
			  $this->rank));
  }

  function get_memberInfo($av_id=Null,$column=Null,$action='get'){
    // Those columns are not cached
    $not_to_cache = array('av_salutation','av_id','u_name',$this->serialized);
    
    // Eventually reset the cache
    $counters_cache = $this->id_unique('Counters');
    $members_cache  = $this->id_unique('Members');

    if ($action == 'reset'){
      if ((int)($av_id)) unset($_SESSION[$members_cache][$av_id]);
      else               unset($_SESSION[$members_cache]);
      if (empty($_SESSION[$counters_cache]['resetCounter'])) $_SESSION[$counters_cache]['resetCounter'] = 1;
      $this->dbg("=================== RESET#".($_SESSION[$counters_cache]['resetCounter']++)." cache for av_id=$av_id",cnf_dev||$this->debug);
      return;
    }   
    
    bTiming()->cpu(__FUNCTION__);    

    //
    // Initialise the cache if not yet done
    //
    if (empty($_SESSION[$members_cache])){
      $_SESSION[$members_cache] = array();
      bTiming()->cpu('BUILD');
      
      // Get list of the packed items to be cached
      $packed_items = array_intersect($this->packed_items,$this->membersInfo_toSelect());
      $select = array_diff($this->membersInfo_toSelect(),$packed_items);
      if (!empty($packed_items)) $select[] = $this->serialized;
      
      $where = array("zzz_unit_members.um_uid = ".$this->ID);
      if ((int)($av_id) && !empty($_SESSION[$members_cache]))  $where[] = "av_id = $av_id";
      $q = myPear_db()->qquery("SELECT u_name,".join(',',array_unique($select))." FROM zzz_unit_members ".
			       " LEFT JOIN zzz_avatars ON zzz_avatars.av_id = zzz_unit_members.um_avid ".
			       " LEFT JOIN zzz_units ON zzz_unit_members.um_uid = zzz_units.u_id ".
			       " WHERE " . join(' AND ',$where) . myPear_db()->orderBy(),
			       ($this->debug||cnf_dev));
      $n_added = myPear_db($q)->num_rows();
      
      while ($r=myPear_db()->next_record($q)){
	if (empty($r)){
	  $n_added--;
	  continue;
	}
	if (empty($_SESSION[$members_cache][$r['av_id']]) || ($_SESSION[$members_cache][$r['av_id']] == $r['av_id'])){
	  // Fill the non-packed data first
	  $_SESSION[$members_cache][$r['av_id']] = array('sfl' => bForm_Avatar::_fmtName('sfl',$r));
	  foreach($r as $k=>$v) if (!empty($v)) $_SESSION[$members_cache][$r['av_id']][$k] = $v;
	  
	  // Fill the packed data
	  if (!empty($r[$this->serialized])){
	    foreach(unserialize($r[$this->serialized]) as $k=>$v){ 
	      if (in_array($k,$packed_items) && !empty($v)) $_SESSION[$members_cache][$r['av_id']][$k] = $v;
	    }
	  }
	  // Clean the unused items
	  foreach($not_to_cache as $k) unset($_SESSION[$members_cache][$r['av_id']][$k]); 
	}else{
	  // Duplication might appear after merging avatars and should be removed
	  if ($this->isWritable() || cnf_dev){
	    myPear::WARNING("Duplicated membership <i>".bForm_Avatar::_fmtName('sfl',$r)."</i> in ".$this->name()." (u_id=".$this->ID.",um_id=$r[um_id]) is REMOVED");
	  }
	  if (cnf_dev){
	    myPear::DEBUG(sprintf("%s(%s,%s,%s)->".__FUNCTION__."(%s,%s) %s",
				  get_class($this),$this->parent_ID,$this->rank,$this->MODE,(is_numeric($av_id) ? $av_id : ''),$column,
				  x('em',"Duplicated membership <i>".bForm_Avatar::_fmtName('sfl',$r)."</i> in ".$this->name()." (u_id=".$this->ID.",um_id=$r[um_id]) is REMOVED")));
	    b_debug::print_r($_SESSION[$members_cache][$r['av_id']],'_SESSION');
	    b_debug::print_r($r,'database');
	  }
	  myPear_db()->query("DELETE FROM ".$this->TABLE_member." WHERE um_id = $r[um_id]");
	}
      }

      // Empty entries... Where from ?
      if ((int)($av_id) && empty($_SESSION[$members_cache][$av_id])) 	$_SESSION[$members_cache][$av_id] = array();

      // Show counters
      if (empty($_SESSION[$counters_cache]['buildCounter'])) $_SESSION[$counters_cache]['buildCounter'] = 1;
      $this->dbg("build#".($_SESSION[$counters_cache]['buildCounter']++)." cache \"av_id=$av_id\", ".count(array_keys($_SESSION[$members_cache]))." members ($n_added added)",
		 (bool)(cnf_dev||$this->debug));
      // Sanity
      if (cnf_dev){
	foreach($_SESSION[$members_cache] as $id=>$data){
	  if (empty($data)) b_debug::xxx("???Empty record ".x("a href='".b_url::same(array('form'=>'Avatar','id'=>$id))."'","av_id=$id"));
	}
      }
      bTiming()->cpu();    
    }
    
    //
    // The cache is ready...
    // See what we are expected to do
    //
    if (empty($_SESSION[$members_cache])){
      $reply = array();      // Sanity
    }else{
      
      self::$counterU[$counters_cache]['cache size'] = count($_SESSION[$members_cache],COUNT_RECURSIVE);
      if (empty(self::$counterU[$counters_cache][$av_id])) self::$counterU[$counters_cache][$av_id] = 0;
      
      $reply = '?';
      switch($action){
      case 'update':
	foreach(array_intersect($this->membersInfo_toSelect(True),
				array_keys($column)) as $k){
	  if (in_array($k,$not_to_cache))   continue;
	  
	  $old_value = @$_SESSION[$members_cache][$av_id][$k];
	  $new_value = @$column[$k];
	  if ($old_value != $new_value){
	    $_SESSION[$members_cache][$av_id][$k] = $new_value;
	    if (cnf_dev){
	      if (is_array($new_value)) $new_value = 'Array('.join(',',$new_value).')';
	      if (is_array($old_value)) $old_value = 'Array('.join(',',$old_value).')';
	      //if (!empty($new_value) && !is_string($new_value)) b_debug::var_dump($new_value,"$k - new_value");
	      myPear::DEBUG(sprintf("%s(%s,%s,%s)->".__FUNCTION__."(%s,%s) %s",
				    get_class($this),$this->parent_ID,$this->rank,$this->MODE,(is_numeric($av_id) ? $av_id : ''),(is_string($column)?$column:'Array'),
				    x('em',(empty($old_value)
					    ? "add $k: '$new_value'"
					    : "update ".@$_SESSION[$members_cache][$av_id]['sfl']." $k: '$old_value' -> '$new_value'"))));
	    }
	  }
	}
	break;
	
      case 'get_byStatus':
	if (!is_array($column)) $column = array($column);
	$reply = array();
	foreach($_SESSION[$members_cache] as $id=>$r){
	  if (isset($r['um_status']) && in_array($r['um_status'],$column)){
	    @self::$counterU[$counters_cache][$id]++;
	    $reply[$id] = $r;
	  }
	}
	break;
	
      case 'get':
	if    ($av_id ===Null) $reply =  $_SESSION[$members_cache];
	elseif($column===Null) $reply = @$_SESSION[$members_cache][$av_id];
	else                   $reply = @$_SESSION[$members_cache][$av_id][$column];
	self::$counterU[$counters_cache][$av_id]++;
	break;
	
      default:
	if (cnf_dev) b_debug::traceBack("Forgot '$action'");
      }  
    }
    $this->dbg($reply);
    bTiming()->cpu();
    if (0)    if (empty($reply)){
	$this->dbg($reply,empty($reply));
	b_debug::var_dump($_SESSION[$members_cache][$av_id],$av_id);
      }
    return $reply;
  }  

  protected function membersInfo_toSelect($extended=False){
    $reply = array('av_id',
		   'av_email',
		   'av_firstname',
		   'av_lastname',
		   'av_von',
//		   'av_salutation',
		   'av_photo',
		   'um_id',
		   'um_uid',
		   'um_avid',
		   'um_status',
		   );
    if ($extended) $reply[] = 'sfl';
    return $reply;
  }

  /*
   * Get duty holders
   */
  protected function getDutyHolders($trigger,$duty=Null){
    $reply = array();
    foreach($this->getUnitMemberIDs() as $av_id){
      if (empty($duty)){
        $accept = True;
      }else{
        $duties = $this->arg_explode(@$this->get_memberInfo($av_id,$trigger));
	$accept = in_array($duty,$duties);
      }
      if ($accept) $reply[$av_id] = array('av_name' => (mb_strToUpper($this->get_memberInfo($av_id,'av_lastname'),'utf-8').' '.
							$this->get_memberInfo($av_id,'av_firstname')),
					  'av_email'=> $this->get_memberInfo($av_id,'av_email'));
    }
    return $reply;
  }

  /*
   *
   */
  function getUnitMemberIDs($status=Null){
    if (empty($status)) return array_keys($this->get_memberInfo());
    else                return array_keys($this->get_memberInfo(Null,$status,'get_byStatus'));
  }
  
  
  protected function isMemberEditable(){
    return False;
  }

  protected function isUnitMemberDeletable(){
    return $this->isWritable();
  }
  
  /*
   *
   */
  static public function show_all($parent=''){
    foreach (self::getUnits('',$parent) as $r){
      $u = new bUnit($r['u_parent'],$r['u_rank']);
      print "<ol>";
      foreach ($u->getUnitMemberIDs() as $av_id){
	$av = myPear::getInstance_new('bForm_Avatar',$av_id,'nocache');
	if (is_object($av)){
	  $av->defineVariables();
	  print "<li>".$av->name()."</li>";
	}                                                                                                                           
      }                                                                                                                           
      //      print "</ol>";
      b_fmt::collapsible();
    }
  } 

  function __toString() {
    return get_class($this) . "($this->parent_ID,$this->rank)";
  }
}
