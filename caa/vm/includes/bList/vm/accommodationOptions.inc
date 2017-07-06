<?php
/*
 * Helper for bForm_vm_Hut & bForm_vm_Room classes.
 * Keep accommodation options available for the given event AND/OR organization
 * 
 * Member format:
 *  lm_key:    Hut ID
 *  lm_option: array
 *   a_id1 => array(rent price, etc.)
 *   ...
 *   a_idN => array(rent price, etc.)
 *  
 */
locateAndInclude('bList_vm');

/*
 *
 */
class bList_vm_accommodationOptions extends bList_vm{

  public static $_debug = True;
  var $list_quering = array('lm_key'  =>'Name',  // hut_id
			    );
  var $list_showing = array('_hut_id' => '',
			    '_TR' => '',
			    '_number' => '<br/>N tenants',
			    );
  protected $packed_items = array();
  protected $isReady  = True;

  public static $default_option = array('a_quantity'=>VM_quantity2request,
					'a_price'=>'999=9999');
  
  
  public static $whatIsSaved = array('a_price',
				     'a_quantity');
  
  /*
   *
   */
  public static function get_allHuts(){
    $q = myPear_db()->query("SELECT hut_id,hut_name FROM abs_huts WHERE hut_code != '".HUT_BI."' ORDER BY hut_code");
    $reply = array();
    while($r = myPear_db()->next_record($q)){
      var_dump($r);
      $reply[$r['hut_id']] = $r['hut_name'];
    }
    return $reply;
  }
  

  /*
   * Parent is either the Organization or an Event
   */
  function __construct($parent_ID=Null){
    if ($this->debug) b_debug::_debug(starting);

    //    This class makes sense for the events only...
    //    Difficult to imaging how it might be used for the "out of events" visits...
    //    (however, the code supports it)
    if ($parent_ID == myOrg_ID){
      if (cnf_localhost) b_debug::internalError("Think twice, ".__class__."(myOrg) does not really make sense...");
      for ($n=1;$n<10;$n++) myPear::WARNING(str_repeat('?',$n)." Think twice, ".__class__."(myOrg) does not really make sense...");
      for ($n=10;$n>0;$n--) myPear::WARNING(str_repeat('!',$n)." Think twice, ".__class__."(myOrg) does not really make sense...");
      b_debug::traceBack();
    }

    parent::__construct($parent_ID);

    // Workaround, TBD better
    //    Clean up ghosts
    myPear_db()->qquery("DELETE FROM zzz_list_members WHERE lm_lid=".$this->ID." AND (lm_key ='' OR lm_key IS NULL)",True);

    $this->title = (($parent_ID == myOrg_ID)
		    ? myOrg_name.' accommodation options'
		    : $this->e->name());
    $this->dbg($this->title);

    // Wipe out the list if requested
    if (!empty($_GET['reset_once']) || !$this->getMembers()){
      $this->deleteMember(array('lm_lid'=>$this->ID),$verbose=True);
    }

    // Populate the list if not yet done
    $this->update();

    // Build rent prices for the selected accommodations
    $this->build_dynamic_packables();
    if ($this->debug) b_debug::_debug(exiting);
  }

  private static $id2x_prefix = 'r';
  public  static function id2x($a_id){
    $reply = self::$id2x_prefix . $a_id;
    return $reply;
  }

  /*
   * Get value
   */
  public function get_a_value($a_id,$hut_id,$what=False){
    if (!(int)$a_id || !(int)$hut_id)          b_debug::internalError("Empty input argument(s)");
    $r = $this->getMember(array('lm_key'=>$hut_id));
    $packed_value = @$r[self::id2x($a_id)];
    if ($what === True){
      $reply = $packed_value;
    }else{
      if ($packed_value === Null){
	$reply = Null;
      }else{
	if (strpos($packed_value,'#') === False)b_debug::internalError('?',$r);
	list($a_quantity,$a_price) = explode('#',$packed_value);
	$reply = ($what === 'a_quantity'
		  ? $a_quantity
		  : $a_price);
      }
    }
    $this->dbg(self::id2x($a_id).'='.var_export($reply,True),cnf_dev);
    return $reply;
  }

