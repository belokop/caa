<?php
/*
 * Implement a simple list of 2-keys objects.
 */

define('bList_use_membersCache',True);
locateAndInclude('myPear_getValue');

class bList extends myPear_getValue{

  public static function get_all($class=Null,$parent_ID=Null,$rank=Null){ 
    $reply = array();
    foreach(array('zzz_lists'=>'l',
		  'zzz_units'=>'u') as $table=>$prefix){
      $where = array(1);
      if ($class)     $where[] = "${prefix}_class  = '$class'";
      if ($parent_ID) $where[] = "${prefix}_parent = '$parent_ID'";
      if ($rank && myPear_db()->columnExists("${prefix}_rank",$table)) $where[] = "${prefix}_rank = '$rank'";
      $q = myPear_db()->query("SELECT ${prefix}_id FROM $table WHERE ".join(' AND ',$where));
      while($r = myPear_db()->next_record($q)) $reply[] = $r["${prefix}_id"];
    }				    
    return $reply;  
  }

  /*
   *
   */
  public static function b_tabs_bLists($module,$lists=array()){
    
    foreach($lists as $list_name=>$parent_ID) $tags[] = b_reg_bList::get($module,$list_name,0);    

    locateAndInclude('b_tabs');
    $b_tabs = new b_tabs_myPear($tags);
    foreach($lists as $list_name=>$parent_ID){
      if ($b_tabs->body()){
	locateAndInclude($list_name);
	$bList = new $list_name($parent_ID);
	$bList->getForm();
      }
    }
    $b_tabs->close();
  }
  
  /*
   * Scan the list members for the given arguments, get back the parent list class.
   * @param return  the list class instance or Null
   */
  static public function getListInstance($argsP,$bList='',$createIfMissing=False){

    if (strpos($bList,'bList')!==False){
      if (is_numeric($argsP))  $argsP = array('l_parent'=>$argsP);
      elseif(is_object($argsP))$argsP = array('l_parent'=>$argsP->ID);
      $parent_ID = @$argsP['l_parent'];
    }else{
      if (is_numeric($argsP))  $argsP = array('u_parent'=>$argsP);
    }
      
    $instance_ID = b_crypt::hash(serialize(array_merge(array('list'=>$bList),$argsP)));

    static $instances = array();
    if (isset($instances[$instance_ID])){
      //      static $conter = array();      @$conter[$instance_ID]++;      b_debug::xxx($conter[$instance_ID]);
      return $instances[$instance_ID];      
    }

    // The vanilla bUnit class might be used only for the Organization-wide units
    if ($bList === 'bUnit'){
      if (empty($argsP['u_parent'])) $argsP['u_parent'] = myOrg_ID;
      $valid_combination = (($argsP['u_parent'] == myOrg_ID) ||
			    ($argsP['u_parent'] == myPear_MODULE &&  $argsP['u_rank'] == RANK__superuser));
      if (!$valid_combination){      
	myPear::WARNING("The basic 'bUnit' class might be used only for the Organization-wide units.<br/>Please create a separate class.");
	return Null;
      }
      return ($instances[$instance_ID] = new bUnit($argsP['u_parent'],$argsP['u_rank']));
    }
    
    // Search for the class
    if (!empty($argsP)){

      // First search the bList instances, then bUnit
      foreach(array('bList' => array('l' => 'l',
				     'lm'=> 'lm',
				     't' => array('zzz_lists','zzz_list_members')),
		    'bUnit' => array('l' => 'u',
				     'lm'=> 'um',
				     't' => array('zzz_units','zzz_unit_members')),
		    ) as $list=>$vars){
	if (!empty($bList) && (stripos($bList,$list) === False)) continue;

	$l = $vars['l'];
	$lm= $vars['lm'];
	if (empty($bList) || (stripos($bList,$list)!==False)){
	  if (is_array($argsP))$args = $argsP;
	  else                 $args = array("${l}_parent"=>$argsP);
	  if ($bList)          $args["${l}_class"] = $bList;

	  $tables = join(' LEFT JOIN ',$vars['t']);
	  $q = myPear_db()->query($qq="SELECT * FROM $tables ON ${l}_id = ${lm}_${l}id WHERE ".
				  join(' AND ',$where=self::args2select($args,$vars['t'],"${lm}_option")).
				  " GROUP BY ${l}_class");
	  if ($alarm=((($n=myPear_db()->num_rows($q)) > 1)&&cnf_dev)){
	    b_debug::print_r($where,__METHOD__." !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! $n records for 'where'");
	  }

	  while($r = myPear_db()->next_record($q)){
	    if ($alarm)b_debug::var_dump($r);
	    if (empty($bList)) $bList = $r["${l}_class"];
	    if ($bList !== $r["${l}_class"]) b_debug::traceBack("$bList !== ".$r["${l}_class"]);
	    if (locateAndInclude($bList)){
	      if ($rank = @$r['u_rank']) $instances[$instance_ID] = new $bList($r["${l}_parent"],$rank);
	      else                       $instances[$instance_ID] = new $bList($r["${l}_parent"]);
	      break 2;
	    }
	  }
	}
      }
    }

    if (isset($instances[$instance_ID])){
      $reply = $instances[$instance_ID];     
    }elseif($createIfMissing && $parent_ID && locateAndInclude($bList)){
      $reply = $instances[$instance_ID] = new $bList($parent_ID);
    }else{
      $reply = Null;
    }
    b_debug::xxx($reply);
    return $reply;
  }

  /*
   * Delete the list and its members
   */
  static public function deleteList($l_id,$membersOnly=False){
    $debug = (cnf_dev || cnf_CLI || cnf_show_db);
    if (is_object($l_id))  $l_id = $l_id->ID;
 
    // print message
    if ($debug){
      $q = myPear_db()->query("SELECT * FROM zzz_lists WHERE l_id = '$l_id'");
      while ($r = myPear_db()->next_record($q)) b_debug::xxx("id=$l_id parent=\"$r[l_parent]\" class=\"$r[l_class]\", created=$r[l_timestamp]");
    }
    // delete the records
    myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_lid = '$l_id'",$debug);
    if (!$membersOnly) myPear_db()->qquery("DELETE FROM zzz_lists WHERE l_id = '$l_id'",$debug);
  }
  
  /*
   * get the sql query "where" 
   */
  protected static $_serialized;
  protected static function args2select($args,$table,$_serialized=''){
    $w[] = 1;
    if (!is_array($table)) $table = array($table);
    if (empty($_serialized)) $_serialized = self::$_serialized;
    foreach($args as $key=>$value){
      $ok = False;
      foreach($table as $t) if (myPear_db()->columnExists($key,$t)) $ok = True;
      if (cnf_dev && is_array($value)){
	print '<pre>';
	b_debug::internalError('value must be a string, not '.var_export($value,True));
      }
      // escape symbols [+)(] for the regexp search
      if ($ok) $w[] = myPear_db()->quote($value, 1, $key);
      else     $w[] = myPear_db()->quote(preg_replace('/\.$/','.*',
							str_replace('_DOT_STAR_','[[:>:]].*[[:<:]]',
								    preg_replace('/\W/','.',"${key}_DOT_STAR_${value}"))),
					   'R',$_serialized,True);
    }
    return $w;
  }

