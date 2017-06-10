<?php
/*
 * List which keeps links to shared rooms
 * List structure:
 * - l_parent  org_id
 * - lm_key    start of the share
 * - lm_value  end of the share
 * - lm_option array of the lease_ids
 */
locateAndInclude('bList_vm');
locateAndInclude('bForm_vm_Lease');
locateAndInclude('bForm_vm_Room');

define('bList_vm_cotenants_name','Shared accommodation');
define('bList_vm_cotenants_titl','cotenant');

class bList_vm_cotenants extends bList_vm{

  protected $isReady = True;
  protected $orderBy ='lm_key';
  protected $packed_items = array('leases');
  public    $ap    = Null;
  private   $names = array();

  /*
   * The constructor
   */
  function __construct(){
    parent::__construct(myOrg_ID);
    //    $this->checkup();
  }

  /*
   * Fix eventual mistakes in the database
   */
  private function checkup(){
    if (@$_SESSION[__method__]++) return;

    foreach($this->getMembers() as $lm_id=>$rec){
      $rec['leases'] = $this->safe_explode(@$rec['leases']);
      
      if ($damaged_entry = (($n=count($rec['leases'])) < 2)){
	$message = "$n cotenants";

      }else{
	foreach ($rec['leases'] as $lease_id){
	  if (empty($lease_id)){
	    $damaged_entry = True;
	    $message = 'empty lease_id';
	    break;
	  }else{
	    $n = count($this->getMembers(array('leases'=>$lease_id)));
	    if ($damaged_entry = ($n != 1)) $message = "lease $lease_id is used $n times";
	  }
	}
      }
      
      if ($damaged_entry){
	if (cnf_dev){
	  $this->dump();
	  $msg = b_fmt::redText("damaged entry $lm_id - $message");
	  b_debug::xxx($msg);
	  myPear::ERROR($msg);
	}	
	$this->deleteMember($lm_id);
      }
    }
  }


  /*
   * Debugging utility
   */
  function dump($lm_id=Null,$txt=''){
    static $dejaVu = 0;
    if ($dejaVu++) return;

    if (empty($lm_id)) $members = $this->getMembers();
    else               $members = $this->getMembers(array('lm_id'=>$lm_id));
    
    $list = array();
    foreach($members as $lm_id=>$rec){
      $rec['leases'] = $this->safe_explode(@$rec['leases']);
      $t = array();
      foreach ($rec['leases'] as $lease_id){
	$info  = bForm_vm_Lease::_leaseInfo($lease_id,array('a_name'));
	$t[]   = sprintf("%s(%s)",$lease_id,$info['lease_name']);
	$a_name= $info['a_name'];
      }
      $list[$lm_id] = sprintf("<tt>%s=>%s %-25s %s\n",$rec['lm_key'],$rec['lm_value'],$a_name,join(' + ',$t));
    }
    if (!empty($list)) b_debug::print_r($list,$txt);
  }
  
  /*
   * The list can't be explicitly edited by the users
   */
  function isWritable(){
    $reply = False;
    return $reply;
  }

  protected function message_createList(){
    myPear::MESSAGE('Creating new list "'.trim($this->name(True)).'" for '.myOrg_name);
  }

  protected function message_addMember($r=array()){
    myPear::MESSAGE(x('em',join(' &amp; ',$this->names)).' share room'); //  at "'.$this->ap->name().'"');
  }

  protected function message_deleteMember($args){
    $r = $this->getMember($args);
    foreach($this->safe_explode(@$r['leases']) as $lease_id) $this->message_deleteMember_sharing($lease_id,$args);
  }
  
  function message_deleteMember_sharing($lease_id,$args=array()){
    $name = bForm_vm_Lease::_leaseInfo($lease_id,True);
    if (empty($name)){ 
      myPear::WARNING_T("non-existing lease in the share ".b_fmt::joinX(',',$args)); 
    }else{ 
      myPear::MESSAGE("drop sharing for <em>$name</em>"); 
    }
  }