  /*
   * Set value
   */  
  public function set_a_value($new_data,$a_id,$hut_id,$what='a_price'){
    if (!in_array($what,self::$whatIsSaved))  b_debug::internalError("??? \"$what\" is unexpected");
    $r = $this->getMember(array('lm_key'=>$hut_id));
    $oldValue = $this->get_a_value($a_id,$hut_id,True);
    
    if (!empty($oldValue) && strpos($oldValue,'#')==False) b_debug::traceBack("oldValue=$oldValue");
    if (empty($oldValue)){
      $this->dbg('??? empty value for $a_id');
      if (!isset($r[self::id2x($a_id)])) $this->add_room($hut_id,$a_id);
      $oldValue = implode('#',array(self::$default_option['a_quantity'],self::$default_option['a_price']));;
    }
    list($a_quantity,$a_price) = explode('#',$oldValue);
    $$what = $new_data;
    $newValue = implode('#',array($a_quantity,$a_price));
    
    $update = array(self::id2x($a_id) => $newValue);
    $this->updateMember($update,$r['lm_id'],False);
    $this->dbg($update,cnf_dev);
  }
  
  /*
   *
   */
  protected function message_createList(){
    myPear::MESSAGE("Creating ".$this->name(True).' for '.$this->title);
  }
  
  /*
   *
   */
  function getFormAdd_OK(){
    if ($this->parent_ID != myOrg_ID){
      
      $see_here = '';
      static $dejaVu=0; if (!$dejaVu++) myPear::INFO(["Select the accommodation option, click <em>\"continue ...\"</em>, after that select the desired hotel.",
						      "Note, you can select only hotels defined by the ".myOrg_name." accommodation $see_here, ".
						      "so a new building should be first added to the  ".myOrg_name." list."]);
    }
    
    if ($reply = parent::getFormAdd_OK()){
      $this->isMBE['hut_code'] = True;
    }
    
    $this->dbg($reply);
    return $reply;
  }

  /*
   * This list might be administrated by the event organizers
   */
  function isAdmin(){
    return VM_prg_coordinator_here || VM_organizer_here;
  }
  
  /*
   * Deletable if no tenants in aps of this type 
   */
  protected function isMemberDeletable(){
    return b_cnf::check_conditions(array(True =>array('??'  =>empty($this->r['lm_key'])),
					 False=>array('used'=>$this->get_occupiedRooms($this->r['lm_key'])),
					 'default'=>array($this->isWritable())),
				   $this);
  }
	     