  /*
   * The keys of the list:
   * id     - column which keeps the unique list ID
   * parent - column which keeps the parent ID
   * name   - column which keeps the list name. The "list name" might be anything
   *
   * The pair parent/name (as well as 'id') defines the list
   */
  protected $keys   = array('id'     => 'l_id',
			    'parent' => 'l_parent',
			    'class'  => 'l_class',
			    'name'   => 'l_name',
			    'memberT'=> 'l_member_title');
  /*
   * All the bList instances have rank 0,
   * while the bUnit instances have rank > 0, see bUnit class
   */
  public $rank = 0;

  /*
   * What to ask and/or to show during the list editing dialog
   */
  var $list_quering = array('lm_key'   => 'name',
			    'lm_value' => Null);
  var $list_showing = array('lm_key'   =>'name',
			    'lm_value' => Null);
  var $isMBE = array(); // Might Be Empty

  /*
   * The list ID & name
   */
  public $ID        = Null;
  public $parent_ID = Null;

  /*
   * R/W mode (currently useless, hardly used, to be(?) removed in the future)
   */
  var $MODE            = 'RO';

  var $TABLE           = 'zzz_lists';
  var $TABLE_member    = 'zzz_list_members';
  var $TABLEID_member  = 'lm_id';
  var $TABLEID_member_p= 'lm_lid';

  var $isRO            = array();
  var $debug           = False;

  // edit / add / drop
  protected $act_code     = Null;

  // b_table options
  protected $repetValuesCol1only = False;
  protected $noSort              = False;
  protected $table_attributes    = '';

  private   $w = array();
  private   $where_eq      = array(); // 
  protected $where_regexp  = array(); // 
  public    $where_negates = array(); // 
  public    $getMembers_where = array();

  public    $member_ID  = 0;  // current member ID got via $_POST and/or $_GET[lm_id]
  public    $maxPerPage = 50; // maximum number of lines in the list
  protected $tabTrigger = 'lm_key';

  // those items are packed into the lm_option field
  protected $packed_items= array();
  protected $serialized  = 'lm_option';

  public    $sortArgs    = '';
  protected $orderBy     = 'lm_key';
  protected $orderByDir  = 'ASC';
  public    $preHeaders  = array();

  public $tips        = array();
  public $tips_byMode = array();

  /**
   * bList constructor. Creates the list if it does not exist 
   * 
   * @param $parent_ID
   *   Parent ID
   *
   * @param $name
   *  The list name
   */
  public function __construct($parent_ID,$rank=Null){

    $this->TABLEID   = $this->keys['id'];
    $this->parent_ID = $parent_ID;

    parent::__construct();
    $this->dbg("----------------------------------------------------------------");
    
    // "Rank" is zero for bLists, check that it is as expected    
    $this->rank = (int)$rank;
    if(($this instanceof bUnit) && ($this->rank <= 0)){
      b_debug::internalError("Wrong invocation, rank='$rank'");
    }
    
    // Read the class record to $this->formDB
    $this->readDefRecord();

    // Perform the records editing
    $this->edit_bList();
    
    // Optionally check for duplicated information
    if (($s=strToLower(b_cnf::get('sanity'))) && (($s = 'all') || (stripos(get_class($this),$s) !== False)))    $this->sanity();
    
    //
    // Set the 'sort by' value for the output array
    $sBy = explode(',',$this->orderBy);
    $sBy = array_shift($sBy);
    if (empty($sBy) || !(myPear_db()->columnExists($sBy,$this->TABLE_member))){
      if (isset($_GET['sBy']) && myPear_db()->columnExists($_GET['sBy'],$this->TABLE_member)) $sBy = $_GET['sBy'];
      else                                                                                    $sBy = $this->TABLEID_member;     
    }
    $this->repetValuesCol1only = $sBy;
  }

  /*
   *
   */
  public function get_packed_items(){
    return $this->packed_items;
  }

  /*
   * Perform the add / edit session
   */
  protected function edit_bList(){

    if (!b_cnf::get('act_code')) return;
    
    //
    // Do the editing
    //
    if (b_cnf::get('act_id') == $this->ID){
      $this->act_code = b_cnf::get('act_code');
      $this->MODE = 'RW';
      $this->dbg("act_code=$this->act_code");
      
      $myPost_bList = $this->myPost_bList();
      switch($this->act_code){
      case 'add':  /////////////////////////////////////////////////////////////////
	if (!empty($_POST) && !$myPost_bList) b_debug::var_dump($_POST,"adding bList member, but not myPost_bList"); 
	if (!empty($_POST) && !$myPost_bList) b_debug::traceBack("adding bList member, but not myPost_bList"); 
	// Return to the dialog if the data is not complete 
	if ($this->getFormAdd_OK()){
	  
	  if ($member_ID = @$_REQUEST[$this->TABLEID_member]){
	    $this->dbg("======update member record ",cnf_dev);
	    $this->member_ID = $member_ID;
	    $this->updateMember($_POST,$this->member_ID,True);
	  }else{
	    // No member_ID on input, add a new member record 
	    $this->dbg("======creating new list member from _POST",cnf_dev);
	    $this->addMember($_POST);
	    $this->getValue_reset($this->TABLEID_member,$this->member_ID);
	    $this->dbg("======created new list member ".$this->member_ID,cnf_dev);
	  }
	  if (empty($_GET['add_continue_once'])) $this->declare_action_completed($this->act_code.' completed');
	}
	break;
	
      case 'edit': ////////////////////////////////////////////////////////////////
	if (!empty($_POST) && !$myPost_bList) b_debug::var_dump($_POST,"editing bList member, but not myPost_bList"); 
	if (!empty($_POST) && !$myPost_bList) b_debug::traceBack("editing bList member, but not myPost_bList"); 
	$this->getMember($this->member_ID = b_cnf::get('act_lmid'), True);
	if ($this->getFormAdd_OK()){
	  $this->dbg("======$this->TABLEID_member=$this->member_ID applying updates from _POST");
	  $this->updateMember($_POST,$this->member_ID,True);
	  $this->declare_action_completed($this->act_code.' completed');
	}
	break;

      case 'drop': //////////////////////////////////////////////////////////
	$this->deleteMember($_GET['lm_id']);
	$this->declare_action_completed('record dropped');
	break;
      }
    }
  }

  /*
   * Handle "i am ready" flag
   */
  protected $isReady = False;
  function isReady($setReady=False){
    if (func_num_args() != 0)         $this->isReady = (bool)$setReady;
    $this->dbg($this->isReady);
    return $this->isReady;
  }

  /*
   * Get the list member title 
   */  
  public function memberTitle(){
    $reply = $this->getValue($this->keys['memberT']);
    return $reply;
  }