  private function VM_no_cotenants_text($lease_id){
    $reply = sprintf('*** NO %sCOTENANTS PLEASE ***',
		     (count(VM_cotenants()->get_cotenants($lease_id,False,'names')) ? 'MORE ' : ''));
    return $reply;
  }

  /*
   * Get list of tenants in the room within the given time period
   *
   * @param (array) $rec contains
   *  - lease_start
   *  - lease_end
   *  - a_id
   */
  function get_candidate($rec){

    // Get the current cotenants 
    if (cnf_dev){
      $current_cotenants = array();
      foreach(VM_cotenants()->get_cotenants($rec) as $lm_id=>$leases) $current_cotenants = $leases;
      b_debug::_debug("current_cotenants=".join(',',$current_cotenants));
    }

    $q = myPear_db()->query(sprintf("SELECT lease_id,lease_option,a_capacity,av_firstname,av_lastname,v_status,hut_code,lease_aid".
				    " FROM abs_leases ".
				    " LEFT JOIN abs_rooms ON lease_aid = a_id ". 
				    " LEFT JOIN abs_huts  ON a_hutid = hut_id ". 
				    " LEFT JOIN abs_visits     ON lease_vid = v_id ". 
				    " LEFT JOIN zzz_avatars    ON v_avid = av_id ". 
				    " WHERE ".join(' AND ',array("a_id = %s",
								 "lease_option NOT REGEXP '%s|%s'",
								 "lease_id != %s",					
								 "lease_start <= %s",
								 "lease_end >= %s")),
				    $rec['a_id'],VM_no_cotenants,VM_room_is_full,$rec['lease_id'],$rec['lease_end'],$rec['lease_start']),cnf_dev);
    $reply = array(VM_no_cotenants => $this->VM_no_cotenants_text($rec['lease_id']));
    while($r = myPear_db()->next_record($q)){
      if (bForm_vm_Lease::_room_is_full($r)){
	b_debug::internalError("$r[av_lastname] _room_is_full, but is in the candidates list");
      }elseif(($names=$this->get_cotenants($r,True,'info')) && (count($names) < $rec['a_capacity'])){
	$reply[join(',',array_keys($names))] = join(',',array_values($names));
      }else{
	$reply[$r['lease_id']] = bForm_Avatar::_fmtName('Lf',$r);
      }
    }
    if (count($reply) == 1) $reply = array();
    asort($reply);

    b_debug::_debug(join(', ',array_values($reply)));
    return $reply;
  }
  