  /*
   * Should that be editable???
   */
  protected function isMemberEditable(){
    //    $reply = $this->isWritable();
    $reply = False;
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * Skip the build-in objects (like "own accommodation", etc.)
   */
  function rejectRecord(){ 
    return ($this->r['hut_code'] == HUT_BI);
  }

  /*
   *
   */
  function createMember(){
    $this->dbg(__method__,True);
    $lm_id = parent::createMember();
    $lm_key = $this->getValue('lm_key');

    $this->updateMember(array('lm_option'=>serialize(array()),
			      'lm_key'   =>$lm_key),
			$lm_id);

    // Update the list of rooms 
    $this->update($lm_key);

    return $lm_id;
  }

  /*
   *
   */
  protected function getFormAdd(){
    if ($this->debug) b_debug::_debug(starting);

    $this->bForm($this->list_quering,True);
    $this->f->tips = array();
    $this->f->defineVariable('Accommodation option','hut_code');

    // Select accommodation which is available for the event duration
    if (!$this->getValue('lm_id')){
      // Remove the already used huts from selection
      $excludes = array(); 
      foreach($this->getMembers() as $x=>$r){
	$hut_id = $this->getMemberValue($this->getValue('lm_id'),'lm_key');
	if ($hut_id != $r['lm_key']) $excludes[] = $r['lm_key'];
      }
      $this->dbg("excludes=".join(', ',$excludes));      

      // Select those huts which are "valid" from today for another year (or during the event time period)
      list($from,$to) = (is_object($this->e)
			 ? array($this->e->getValue('e_start'),
				 $this->e->getValue('e_end'))
			 : array(time(), 
				 time()+365*24*3600));
      
      locateAndInclude('bForm_vm_Lease');
      $this->f->selectMenu('lm_key',bForm_vm_Lease::_freeRooms($excludes,
							       array('lease_start'=>$from,
								     'lease_end'  =>$to),
							       False,
							       'all'));
      if (!$this->getValue('lm_id')) $this->f->textHidden('lm_id','new');
    }
    $this->f->closeForm();
    if ($this->debug) b_debug::_debug(exiting);
  }
  
  /*
   * Helper function
   */
  private function hutSelectMenu(){
    
  }
  
  /*
   * Get the hut name
   */
  private $hut = array();
  private function getHutInfo($hut_id,$what='name'){
    if (empty($hut_id)){
      $reply = '';
    }else{
      if (empty($this->hut[$hut_id])) $this->hut[$hut_id] = myPear::getInstance_new('bForm_vm_Hut',$hut_id,'fatal');
      switch($what){
      case 'name':    $reply = $this->hut[$hut_id]->name();               break;
      case 'hut_code':$reply = $this->hut[$hut_id]->getValue('hut_code'); break;
      case 'capacity':$reply = $this->hut[$hut_id]->getCapacity();        break;
      default:
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  protected function message_addMember_name($r){
    return $this->getHutInfo($r['lm_key']);
  }

  /*
   * Do not build tabs for this list, it is used as a tab element
   */
  private $getMembers_forListing = array();
  function show($av_id=''){
    if (empty($av_id)){
      $this->hookJOIN = join(' ',array("LEFT JOIN abs_huts ON hut_id = lm_key",
				       ));
      $this->orderBy     = 'hut_code';
      $this->orderByDir  = 'ASC';
    }else{
      $this->debug = True;
      $this->hookJOIN = join(' ',array("LEFT JOIN abs_huts ON hut_id = lm_key",
				       "LEFT JOIN abs_rooms ON a_hutid = hut_id",
				       "LEFT JOIN abs_leases ON lease_aid = a_id",
				       "LEFT JOIN abs_visits ON lease_vid = v_id"));
      $this->where_eq = array("v_host_avid = '$av_id'");
    }
    if ($this->parent_ID == myOrg_ID){
      print x('h4',"Statistics for ".$this->title);
    }else{
      print x('h4',$this->e->name());
    }
    parent::show(' ');
  }

  private $hookJOIN = '';
  protected function hookJOIN(){
    $reply = array(parent::hookJOIN());
    if (!empty($this->hookJOIN)) $reply[] = $this->hookJOIN;
    return join(' ',$reply);
  }

  /*
   * Print a simple table of 2 columns showing usage of the acc. option
   */
  function show_usage($hut_code,$lease_start=Null,$lease_end=Null,$v_host_avid=Null){
    //    b_debug::traceBack($hut_code);
    foreach($this->getMembers() as $lm_id=>$r){
      if(($n = count($this->get_occupiedRooms($r['lm_key'],$lease_start,$lease_end,$v_host_avid))) == 0) continue;
      if($hut_code != $this->getHutInfo($r['lm_key'],'hut_code'))   continue;
      
      $hut = myPear::getInstance_new('bForm_vm_Hut',$r['lm_key'],'fatal');
      if ($hut_code != $hut->getValue('hut_code')) continue;
      
      if (empty($t)){
	$t = new b_table_zebra(array('n'=>' ','acc'=>' '));
	$t->showLineCounter = False;
      }
      $acc = $this->getHutInfo($r['lm_key']);
      $t->prt(array('n'  =>$n,
		    'acc'=>trim(str_ireplace('hotel','',$acc))));
    }
    if (!empty($t)) $t->close();
  }

  /*
   * Do not show buttons if there is no huts left for the selection
   */
  protected function draw_buttons(){
    $reply = parent::draw_buttons();
    if (!empty($this->e) && $this->isWritable()) $reply[] = b_btn::submit('reset to default',
									  b_url::same('?reset_once=y'),
									  $confirm=True);
    return $reply;
  }

  /*
   *
   */
  private $setPageHeader_dejaVu = 0;
  protected function setPageHeader(){
    if (!$this->setPageHeader_dejaVu++){
      $header = b_fmt::unescape($this->title.' usage');
      myPear::setPageHeader($header);
      $this->dbg($header,cnf_dev);
    }
  }

  /*
   * Format member record for the list
   */
  protected function format($secondCall=False){
    locateAndInclude('bForm_vm_Hut');
    static $ao_was = '?';

    // Split by the accommodation options
    if ($ao_was != ($ao_now=$this->getHutInfo($this->r['lm_key'],'hut_code'))){
      if (empty($ao_now)){
	b_debug::var_dump($this->r,'??? empty "$ao_now"');
	$this->r = array();
	return;
      }
      $this->t->showLineCounter = False;
      print $this->t->tr(x('strong',VM::$known_hut_codes[$ao_now]['prefix'].'s'),'colspan=3');
      $ao_was = $ao_now;
    }
    
    $hut = myPear::getInstance_new('bForm_vm_Hut',$this->r['hut_id'],'fatal');
    $this->r['_TR'] = $hut->name_total_rooms($this);
    
    $this->r['_hut_id'] = x(sprintf("a href='%s'",b_url::same(sprintf("?function=bHolder_vm_Rooms::list_hut&arg_once=%s&arg2_once=%s",$this->r['lm_key'],$this->e->ID))),
			    $this->getHutInfo($this->r['lm_key']));
    
    $_number  = count($this->get_occupiedRooms($this->r['lm_key']));
    //    $this->r['_rate']   = $this->r['lm_option'];
    $this->r['_number'] = (empty($_number)
			   ? ' '
			   : ' '.$_number);
  }
  
  /*
   * Update list of accommodation options (or available huts) for the given event
   */
  private function build_dynamic_packables(){
      
    // We do not know yet the list of packables, hence
    // keep yet the serialized options packed and not break the unpacking procedure
    $this->unserialize_packables = False;
    $packed_items = array();
    foreach($this->getMembers() as $lm_id=>$r){
      foreach(b_fmt::safe_decode($r['lm_option'],True) as $a_id=>$data) $packed_items[] = $a_id;
    }
    $this->unserialize_packables = True;

    // See the result
    $this->update_packed_items($packed_items);
  }

  /*
   *
   */
  public function update($hut_id=null){
    
    $queries = array();
    // Initialize the list
    if (!$this->getMembers()){
      // first select the huts where participants already accommodated
      $queries[] = ('FROM abs_visits '.
		    ' LEFT JOIN abs_leases ON v_id = lease_vid '. 
		    ' LEFT JOIN abs_rooms ON a_id = lease_aid '.
		    ' LEFT JOIN abs_huts ON hut_id = a_hutid '. 
		    ' LEFT JOIN abs_events ON e_id = v_eid '.
		    ' WHERE v_eid = "'.$this->parent_ID.'"');
      
      // then get the default huts and include them in the event list
      foreach(b_checkBox::$ON as $on) $s_isON[] = "hut_status = '$on'";
      $queries[] = ('FROM abs_huts '.
		    ' LEFT JOIN abs_rooms ON hut_id = a_hutid WHERE ('.join(' OR ',$s_isON).') ');
    }

    // add a new hut if requested
    if (!empty($hut_id)){      
      $queries[] = ('FROM abs_huts '.
		    ' LEFT JOIN abs_rooms ON hut_id = a_hutid WHERE hut_id = '.$hut_id);
    }
    
    // Build the updates list
    $updates = $packed_items = array();
    foreach($queries as $query){
      $q = myPear_db()->qquery("SELECT hut_id,hut_name,a_id,a_price,a_quantity,a_name $query AND hut_code != '".HUT_BI."' ORDER BY CONCAT(hut_id,a_id)",cnf_dev);
      while($r = myPear_db()->next_record($q)){
	$id = self::id2x($r['a_id']);
	$packed_items[] = $id;
	if (@$hut_was !== $r['hut_id']){
	  $this->dbg($r['hut_name'],cnf_dev); 
	  $lm_id = $this->addMember(array('lm_key'=>$r['hut_id']));
	  $r_now = $this->getMember($lm_id);
	  $hut_was = $r['hut_id'];
	}
	// Do NOT replace the existing entries, just add new ones if any
	if (empty($r_now[$id])){
	  $this->dbg("adding $r[a_name]",cnf_dev);
	  $updates[$lm_id][$id] = $r['a_quantity'].'#'.$r['a_price'];
	}
      }     
    }

    $this->update_packed_items($packed_items);
    
    foreach($updates as $lm_id=>$lm_option){ 
      $this->updateMember(array('lm_option'=>serialize($lm_option)), $lm_id);
    }  
  }

  /*
   *
   */
  private function update_packed_items($packed_items){
    $packed_items     = array_unique($packed_items);
    $packed_items_was = array_unique($this->packed_items);
    sort($packed_items);
    sort($packed_items_was);
    
    if ($packed_items !== $packed_items_was){
      b_debug::_debug(join('<br>',array('was/now',
					b_debug::parse_value($packed_items_was,10),
					b_debug::parse_value($packed_items,10))),
		      cnf_dev);
      $this->packed_items = $packed_items;
    }
  }
  
  /*
   * input - array(hut_id => array(a_id =>array(a_quantity=>value,
   *                                            a_price=> =>value)
   */
  private function populate($input){

    // Populate now...
    b_debug::print_r($input,'input');

    // Convert input array to format suitable for the list
    $updates = array();
    foreach($input as $hut_id=>$data){
      $lm_id = $this->addMember(array('lm_key'=>$hut_id));
      $r_now = $this->getMember($lm_id);
      $lm_option = array();
      foreach($data as $a_id=>$r){
	$id = self::id2x($a_id);
	$this->packed_items[] = $id;
	// Do NOT replace the existing entries, just add new ones if any
	if (empty($r_now[$id])){
	  $lm_option[$id] = $r['a_quantity'].'#'.$r['a_price'];
	}
      }
      $this->packed_items = array_unique($this->packed_items);
      sort($this->packed_items);
      $this->updateMember(array('lm_option'=>serialize($lm_option)), $lm_id);
    }
  }
  
  /*
   *    $this->add_room($hut_id,$a_id,array('a_quantity'=>,'a_price'=>'999=9999');
   */
  private function add_room($hut_id,$a_id,$option=array()){

    if (empty($option)) $option = self::$default_option;

    $id = self::id2x($a_id);
    if (!in_array($id,$this->packed_items)) $this->packed_items[] = $id;
    
    $lm_id = $this->addMember(array('lm_key'=>$hut_id));
    $this->updateMember(array($id=>$option['a_quantity'].'#'.$option['a_price']),
			$lm_id);
    
    $this->dbg("add '$id' to lm_option");
  }      
  
  /*
   * Get "rooms" for the build-in options
   */
  function get_BI($return_a_code=False){
    $reply = array();
    $q = myPear_db()->query("SELECT a_id,a_name,hut_code FROM abs_huts LEFT JOIN abs_rooms ON a_hutid = hut_id WHERE hut_code = '".HUT_BI."'");
    while($r = myPear_db()->next_record($q)){
      if ($return_a_code){
	locateAndInclude('bForm_vm_Lease');
	$reply[$r['a_id']] = bForm_vm_Lease::_hut_code($r);
      }else{
	$reply[$r['a_id']] = $r['a_name'];
      }
    }
    return $reply;  
  }

  /*
   *
   */
  public function get_occupiedRooms($hut_id=Null,$lease_start=Null,$lease_end=Null,$v_host_avid=Null){
    
    $where = array("hut_code != '".HUT_BI."'");
    if ($this->parent_ID != myOrg_ID)   $where[] = 'e_id = '.$this->parent_ID;
    if (!empty($hut_id))                $where[] = "hut_id = $hut_id";
    if (!empty($v_host_avid))           $where[] = "v_host_avid = $v_host_avid";
    if (!empty($lease_start) && !empty($lease_end)){
      $where[] = "lease_end >= $lease_end ";
      $where[] = "lease_start <= $lease_start";
    }
    
    $q = myPear_db()->qquery('SELECT lease_id,a_name FROM abs_visits '.
			     ($this->parent_ID == myOrg_ID ? '' : ' LEFT JOIN abs_events ON e_id = v_eid '). 
			     ' LEFT JOIN abs_leases ON v_id = lease_vid '. 
			     ' LEFT JOIN abs_rooms ON a_id = lease_aid '.
			     ' LEFT JOIN abs_huts ON hut_id = a_hutid '. 
			     ' WHERE '.join(' AND ',$where).
			     ' GROUP BY lease_id',cnf_dev);
    $reply = array();
    while($r = myPear_db()->next_record($q)) $reply[$r['lease_id']] = $r['a_name'];
    $this->dbg(join(',&nbsp;',array_unique(array_values($reply))),cnf_dev);
    return $reply;
  }
} 
    