  /*
   * Read the list record from the 'zzz_lists' table,
   * create the list if not yet done
   */
  private $bList_isJustCreated = False;
  private function readDefRecord(){
    $just_created = False;
    if (myPear_db()->isReady()){    
      // find the record in the db
      if(empty($this->ID)){
	$w = array(myPear_db()->quote($this->parent_ID,1,$this->keys['parent']),
		   myPear_db()->quote(get_class($this),1,$this->keys['class']));
	if(isset($this->keys['rank'])) $w[] = myPear_db()->quote($this->rank,1,'u_rank');
      }else{
	$w = array(myPear_db()->quote($this->ID,1,$this->TABLEID));
      }
      
      // The result of this query must be either unique or empty.
      // If it is not, then something is wrong...
      $q = myPear_db()->query("SELECT ".$this->TABLEID." FROM ".$this->TABLE." WHERE ".join(' AND ',$w));
      if ($alarm = cnf_dev && (($n=myPear_db()->num_rows($q)) > 1)) b_debug::print_r($w,b_debug::__()." !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! $n records");
      while ($r = myPear_db()->next_record($q)){
	if ($alarm) b_debug::print_r($r,get_class($this).'->'.__FUNCTION__.'()');
	$this->ID = $r[$this->TABLEID];
      }
      
      //
      // Create the list root record if it does not exists yet.
      //
      if (empty($this->ID)){
      	// Create the list root record,
	// The list members will be hooked to it when requested
	$this->ID = bMaster()->reserve_masterCounter();
	$w[] = $this->TABLEID . '=' . $this->ID;

	// Get the default class name and member title from the configuration file.
	// They might be changed later (TBI)
	list($l_name,$l_member_title) = b_reg_bList::set__l_name__l_member_title($this,$this->keys);
	$w[] = $this->keys['name'] . "='$l_name'";
	$w[] = $this->keys['memberT'] . "='$l_member_title'";
	
	myPear_db()->query("INSERT INTO ".$this->TABLE." SET " . join(',',$w));
	$just_created = True;
	
      }
      
      //
      // We are sure now that the list exists, read the root record
      //
      $q = myPear_db()->query("SELECT * FROM $this->TABLE ".$this->readDefRecord_joinTables()." WHERE $this->TABLEID = $this->ID");
      while ($r = myPear_db()->next_record($q)) $this->formDB = $formDB = $r; 	  
      
      // Just to be sure...
      if (empty($formDB[$this->keys['name']]) || 
	  empty($formDB[$this->keys['memberT']])) b_reg_bList::set__l_name__l_member_title($this,$this->keys);
      
      // Announce the list creation  
      if ($just_created) $this->message_createList();
    }
  }

  /*
   * Optionally join other tables
   */
  protected function readDefRecord_joinTables(){
    return '';
  }


  /*
   * Add a new member to the list
   */
  protected $member_ID_lastAdded = 0;
  function createMember(){
    $this->member_ID_lastAdded = bMaster()->reserve_masterCounter();
    $this->message_createMember();
    myPear_db()->qquery("INSERT INTO $this->TABLE_member SET ".
			join(',',array($this->hookParent(),
				       $this->TABLEID_member.'='.$this->member_ID_lastAdded)),
			(bool)$this->debug||cnf_show_db);
    return $this->member_ID_lastAdded;
  }
  
  protected function message_createMember(){
    $this->dbg($this->TABLEID_member.'='.$this->member_ID_lastAdded);
  }

  /*
   */
  protected $message_deleteMember_custom = '';
  function deleteMember($args=array(),$verbose=True){
    if (is_numeric($args)) $args = array($this->TABLEID_member => $args);

    $w[]= $this->hookParent();
    foreach($args as $k=>$v) $w[] = myPear_db()->quote($v,  1, $k);

    // First count the records to be deleted
    $n_toBeDeleted = 0;
    foreach($this->getMembers($args) as $lm_id=>$deleted_record){
      $n_toBeDeleted++;
      if ($verbose) $this->message_deleteMember(array($this->TABLEID_member=>$lm_id));
    }
    $this->message_deleteMember_custom = '';

    // Then delete 
    if ($n_toBeDeleted > 0){
      $this->reset_cache_bList_getMembers();
      myPear_db()->qquery("DELETE FROM ".$this->TABLE_member." WHERE ".join(' AND ',$w),cnf_dev);
      $this->report_changes('delete',$w);
    }
    if (!empty($deleted_record)) return $deleted_record;
  }

  /*
   * Report changes to the "monitoring body"
   */
  protected function report_changes($what,$args){
    //    if (cnf_dev) b_debug::print_r($args,get_class($this)."($this->ID)->".__FUNCTION__);
  }

  /*
   * Add list member if it does not exist
   */
  function addMember($args=array()){
    // Massage the agruments if needed
    $this->addMember_check_args($args);
    
    if (!myPear::is_empty($records = $this->getMembers($args))){
      if (($n=count($ids=array_keys($records))) > 1) b_debug::traceBack("expect 1, got $n"); 
      $reply = array_pop($ids);
    }else{
      $reply = $this->updateMember($args,$member_ID=Null,$set_as_current_member=False);
      $this->message_addMember($this->getMember($reply));
    }

    // Sanity...
    if (empty($reply)) b_debug::internalError("??? Empty reply: member_ID='$reply'");
    return ($this->member_ID = $reply);
  }

  protected function addMember_check_args(&$args) {}
  
  /*
   *
   */
  protected function message_addMember($r){
    if (empty($this->member_ID_lastAdded)){
      // if (cnf_CLI || cnf_dev) b_debug::traceBack("empty 'member_ID_lastAdded'");
    }else{
      $entry_name = $this->message_addMember_name($r);
      if (empty($entry_name)) $entry_name = 'new entry';
      else                    $entry_name = x('"',$entry_name);
      myPear::MESSAGE("Adding member $entry_name ".($this->debug||cnf_show_db ? 'lm_id='.$this->member_ID_lastAdded : '').
		      ' to the '.($this instanceof bUnit ? 'unit' : 'list').'" '.$this->name().'"');
    }
  }
  
  protected function message_addMember_name($r){
    return '';
  }