  /*
   * Get cotenants (lease_id's) for the given lease
   *
   * @param  $lease_id - the lease to check for co-tenants
   * @param  $include_myself 
   * @param  $period (optional) check for this time period
   * @return array(lm_id => array(leases))
   */
  private static $cache_get_cotenants = array();
  private static $count_get_cotenants = array();
  function get_cotenants($rec,$include_myself=False,$period=array()){
    
    bTiming()->cpu(__function__);
    
    if (!is_array($rec)) $rec = array('lease_id'=>$rec);
    
    // What do we want on output?
    if($return_names = ($period === 'names')) $period = array();
    if($return_info  = ($period === 'info'))  $period = array();
    
    $cache_id = serialize($rec);
    $this->dbg('call #'.(int)(@self::$count_get_cotenants[$cache_id]++),cnf_dev);
    
    if (empty(self::$cache_get_cotenants[$cache_id])){
      $reply = $leases = $names = $periods = array();

      // the lease might present only once in the list, otherwise something is wrong
      $q = myPear_db()->qquery($sql=sprintf("SELECT lm_id,lm_option FROM zzz_list_members WHERE lm_lid=%s AND lm_option REGEXP 'leases.*%s'",
					    $this->ID,$rec['lease_id']),cnf_dev);
      while($r = myPear_db()->next_record($q)){
	$lm_id = $r['lm_id'];
	$lm_option = unserialize($r['lm_option']);
	$leases = $this->safe_explode($lm_option['leases'],True);
	foreach($leases as $l_id){
	  $l = ($l_id == $rec['lease_id'] && !empty($rec['lease_start'])
		? $rec
		: bForm_vm_Lease::_leaseInfo($l_id,array('lease_start','lease_end','av_firstname','av_lastname')));
	  $names[$l_id] = bForm_Avatar::_fmtName('Lf',$l);
	  $periods[$l_id] = array($l['lease_start'],$l['lease_end']);
	}
      }
      if (!empty($lm_id)) $reply[$lm_id] = $leases;
      self::$cache_get_cotenants[$cache_id] = array('reply'=>$reply,
						    'names'=>$names,
						    'periods'=>$periods);

    }

    // Extract reply from the cache
    if (!empty($period)){
      $names = self::$cache_get_cotenants[$cache_id]['names']; 
      foreach(self::$cache_get_cotenants[$cache_id]['periods'] as $l_id=>$p){
	if (!b_time::inInterval($period[0],$period[1], $p[0],$p[1])){
	  $lease_ref = bForm_vm_Lease::_leaseInfo($rec['lease_id'],array('lease_id'));
	  myPear::WARNING_T($lease_ref['lease_name'].' & '.$l['lease_name'].' are NOT cotenants for '.b_time::period($l['lease_start'],$l['lease_end']));
	  unset($names[$l_id]);
	}
      }
      $reply = array_values($names);
      if ($reply) $this->dbg(join(', ',$reply),cnf_dev);
    }elseif ($return_names || $return_info){
      $names = self::$cache_get_cotenants[$cache_id]['names'];
      if (!$include_myself) unset($names[$rec['lease_id']]);
      if ($return_names){
	$reply = array_values($names);
	sort($reply);
      }else{
	$reply = $names;
	ksort($reply);
      }  
      if ($reply) $this->dbg(b_fmt::joinX(', ',$reply),cnf_dev);
    }else{
      $reply = self::$cache_get_cotenants[$cache_id]['reply'];
      if (empty($reply))	$reply = array();
      foreach($reply as $lm_id=>$leases){
	if (!$include_myself) $reply[$lm_id] = array_diff($reply[$lm_id],array($rec['lease_id']));
	sort($reply[$lm_id]);
	$this->dbg("$lm_id=>array(".join(', ',$reply[$lm_id]).")",cnf_dev);
      }
    }
    bTiming()->cpu();
    return $reply;
  }

  /*
   * Impose flag VM_no_cotenants if number of cotenants reaches the room capacity
   */
  public static function set_options($leases){
    foreach($leases as $lease_id){
      $rec = bForm_vm_Lease::_leaseInfo($lease_id,array('a_capacity','hut_code','lease_id','lease_aid','lease_option'));
      if ($rec['a_capacity'] == count($leases) && !bForm_vm_Lease::get_lease_options($rec,VM_no_cotenants)){
	b_debug::_debug("$lease_id - VM_no_cotenants");
	bForm_vm_Lease::set_lease_options($rec,array(VM_no_cotenants=>1));
      }
    }
  }