  /*
   *
   */
  function updateMember($updates=array(),$member_ID=Null,$set_as_current_member=True){
    $this->dbg();
    $debug = (bool)$this->debug || cnf_show_db || cnf_dev;
    $was_record = array();

    // Create the member record if it does not exist yet
    if($member_ID === 'new') $member_ID = $this->createMember();
    
    // The member_ID is an array, search using it as keys
    if (is_array($member_ID)){
      $candidates = array_keys($this->getMembers($member_ID));
      if (count($candidates) == 1){
	$member_ID = $candidates[0];
	$this->dbg("Discover member_ID=$member_ID");
	$was_record = $this->getMember($member_ID);
	$already_ok = True;
	foreach($updates as $k=>$v) if (@$was_record[$k] !== $v) $already_ok = False;
	if ($already_ok){
	  $this->dbg('already ok');
	  return $member_ID;
	}
      }else{
	$member_ID = Null;
      }
    }
    
    // There is no explicit member_ID, try to locate the record using the updates
    if (empty($member_ID)){
      if (empty($updates)) $updates = array_merge($this->hookParent(True),$updates);
      // First look the the record_ID, then for the keys
      foreach(array(array($this->TABLEID_member),
		    array($this->TABLEID_member_p,'lm_key','lm_value')) as $keys){
	$args = array();
	foreach($keys as $key){
	  if (!empty($updates[$key])) $args[$key] = $updates[$key];
	}
	if (empty($args))  continue;
	if (count($candidates=array_keys($this->getMembers($args))) != 1) continue;
	$member_ID = $candidates[0];
	$this->dbg("member_ID=$member_ID is located from the updates ".b_fmt::joinX(', ',$args));
	$was_record = $this->getMember($member_ID);
	break;
      }
    }
    
    //
    // The record to be updated is not located...
    //
    if (empty($member_ID)){
      //  Just return the member_ID if the updates are destinate to a "black hole" (i.e. the record already exists)
      foreach($this->getMembers($updates) as $r){
	return $r[$this->TABLEID_member];
      }
      // Or continue with creating a new record
      $member_ID = $this->createMember();
    }

    //
    // Now the member record is located, remove the member_ID from updates not to confuse MySQL
    //
    unset($updates[$this->TABLEID_member]);

    // Update the member record 
    if (empty($was_record))     $was_record = $this->getMember($member_ID);
    $upd_record = $this->unpack($was_record,$updates);


    // Reset the cache
    $this->reset_cache_bList_getMembers();
    
    // Update the database
    $w[]= $this->hookParent();
    foreach($upd_record as $k=>$v){ 
      if (myPear_db()->columnExists($k,$this->TABLE_member) &&
	  (($was=@$was_record[$k]) !== $v)){
	if (!is_string($v) && !is_numeric($v)){
	  b_debug::internalError('$v is not a string, but '.var_export($v,True));
	}
	$w[] = myPear_db()->quote(trim($v), 1, trim($k)); 
	$report_changes[$k] = "\"$was\" -> \"$v\"";
      }
    }

    if (count($w) > 1){ 
      myPear_db()->qquery("UPDATE $this->TABLE_member SET ".join(',',$w).
			  " WHERE $this->TABLEID_member=$member_ID",$debug);
      $this->report_changes('update',$report_changes);
      $this->reset_cache_bList_getMembers();

      // reread back the record 
      if ($set_as_current_member){
	$this->member_ID = $member_ID;
	$this->formDB_member = $this->getMembers($this->member_ID);
      }
    }
    
    if (False && $debug){
      ksort($upd_record);
      ksort($was_record);
      b_debug::print_r($updates,'updates');
      b_debug::print_r($was_record,'was_record');
      b_debug::print_r($upd_record,'upd_record');
      $diff = array();
      foreach(array_unique(array_merge(array_keys($was_record),array_keys($upd_record))) as $k){
	if (!in_array($k,$this->packed_items) && !myPear_db()->columnExists($k,$this->TABLE_member)) continue;
	if (($b=@$was_record[$k]) !== ($a=@$upd_record[$k])){
	  if (!myPear::is_empty($b)) $diff[$k]['bef'] = $b;
	  if (!myPear::is_empty($a)) $diff[$k]['aft'] = $a;
	}
      }
      b_debug::print_r($diff,'changes...');
      if ($diff){
	b_debug::print_r($updates,'updates');
	b_debug::print_r($was_record,'was_record');
	b_debug::print_r($upd_record,'upd_record');
      }
    }
    return $member_ID;
  }
  
  /*
   * Same as getMembers (see below), but returns only one member record, not an array of them.
   * More then 1 record on output is a fatal error.
   */
  function getMember($args,$set_as_current_member=False){

    $reply = array();
    if($args === 'new'){
      // Well... Request for a new empty member record...
      // Check then, maybe there is already an empty record available
      $args = array();
      foreach (array('lm_key'  =>'',
		     'lm_value'=>'',
		     'um_avid' =>0) as $f=>$v){
	if (myPear_db()->columnExists($f,$this->TABLE_member)) $args[$f] = $v;
      }
      if($ids=array_keys($this->getMembers($args))) $empty_member_ID = array_pop($ids);
      else                                          $empty_member_ID = $this->createMember();
      $args = array($this->TABLEID_member => $empty_member_ID);
    }
    
    // Trick - cast args as array, this will force "getMembers" to create a proper cache
    if (is_numeric($args)) $args = array($this->TABLEID_member => $args);

    // Read the database
    foreach(($getMembers=$this->getMembers($args)) as $lm_id=>$data){
      if (!empty($reply)){      // Sanity
	if (cnf_dev){
	  echo "<pre style='text-align:left'>\n------------------------getMember args\n";
	  print_r($args);
	  echo "------------------------getMember reply\n";
	  print_r($getMembers);
	  echo "</pre>";
	}
	$n = count(array_keys($getMembers));
	b_debug::internalError("Expect 1 reply, but got $n");
      }
      $reply = $data;
    }
    
    // Copy the record to the "current member record" if requested
    if (!empty($reply) && $set_as_current_member){
      $this->member_ID     = $reply[$this->TABLEID_member];
      $this->formDB_member = $reply;
    }
    return $reply;
  }

  /*
   *
   */
  private $reset_counter = 0;
  private function reset_cache_bList_getMembers(){
    $this->dbg('Reseting cache_bList_getMembers#'.(++$this->reset_counter));
    unset(self::$counterL[get_class($this)]);
    $this->cache_bList_getMembers = array();
  }