  /*
   * Set cotenants
   */
  function set_cotenants($leases){
    // Set flag 'no cotenants wanted'
    if (in_array(VM_no_cotenants,$leases)){
      $this->dbg("set lease flag VM_no_cotenants for lease_id=".$leases[1],cnf_dev);
      $_leaseInfo = bForm_vm_Lease::_leaseInfo($leases[1],array('v_accompaning','av_lastname','av_firstname',
								'a_capacity','lease_aid','lease_option','lease_id','v_status','hut_code'));
      bForm_vm_Lease::set_lease_options($_leaseInfo,array(VM_no_cotenants=>1),True);
      if (($n = @$_leaseInfo['v_accompaning']) && (bForm_vm_Lease::get_lease_options($_leaseInfo,'get_pay') != VM::$pay_options[1])){
	bForm_vm_Lease::set_lease_options($_leaseInfo,array('pay'=>VM::$pay_options[1]),True);
	myPear::MESSAGE(sprintf("Impose \"%s\" for %s",
				bForm_vm_Lease::get_lease_options($_leaseInfo,'pay_room',array(),True),
				bForm_Avatar::_fmtName('LF',$_leaseInfo))); 
      }
      $leases = array();
    }
    if (empty($leases)) return;

    if (cnf_dev){
      if (($n=count($leases)) < 2) b_debug::traceBack("just $n lease requested");
      b_debug::print_r($leases,__function__.'(leases)'); 
    }
    
    $lm_id    = 'new';
    $lm_key   = +CONST_eternity_t;
    $lm_value = -CONST_eternity_t;
    $this->names = $leases_current = array();

    foreach($leases as $lease_id) {
      $leases_rec[$lease_id] = $r = bForm_vm_Lease::_leaseInfo($lease_id,array('lease_start','lease_end'));
      if (empty($r['lease_start'])) b_debug::internalError('empty lease_start',$r);
      $lm_key   = min($lm_key,  $r['lease_start']);
      $lm_value = max($lm_value,$r['lease_end']);
      $this->names[] = $r['lease_name'];
      foreach($this->get_cotenants($lease_id,True) as $id=>$leases_now){
	if ($leases_now == $leases){
	  // Check is the share already known, return if yes
	  $this->dbg("NO CHANGE in the lease");
	  return;
	}
	$lm_id = $id;
	$leases_current = array_merge($leases_current,$leases_now);
      }
    }

    // walk thru the current list of cotenants and check the changes
    foreach(array_unique($leases_current) as $lease_id){
      $this->dbg("lease $lease_id");
      if (in_array($lease_id,$leases)){
	bForm_vm_Lease::_leaseInfo($lease_id,$name_only=True,'is already there');
      }else{
	$l = bForm_vm_Lease::_leaseInfo($lease_id,array('lease_start','lease_end'));
	$toKeep = True;
	foreach($leases_rec as $l_id=>$r){
	  if (in_array($l_id,$leases_current)) continue;
	  if (b_time::inInterval($l['lease_start'],$l['lease_end'], $r['lease_start'],$r['lease_end'])){
	    $toKeep = False;
	    if (cnf_dev) myPear::DEBUG("$l[lease_name] <==> $r[lease_name]");
	    break;
	  }
	}
	if ($toKeep) {
	  $leases[] = $lease_id;
	  $this->names[] = $l['lease_name'];
	  $lm_key   = min($lm_key,  $l['lease_start']);
	  $lm_value = max($lm_value,$l['lease_end']);
	  bForm_vm_Lease::_leaseInfo($l_id,$name_only=True,'KEEPING !');
	}else{
	  $this->message_deleteMember_sharing($lease_id);
	}
      }
    }
    
    // Create or update the cotenants 
    if (!empty($this->names)){
      $this->names = array_unique($this->names);
      $leases = array_unique($leases);
      sort($leases);
      $memberID = $this->updateMember(array('lm_key'   => date('Y-m-d',$lm_key),
					    'lm_value' => date('Y-m-d',$lm_value),
					    'leases'   => join(',',$leases)),
				      $lm_id); 
      if (is_numeric($lm_id)) $this->message_addMember();
      if (cnf_dev)    $this->dump($memberID);

      // Set flag 'VM_no_cotenants' if the rooms is full
      self::set_options($leases);
    }
  }

  function getMember($args,$set_as_current_member=False){
    $reply = parent::getMember($args,$set_as_current_member);
    $reply['leases'] = $this->safe_explode(@$reply['leases']);

    if (($n=count($reply['leases'])) != ($nu=count(array_unique($reply['leases'])))) myPear::WARNING_T("$n leases, but only $nu are unique"); 
    return $reply;
  }

  protected function header($arg=Null){
    return array('period'  => '',
		 'tenants' => '');
  }

  protected function format(){
    $cotenants = array();
    foreach($this->safe_explode(@$this->r['leases']) as $lease_id){
      $cotenants[] = bForm_vm_Lease::_leaseInfo($lease_id,True);
    }
    sort($cotenants);
 
    $this->r['period']  = $this->r['lm_key'] .' => '.$this->r['lm_value'];
    $this->r['tenants'] = join('<br/>',$cotenants);
    $this->t->css['tenants']['align'] = 'align_left';
  }
}