  /*
   * Return array of the list/unit entries corresponding to the set of arguments.
   * If the argument is a number, then return one record with <primary key> == <argument>.
   */
  public static $counterL = array();
  protected $cache_bList_getMembers = array();
  function getMembers($args=array()){

    bTiming()->cpu(array(get_class($this),__FUNCTION__));

    if ($reply_as_one_record = is_numeric($args)){
      $args = array($this->TABLEID_member => $args);
    }

    // convert the input arguments into the sql query
    self::$_serialized = $this->serialized;
    $w = array_merge($this->getMembers_where,
		     self::args2select($args,array($this->TABLE,$this->TABLE_member),$this->serialized));
    $this->getMembers_where = array();
    // To get a unique cache when called from getMember
    if ($reply_as_one_record) $w[] = 1;
    
    // The function might be used to extract the members records of any list,
    // not necessarily of the current one. Hence, do not hard-code the parent record link
    if (empty($args[$this->TABLEID_member_p]) && 
	empty($args[$this->keys['parent']])) $w[] = $this->hookParent();
    
    foreach ($this->where_eq      as $k=>$v) $w[] = myPear_db()->quote($v,   1, $k);     $this->where_eq      = array();
    foreach ($this->where_regexp  as $k=>$v) $w[] = myPear_db()->quote($v, 'R', $k,True);$this->where_regexp  = array();
    foreach ($this->where_negates as $k=>$v) $w[] = myPear_db()->quote($v, 'N', $k);     $this->where_negates = array();
    
    // Build the query string
    if (empty($this->sortArgs))  $this->sortArgs = " ORDER BY $this->orderBy $this->orderByDir";
    $query = 'SELECT * FROM '.$this->TABLE_member.$this->hookJOIN().' WHERE '.join(' AND ',$w).' '.$this->sortArgs;
    $this->sortArgs = '';

    //
    // Shortcut... Get the answer from a cache if the latter exists
    //
    $cache_hash = b_crypt::hash($query);
    if (empty(self::$counterL[get_class($this)][$cache_hash])){
      self::$counterL[get_class($this)][$cache_hash] = array('c'=>0);
    }

    if (!empty($this->cache_bList_getMembers[$cache_hash])){
      self::$counterL[get_class($this)][$cache_hash]['c']++;
      self::$counterL[get_class($this)][$cache_hash]['s'] = count($this->cache_bList_getMembers[$cache_hash],COUNT_RECURSIVE);
      $this->dbg("reading from cache #".self::$counterL[get_class($this)][$cache_hash]['c']);
      bTiming()->cpu();
      return $this->cache_bList_getMembers[$cache_hash];
    }

    //
    // No cache available, read the records from database
    //
    $q = myPear_db()->query($query);
    
    $n = myPear_db()->num_rows($q);
    $reply = array();
    while($r = myPear_db()->next_record($q)) {
      if ($reply_as_one_record){
	if ($n > 1) b_debug::internalError("Expect 1 reply, but got $n",$r);
	$reply = $this->unpack($r);
      }else{
	$reply[$r[$this->TABLEID_member]] = $this->unpack($r);
      }
    }
    
    // Sanity, check the records for duplications
    if (!$reply_as_one_record){
      if (empty($reply)) $reply = array();
      $this->getMembers_removeDuplications($reply);
    }
    
    // Return the results
    $n = count($reply);
    if (!$reply_as_one_record)    $this->dbg("$n entr".($n==1?'y':'ies'));
    
    // Optionally save the result in a cache
    if (bList_use_membersCache){
      $this->cache_bList_getMembers[$cache_hash] = $reply;
    }else{
      self::$counterL = array();
    }
    bTiming()->cpu();
    return $reply;
  }
  
  /*
   * Check the array of records for duplications and remove them, if any.
   */  
  private $getMembers_removeDuplications_running = 0;
  protected function getMembers_removeDuplications(&$reply){

    if (!cnf_show_dupl){
      $a = array($this->parent_ID);
      if ($this instanceof bUnit) $a[] = $this->rank;
      static $dejaVu = 1;
      if (cnf_dev && !$dejaVu++) myPear::DEBUG(sprintf(get_class($this).'(%s)->'.__FUNCTION__.'() <em>Skiping slow function</em>',join(',',$a)));
      return;
    }

    if ($this->getMembers_removeDuplications_running++){
      myPear::WARNING_T('getMembers_removeDuplications_running');
      return;
    }
    bTiming()->cpu(array(get_class($this),__FUNCTION__));

    // create a hash of all data records for the output array (ignore timestamp, ID, locale, etc.)
    $hash = array();
    foreach($reply as $r_id=>$r){
      foreach(array('l_','u_','lm_','um_','') as $t){
	foreach(array('id','lid','uid','timestamp','locale','member_title',$this->serialized) as $m){
	  unset($r[$t . $m]);
	}
      }
      ksort($r);
      $hash[$id=mb_strToLower(serialize(($r)))][] = $r_id;
      $hash[$id] = array_unique($hash[$id]);
      sort($hash[$id]);
    }
    
    // see the duplications
    foreach($hash as $h=>$lm_ids){
      $r_id_keep = array_pop($lm_ids);
      while(!empty($lm_ids)){
	$member_ID = array_pop($lm_ids);
	$this->message_deleteMember_custom = 'duplicated';
	$this->deleteMember($member_ID);
	if (cnf_dev){
	  b_debug::print_r($reply[$r_id_keep],"Kept list member $r_id_keep");
	  b_debug::print_r($reply[$member_ID],"Removed duplicated list member $member_ID");
	}
	unset($reply[$member_ID]);
      }
    }
    $this->getMembers_removeDuplications_running = 0;
    bTiming()->cpu();
  }

  protected function myPost($arg=array()){
    return $this->myPost_bList($arg);
  }
  
  /*
   * Check that the _POST contains data for the given list member.
   *
   * @param (array)$r - the member record to be checked against the _POST array.
   *                    If this argument is missing (or $r is empty), the current
   *                    member record (i.e. $this->formDB_member array) is checked
   * @return True if the $_POST is "mine".
   */
  function myPost_bList($r=array()){

    $reply = False;
    if(isset($_GET['_POST__post_bList_class'])){
      if ($_GET['_POST__post_bList_class'] !== get_class($this)) return $reply;
    }else{
      if (@$_POST['_post_bList_class'] !== get_class($this)) return $reply;
    }

    $get2post = array();
    $posts = preg_grep('/^_POST_/',array_keys($_GET));
    foreach($posts as $kk){
      $k = str_replace('_POST_','',$kk);
      $get2post[$k] = $_POST[$k] = $_REQUEST[$k] = $_GET[$kk];
    }
    b_cnf::_unset($posts);
    if (cnf_dev && !empty($get2post)) b_debug::print_r($get2post,'get2post');
 
    // Load the list member declared 
    if ($id = $this->getValue($this->TABLEID_member)) $this->getMember($id,True);

    if (myPear::is_empty($rr=$r)) $rr = array($this->TABLEID       =>  $this->ID,
					      $this->TABLEID_member=> @$this->member_ID);
    $l_id      = @$rr[$this->TABLEID];
    $member_ID = @$rr[$this->TABLEID_member];
    $lm_id     = @$_POST[$this->TABLEID_member];
    $parent_id = @$_POST[$this->TABLEID_member_p];
    
    if (empty($l_id)){
      if ($l_id = @$_POST['lm_lid']){
	myPear::WARNING_T("Recover 'l_id'=$l_id from 'lm_lid'");
      }else{
	b_debug::traceBack("both l_id & lm_id are empty");
      }
    }
    
    if (!empty($parent_id) && ($parent_id != $l_id)){
      $reply = False;
    }elseif ($lm_id === 'new'){
      $reply = True;
    }elseif (@$_POST['_post_bList_classID'] === $l_id){
      $reply = True;
      // OK, but check the list member
      if (is_numeric($lm_id)){
	if (empty($r)){ // Accept the _POST if the current member is not setup yet
	  if (!($reply=empty($this->formDB_member)))  $reply = ($lm_id == $member_ID);
	}else{
	  $reply = ((int)$lm_id == (int)$member_ID);
	}
      }
    }
    
    // Set the current member ID from the post if it is "mine"
    if ($reply){
      if ($lm_id !== Null)   $this->member_ID = $lm_id;
      if (@$_POST['um_status'] === 'off') $_POST['um_status'] == '';
    }

    if($this->debug || cnf_dev) $this->dbg(($reply ? 'YES ' : 'NO ').$this->TABLEID."=$l_id, member_ID=$member_ID",
					   array(3,'blueText',$this,True));
    return $reply;
  }

  function dump() {}

  protected function sanity_action($where) {}

  protected function sanity(){
    foreach($this->getMembers() as $r_id=>$r){
      $w = array($this->TABLEID_member." != $r_id"); 
      foreach($r as $k=>$v){
	if ($k == $this->serialized)                            continue;
	if (!myPear_db()->columnExists($k,$this->TABLE_member)) continue;
	if ($k == $this->TABLEID_member)                        continue;
	if (stripos($k,'time')  !== False)                      continue;
	if (stripos($k,'local') !== False)                      continue;
	$w[] = myPear_db()->quote($v,1,$k);
      }
      
      $q = myPear_db()->qquery("SELECT * FROM ".$this->TABLE_member." WHERE ".join(' AND ',$w),True);
      if (!($num_rows = myPear_db()->num_rows($q))) continue;
      

      myPear::ERROR("$num_rows duplications ".join(',',$w));
      $this->sanity_action($w);
    }
  }
  
  /*
   * Hook the list / unit root record
   */
  protected function hookParent($associated=False){
    if (!is_numeric($this->ID)) b_debug::internalError("??? \$this->ID = ".var_export($this->ID,True)); 
    if ($associated) return array($this->TABLEID_member_p  => $this->ID);
    else             return       $this->TABLEID_member_p."=".$this->ID;
  }
  
  /*
   * hook the table with the root record
   */
  protected function hookJOIN(){
    $reply = sprintf(" LEFT JOIN zzz_lists ON %s = %s",$this->TABLEID,$this->TABLEID_member_p);
    return $reply;
  }

  /*
   * Merge the updates with the existing record.
   * Pack/unpack the member record packed fields listed in the array $this->packed_items.
   *
   * @param $buffer  - the record to be updated
   * @param $updates - the updates
   * @param return   - array with updates applied. The packed field(s) are unpacked
   */
  protected function unpack($buffer=array(), $updates=array()){

    // sanity
    if(is_array($buffer)){
      foreach($buffer as $k=>$v){
	if(is_array($v)){
	  $buffer[$k] = join(',',$v);
	  if (empty($v)) unset($buffer[$k]); // ??? workarond
	}
      }
    }
    
    if (empty($buffer)) $buffer = array();
    if (empty($updates) || !is_array($updates)){
      $updates = array();
    }elseif(($idu=@$updates[$this->TABLEID_member]) && 
	    ($idb= @$buffer[$this->TABLEID_member]) && ($idu != $idb)){
      myPear::ERROR(b_debug::print_r($updates,"applying updates for $this->TABLEID_member=$idu, expect for $this->TABLEID_member=$idb",True));
      $updates = array();
    }

    return $this->repack_packables($buffer,$updates);
  }

  /*
   *
   */
  protected $unserialize_packables = True;
  protected function repack_packables($buffer,$updates){
    
    if ($this->unserialize_packables){
    // unpack only the packed items in both "current array" & "updates"
    $packables = array_merge($this->unserialize($buffer), $this->unserialize($updates));
    
    // update the non-packed "normal" items
    $reply = array_merge($buffer,$updates);    
    
    // pack back the updated packables
    if (!empty($this->packed_items)){    
      ksort($packables);
      foreach($packables as $k=>$v) $packables[$k] = str_replace("\n",'--LF--',
								 str_replace("\r",'--CR--',
									     str_replace("\r\n",'--CRLF--',$v)));
      $reply[$this->serialized] = serialize($packables);
    }
    }else{
      $reply = $buffer;
    }
    ksort($reply);
    return $reply;
  }


  /*
   * Helper
   */
  protected function arg_explode($value){
    $reply = array();
    if (!empty($value) && is_string($value)) $reply = explode(',',$value);
    elseif (is_array($value))                $reply = $value;
    return $reply;
  }

  /*
   * @return:
   *   True  - the input data is complete
   *   False - the input data is NOT complete, fatal 
   */
  function getFormAdd_OK(){
    $reply = !isset($_GET['act_once']);
    if($lm_id = $this->getValue($this->TABLEID_member)){
      foreach(array_keys($this->list_quering) as $q){
	if ($this->getMemberValue($lm_id,$q) || @$this->isMBE[$q]) continue;
	$this->dbg("missing '$q'",(bool)(cnf_dev || $this->debug));
	$reply = False;
      }
    }
    $this->dbg($reply,(bool)(cnf_dev || $this->debug));
    return $reply;
  }

  protected function getFormHeader(){
    return $this->name();
  }

  public function getForm(){

    // Sanity. Clean the garbage
    if ($ed_bList = b_cnf::get('ed_bList')){
      if ($ed_bList != get_class($this)) b_cnf::_unset('ed_bList');
    }

    // Optionally (re)set the page title
    $this->setPageHeader();
    
    print myPear::H2($this->getFormHeader());

    $this->dbg('act_code='.$this->act_code,(bool)cnf_dev||$this->debug);
    if (empty($this->act_code)){
      $this->show();
    }else{
      switch($this->act_code){
      case 'add':
      case 'edit':
	$this->getFormAdd();
	break;
	
      case 'drop':
      default:
	$this->show();
      }
    }
  }
  
  /*
   *
   */
  function get_action($code,$myGet=array()){
    $myGet['resetcache_once'] = 1;
    return b_url::same('?'.b_fmt::joinX('&',$this->get_action_array($code)).'&'.b_fmt::joinX('&',$myGet));
  }

  /*
   * Terminate action by cleaning the "act_<xxx>" session variables
   */
  private function declare_action_completed($message=''){
    $this->dbg($this->act_code,(bool)($this->debug||cnf_dev));
    $this->MODE     = 'RO';
    $this->act_code = Null;
    $this->get_action_array('cancel');
  }

  /*
   * Helper, builds arguments for the list editing
   */
  function get_action_array($code=''){
    $reply = array('act_once'  => 1,    // flag saying 'Мы начинаем...'
		   'act_id'    =>(int)$this->ID,
		   'act_code'  => $code,
		   'act_parent'=> $this->parent_ID,
		   'act_lmid'  =>@$this->r['lm_id'],
		   'lm_id'     =>@$this->r['lm_id'],
		   'ed_bList'  => get_class($this),
		   'arg1_once' => $this->parent_ID,
		   );
    switch($code){
    case  'add':
      if (False){
	$reply['lm_id'] = 'new';
	$reply['act_lmid'] = 'new';
      }else{
	$reply['clean_once'] = 'lm_id,act_lmid';
	unset($reply['lm_id']);
	unset($reply['act_lmid']);
      }
      break;

    case 'debug':
      foreach(array_keys($reply) as $k)	if ($v = b_cnf::get($k)) $dbg[$k] = $v;
      if (!empty($dbg)) b_debug::print_r($dbg,b_debug::__());
      break;

    case 'cancel':
      // Cancel the member IDs, but keep the list ID 
      b_cnf::_unset(array_diff(array_keys($reply),array('act_id')));
    }
    return $reply;
  }

  protected function getTabsHeader(){
    return ('All '.($this instanceof bUnit ? $this->memberTitle() : $this->memberTitle()).'s');
  }

  protected function defaultTabForListing($tabs){
    return count($tabs) - 1;
  }

  protected function setPageHeader() {}

  /*
   *
   */
  protected function show_open() {
    $this->show_open_table();
  }

  protected function show_close() {
    print b_btn::UL_buttons($this->draw_buttons());
  }

  protected function show_end() {
  }

  /*
   *
   */
  function show($content=''){
    locateAndInclude('b_tabs');

    // We do not need the $_POST array any longer, hence
    // empty it, otherwise getValue($name) will always return value from there
    if ($this->myPost_bList())      $_POST = array(); 

    // Optionally (re)set the page title
    $this->setPageHeader();

    if (empty($content)){
      $tabs = $this->getTabsForListing();
      if ($tabs==array('all')) $b_tabs = new b_tabs(array($this->getTabsHeader()));
      else                     $b_tabs = new b_tabs($tabs,$this->defaultTabForListing($tabs));
    }else{
      $tabs = array(1);
      $b_tabs = Null;
    }
    
    foreach ($tabs as $tab_number=>$tab){
      
      if (($b_tabs === Null) || $b_tabs->body()){	
	// Read all the records into the inner buffer "$this->r"
	foreach ($this->getMembers_forListing($tab_number) as $id=>$this->r) {
	  $this->show_open();
	  if (!empty($content)){
	    print $content;
	    $content = '';
	  }

	  if (!empty($this->r) && !$this->record_rejected()){
	    if ($this->isWritable()){
	      $this->t->extraTD[] = $this->button_editListMember();
	      $this->t->extraTD[] = $this->button_deleteListMember();
	    }
	    $this->format();
	    if ($this->t instanceof b_table){
	      if (!empty($this->r)) $this->t->prt($this->r); 
	      $this->t->extraTD = array();
	    }
	  }
	}
	// $this->t might be not defined, protect
	if (!empty($this->t) && ($this->t instanceof b_table)){
	  $this->draw_summary();
	  $this->t->close();
	  unset($this->t);
	}else{
	  $this->say_emptyListing();
	}
	$this->show_close();
      }
    }
    if (is_object($b_tabs)) $b_tabs->close();
    $this->show_end();
  }

  /*
   *
   */
  protected function button_editListMember(){
    return ($this->isMemberEditable()
	    ?  b_btn::link('ICON_edit',$this->get_action('edit'))
	    :  '');
  }

  protected function button_deleteListMember(){
    return ($this->isMemberDeletable()
	    ? b_btn::link('ICON_drop',$this->get_action('drop'),
			  "onClick='return confirm(\"Really delete this record?\");'")
	    : '');
  }

  protected function show_open_table(){
    if (empty($this->t)){
      $this->t = new b_table_zebra($this->header(),$this->table_attributes);
      $this->t->preHeaders          = $this->preHeaders;
      $this->t->noSort              = $this->noSort;
      $this->t->noSort              = True;
      $this->t->repetValuesCol1only = $this->repetValuesCol1only;
      $this->t->getHeader();
    }
  }

  protected function say_emptyListing(){
    print x('em',"The list is empty...<br/>");
  }

  protected function record_rejected() {
    return False;
  }

  /*
   * Format the record '$this->r' for listing
   */
  protected function format(){
    foreach(array_keys($this->list_showing) as $f){
      if (empty($this->r[$f])) $this->r[$f] = b_fmt::redText(x('em','missing'));
    }
  }

  /*
   * @param return
   *   sql_query selecting "l" (list of tabs) & "n" (number of entries in "l")
   */
  function getTabs_query(){
    return array(0,0);
  }

  /*
   * Get the list of tabs based on the custom selection
   */
  protected function getTabsForListing(){
    return array('all');
  }

  protected function getTabsForListing_OBSOLETE(){
    
    list($total,$c) = $this->getTabs_query();

    $tabs = array();
    if ($total < $this->maxPerPage){
      $tabs[] = 'all';
    }else{    // group the "non-populated letters"
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
	    $tabs[] = join('-',$current_tabTitle);
	  }
	}
	
	if ($current_size == 0){
	  $current_size = $n;
	  $current_tabTitle = array($l);
	}
      }
      $tabs[] = join('-',$current_tabTitle);
    }
    return $tabs;
  }
  
  /*
   *
   */
  protected function getMembers_forListing($arg=Null){
    return $this->getMembers();
  }

  protected function draw_summary() {}

  /*
   * Draw "add new member" button (and optionally other).
   * This function is called at the end of "$this->getForm()"
   */
  public $extra_buttons = array();
  protected function draw_buttons(){
    $reply = array();
    if ($this->isAddable()) {
      $reply[] = b_btn::submit("add new ".$this->memberTitle(),
			       $this->get_action('add'),
			       $confirm=True);
    }
    return array_merge($reply,$this->extra_buttons);
  }
  
  /*
   * Is the list writable?
   */
  private $isWritable_bList = Null;
  function isWritable(){
    if ($this->isWritable_bList === Null) $this->isWritable_bList = (bool)b_cnf::check_conditions(array(True =>array('CLI'  => cnf_CLI,
														     'owner'=> $this->isOwner(),
														     'mngr' => $this->isManager() && !cnf_inside_fb,
														     ),
													'default'=> False),
												  $this);
    return $this->isWritable_bList;
  }

  /*
   * The EA module is optional, hence protect the code by checking "if defined"
   */
  function isManager(){
    myPear_access(EA_MODULE);
    return EA_manager_here;
  }

  function isAdmin(){
    myPear_access(EA_MODULE);
    return EA_administrator_here;
  }

  /*
   *
   */  
  public function isOwner(){
    return False;
  }

  /*
   * Possible to add a new member to the list?
   */
  function isAddable(){
    $reply = $this->isWritable();
    b_debug::xxx($reply);
    return $reply;
  }

  /*
   * Possible to see this member?
   */
  protected function isMemberReadable($r=array()){
    return b_cnf::check_conditions(array('default' => True),
				   $this);
  }

  /*
   * Possible to edit this member?
   */
  protected function isMemberEditable(){
    $reply = $this->isWritable() && ($this->r['lm_status'] !== 'RO');
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Possible to delete this member?
   */
  protected function isMemberDeletable(){
    return $this->isWritable();
  }

  protected function header(){
    return $this->list_showing;
  }
  
  // called from bForm_lists
  function uploadInit_path($module_path=array(),$module=Null){}
  
  protected $f = Null;
  protected function bForm($variables,$inversed=False){
    static $dejaVu = 0;
    if ($dejaVu++) return;
    locateAndInclude('bForm_lists');
    if (empty($this->f)) $this->f = new bForm_lists($this,$variables,$inversed);
    $this->f->debug       = $this->debug;
    $this->f->tips        = $this->tips;
    $this->f->tips_byMode = $this->tips_byMode;
    $this->f->isMBE       = $this->isMBE;
    $this->f->getFormOpen();
    $this->f->textHidden('_post_bList_class',get_class($this));
    $this->f->textHidden('_post_bList_classID',$this->ID);
    //$this->f->textHidden('arg1_once',$this->parent_ID);
    //$this->f->textHidden('b_dummy','shit...'); // workaround (?)
    $this->f->textHidden($this->TABLEID_member_p,$this->ID);

    if ( !($lm_id = (int)@$_GET['act_lmid'])){
      if(!($lm_id = (int)@$this->member_ID))       $lm_id = 'new';
    }

    // Transmit the current member record ID (and show it in the debug mode)
    foreach(array((int)@$_GET['act_lmid'],
		  (int)@$this->member_ID,
		  'new') as $item){
      if (!empty($item)){
	$lm_id = $item;
	break;
      }
    }

    // Reset the _POST if needed
    if ($this->f->myPost_bForm()){
      $this->f->getValue_reset($this->TABLEID_member,$lm_id);
    }else{
      $this->f->formDB[$this->TABLEID_member] = $lm_id;
    }
    $this->f->textHidden($this->TABLEID_member,'show_it');
    $this->dbg('done');
  }

  protected function getFormAdd(){

    $this->bForm(array('name'               =>'lm_key',
		       $this->memberTitle() => 'lm_value'));
    $this->f->textField('lm_key');
    $this->f->textField('lm_value');
    $this->f->closeForm();
  }

  function getMemberID($key=Null,$value=Null){
    $reply = Null;
    if (!is_null($key))  $args['lm_key']   = $key;
    if (!is_null($value))$args['lm_value'] = $value;
    foreach($this->getMembers($args) as $r){
      if (!is_null($reply)) myPear::WARNING_T("Clashing ID=$reply for ".b_fmt::joinX(',',$args)); 
      $reply = $r[$this->TABLEID_member];
    }
    return $reply;
  }

  /**
   * Return indexed array (i.e. $list[key] = value) of the list members.
   * Ready for the <select>X</select>.
   */ 
  function get(){
    $list = array();
    foreach ($this->getMembers() as $id=>$r) $list[$r['lm_id']] = $r['lm_key'];
    return $list;
  }

  /*
   * If the packable iteme present in both packed / unpacked fields, 
   * the "unpacked" one takes precedence (i.e. those are interpreted as updates)
   */
  private function unserialize(&$r){
    return self::_unserialize($r,$this->serialized,$this->packed_items);
  }

  public static function _unserialize(&$r,$serialized_column,$packed_items){
    $packables = array();
    if (!empty($packed_items)){
      // Get updates for packables (updates are NOT serialized)
      foreach(array_intersect($packed_items,array_keys($r)) as $k) $packables[$k] = $r[$k];
      
      // Unpack the existing packables
      if (isset($r[$serialized_column])){
	foreach(($p = b_fmt::safe_decode($r[$serialized_column],True)) as $k=>$v) 
	  $p[$k] = str_replace('--LF--',"\n",
			       str_replace('--CR--',"\r",
					   str_replace('--CRLF--',"\r\n",$v)));
	foreach(array_diff(array_keys($p),array_keys($packables)) as $k){
	  if (!isset($packables[$k])) $packables[$k] = $r[$k] = $p[$k];
	}
      }
    }
    return $packables;
  }

  /*
   *
   */
  protected function message_createList(){
    if (cnf_dev && $this->name(True)=='unit Null') b_debug::internalError('unit Null');
    if (cnf_dev && $this->name(True)=='list Null') b_debug::internalError('list Null');
    myPear::MESSAGE("Creating ".$this->name(True));
  }

  
  /*
   *
   */
  protected function message_deleteMember($args){
    $id = b_fmt::joinX(', ',$args);
    $msg = "Delete $this->message_deleteMember_custom record \"$id\" from \"".$this->name().'"';
    myPear::MESSAGE($msg);
  }

  /*
   *
   */
  function name($show_listType=False){ // 
    $max_length = 99;
    if ($show_listType){
      $name[] = ($this instanceof bUnit ? 'unit' : 'list');
      $name[] = x('"',$this->getValue($this->keys['name']));
    }else{
      $name[] = $this->getValue($this->keys['name']);
    }
    $reply = b_fmt::strcut(join(' ',$name),$max_length,'...');
    return $reply;
  }
  
  /*
   * Same as the parental method, but 'asIs' is always "True".
   * The variable descriptions are not essential for the list members, set them as blanks.
   */
  function getValue($name,$asIs=True,$description_MBE=True) {
    if (!isset($this->vars[$name])) $this->defineVariable(' ',$name);
    return parent::getValue($name,True,True);
  }

  /*
   *
   */
  function getMemberValue($lm_id,$name){
    if (empty($lm_id)) $lm_id = $this->getValue($lm_id);
    if (is_numeric($lm_id) && ((int)$lm_id > 0)){
      if ($this->member_ID != $lm_id) $this->getMember($lm_id,$set_as_current_member=True);
    }elseif(is_array($lm_id)){
      $this->getMember($lm_id,$set_as_current_member=True);  
    }else{
      //      b_debug::internalError("Empty \"lm_id\" argument");
    }
    
    $reply = ($this->myPost_bList() && isset($_REQUEST[$name])
	      ? $_REQUEST[$name]
	      : @$this->formDB_member[$name]);
    return $this->getValue_return('',$reply,$this->debug);
  }
  
  /*
   *
   */
  public function __toString() {
    $id[] = get_class($this) . x('(',$this->parent_ID) . " ID=$this->ID";
    if ($this->member_ID) $id[] = "member_ID=$this->member_ID";
    return join(', ',$id);
  }
}

/*
 * Remove empty records
 */
function fix_bList(){
  $only_see = False;
  if ($only_see){
    $q = myPear_db()->query("SELECT * FROM zzz_list_members ".
			      " LEFT JOIN zzz_lists ON l_id = lm_lid ".
			      " WHERE ( lm_key = '' OR lm_key IS NULL ) AND ( lm_value = '' OR lm_value IS NULL ) ".
			      " ORDER BY l_id ");
    while($r = myPear_db()->next_record($q)) b_debug::print_r($r);
  }else{
    // just delete
    $q = myPear_db()->qquery("DELETE FROM zzz_list_members "
			     ." WHERE ( lm_key = ''   OR lm_key    IS NULL )"
			     ."   AND ( lm_value = '' OR lm_value  IS NULL ) "
			     ."   AND ( lm_option= '' OR lm_option IS NULL ) "
			     //."   AND ( lm_lid = ''   OR lm_lid    IS NULL OR lm_lid = 0) "
			     ,1);
  }
}
