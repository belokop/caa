<?php
/*
 * Helper for bForm_vm_Hut & bForm_vm_Room classes.
 * Keep accommodation options available for the given event AND/OR organization
 * 
 * Member format:
 *  lm_key:    Hut ID
 *  lm_option: array
 *   nrooms    - Number of apartments available (obsolete since 2017-05-01)
 *   a_id1   - rent price, etc.
 *   ...
 *   a_idN   - rent price, etc.
 *  
 */
locateAndInclude('bList_vm');

/*
 *
 */
class bList_vm_accommodationOptions extends bList_vm{

  var $list_quering = array('lm_key'  =>'Name',  // hut_id
                            'nrooms'  =>'Number of rooms allocated',
			    );
  var $list_showing = array('_hut_id' => '',
                            '_nrooms' => 'N rooms<br/>available',			    
			    // '_rate'   => '<br/>Rate',
			    '_number' => '<br/>N tenants',
			    );
  protected $packed_items = array('nrooms');
  protected $isReady  = True;

  public static $whatIsSaved = array('a_price',
				     'a_quantity');
  
  /*
   *
   */
  public static function get_allHuts(){
    $q = myPear_db()->query("SELECT hut_id,hut_name FROM abs_huts WHERE hut_code != '".HUT_BI."' ORDER BY hut_code");
    $reply = array();
    while($r = myPear_db()->next_record($q)){
      $reply[$r['hut_id']] = $r['hut_name'];
    }
    return $reply;
  }
  

  /*
   * Parent is either the Organization or an Event
   */
  function __construct($parent_ID=Null){

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
      $this->update();
    }

    // Build rent prices for the selected accommodations
    $this->build_dynamic_packables();
  }

  /*
   * We pack array("roomID" => "room prices") for the event in this class.
   * Since the roomID's are not known in advance, we build the list packed items on the fly. 
   */
  private function build_dynamic_packables(){
    
    // Add the dynamic packables to the list
    if(!empty($this->e) && $this->getMembers()){
      
      // We do not know yet the list of packables, hence
      // keep yet the serialized options packed and not break the unpacking procedure
      $this->unserialize_packables = False;
      $updates = array();
      
      foreach($this->getMembers() as $lm_id=>$rr){
	
	if (empty($rr['lm_key'])){
	  b_debug::traceBack($m="??? empty \$rr[lm_key]");
	  b_debug::var_dump($rr,$m);
	  continue;
	}
	
	// Loop over all the rooms in the hut
	$q = myPear_db()->query(sprintf("SELECT a_id,a_price,a_name,".(upgraded_rooms_specifications?'a_quantity,':'')."hut_name FROM abs_rooms ".
					"LEFT JOIN abs_huts ON hut_id = a_hutid".
					" WHERE a_hutid = %s ".
					" AND a_end >= %s ".
					" AND ( a_start <= %s OR a_start = 0 ) ".
					" AND hut_code != '%s' ORDER BY CONCAT(hut_name,a_name)",
					$rr['lm_key'],$this->e->getValue('e_end'),$this->e->getValue('e_start'),HUT_BI));
	$lm_option = b_fmt::safe_decode($rr['lm_option'],True);
	while ($r = myPear_db()->next_record($q)){
	  // Add the a_id to the packables list
	  $id = self::id2x($r['a_id']);
	  $this->packed_items[] = $id;
	  // Pack the rent price into the acc.options 
	  if (!in_array($id,array_keys($lm_option))){
	    $this->dbg("add '$id' to lm_option");
	    $updates[$lm_id][$id] = (upgraded_rooms_specifications
				     ? $r['a_quantity'].'#'.$r['a_price']
				     : $r['a_price']);
	    if ($r['hut_name'] != @$hut_name){
	      myPear::MESSAGE(x('strong',sprintf("Setting up %s",$r['hut_name'])));
	      $hut_name = $r['hut_name'];
	    }
	    myPear::MESSAGE(x('em','&nbsp;&nbsp;&nbsp;'.$r['a_name']));
	  }
	}
      }	  
      
      
      // Update the list, fill the prices
      $this->unserialize_packables = True;
      foreach($updates as $lm_id=>$update){
	// Remove first the packable
	$this->updateMember(array('lm_option'=>''),$lm_id,False);
	// Update it now
	$this->updateMember(array('lm_option'=>serialize($update)),$lm_id,False);
      }
    }
  }
  
  private static $id2x_prefix = 'r';
  public  static function id2x($a_id){
    $reply = self::$id2x_prefix . $a_id;
    return $reply;
  }

  public function get_a_value($a_id,$hut_id,$what=False){
    if (!(int)$a_id || !(int)$hut_id)          b_debug::internalError("Empty input argument(s)");
    $r = $this->getMember(array('lm_key'=>$hut_id));
    $a = @$r[self::id2x($a_id)];
    $this->dbg($a,cnf_dev);
    if ($what === True){
      $reply = $a;
    }elseif (upgraded_rooms_specifications){
      list($a_quantity,$a_price) = (strpos($a,'#')===False
				    ? array(0,0)
				    : explode('#',$a));
      $reply = ($what === 'a_quantity'
		? $a_quantity
		: $a_price);
    }else{
      $reply  = $a;
    }
    if (empty($reply)) $reply = 0;  // Build-in objects presumably
    $this->dbg($reply,cnf_dev);
    return $reply;
  }
  
  public function set_a_value($new_data,$a_id,$hut_id,$what='a_price'){
    if (!in_array($what,self::$whatIsSaved))  b_debug::internalError("??? \"$what\" is unexpected");
    $r = $this->getMember(array('lm_key'=>$hut_id));
    $oldValue = $this->get_a_value($a_id,$hut_id,True);
    if (upgraded_rooms_specifications){
      if (strpos($oldValue,'#')===False) var_dump($oldValue);
      list($a_quantity,$a_price) = (strpos($oldValue,'#')===False
				    ? array(0,0)
				    : explode('#',$oldValue));
      $$what = $new_data;
      $newValue = implode('#',array($a_quantity,$a_price));;
    }else{
      $newValue = $new_data;
    }
    $update = array(self::id2x($a_id) => $newValue);
    $this->updateMember($update,$r['lm_id'],False);
    $this->dbg($update);
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
      $see_here = x(sprintf('a href="%s"',b_url::same('?q=/vm/1110161754/3055031512')),'(see_here)');;
      $see_here = '';
      myPear::INFO(["Select the accommodation option, click <em>\"continue ...\"</em>, after that select the building.",
		    "Note, you can select only buildings defined by the ".myOrg_name." accommodation $see_here, ".
		    "so a new building should be first added to the  ".myOrg_name." list."]);
    }
    
    $lm_id    = $this->getValue('lm_id');
    $this->dbg("lm_id='$lm_id'");
    //    $this->adjust_nRooms($lm_id,$this->getMemberValue($lm_id,'lm_key'),True);
    if ($reply = parent::getFormAdd_OK()){
      $this->isMBE['hut_code'] = True;
      $this->isMBE['nrooms'] = True;
    }
    
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
   */
  private function adjust_nRooms($lm_id,$hut_id,$reset_ifChanged){
    $capacity = $this->getHutInfo($hut_id,'capacity');
    $n = ($reset_ifChanged ? $this->getMemberValue($lm_id,'nrooms') : @$this->r['nrooms']);
    if ($n != $capacity) b_debug::xxx("lm_id=$lm_id n=$n capacity=$capacity");
    if ($n > $capacity){
      if ($reset_ifChanged) myPear::WARNING($msg="$n exceeds the maximum capacity of \"".$this->getHutInfo($hut_id)."\" ($capacity rooms)");
      $nrooms = $capacity;
    }elseif(empty($n)){
      $nrooms = $capacity;
    }else{
      $nrooms = $n;
    }

    if ($reset_ifChanged){
      if (($n != $nrooms) && ($reset_ifChanged === True)){
	$this->getValue_reset('nrooms',$nrooms);
	$this->r['nrooms'] = $nrooms;
      }
    }else{
      if ($nrooms >= VM_quantity2request) $this->r['_nrooms'] = '<em>no limit</em>';
      elseif (empty($nrooms))        $this->r['_nrooms'] = ' ';
      else                           $this->r['_nrooms'] = ' '.$nrooms;
    }
    $this->dbg($nrooms);
    return $nrooms;
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
  protected function getFormAdd(){

    $this->bForm($this->list_quering,True);
    $this->f->tips = array();
    $this->f->defineVariable('Accommodation option','hut_code');

    // Create an empty record for the newly added acc. option
    $lm_id = $this->getValue('lm_id');
    if (empty($lm_id)){
      $lm_id = $this->createMember();
      $this->getValue_reset('lm_id',$lm_id);
    }
    $this->f->textHidden('lm_id',$lm_id);

    // Add hut_code from the hut_id 
    $hut_id = $this->getMemberValue($lm_id,'lm_key');
    if (!empty($hut_id)){
      $hut_code = $this->formDB['hut_code'] = $this->getHutInfo($hut_id,'hut_code');
      $this->f->textHidden('lm_key',$hut_id);
    }
    
    // Select accommodation which is available for the event duration
    $VM_hutCodes = VM_hutCodes()->getCodes(($this->parent_ID == myOrg_ID
					    ? Null
					    : bList::getListInstance(myOrg_ID,__CLASS__)),
					   ($this->parent_ID == myOrg_ID
					    ? array(time(),time()+10*365*24*3600)
					    : array($this->e->getValue('e_start'),$this->e->getValue('e_end'))));
    $hut_code = $this->getValue('hut_code');
    if (empty($hut_code)){
      // Ask for the accommodation type
      $this->f->selectMenu('hut_code',$VM_hutCodes);
    }else{
      // Ask for the hut from the chosen accommodation type
      $this->f->isRO['hut_code'] = True;
      $this->f->formDB['nrooms'] = $this->getMemberValue($lm_id,'nrooms');
      $this->f->selectMenu('hut_code',$VM_hutCodes);
      if (empty($hut_id)){
	$this->hutSelectMenu($hut_code);
      }else{
	$this->f->formDB['_hut_id'] = $this->getHutInfo($hut_id,'name');
	$this->f->isRO['_hut_id'] = True;
	$this->f->defineVariable(VM_hutCodes()->getName($hut_code),'_hut_id');
	$this->f->textField('_hut_id');
      }
      if ($hut_code == HUT_H) $this->f->textHidden('nrooms',999);
      else                    $this->f->textField('nrooms');
    }
    $this->f->closeForm();
  }
  
  /*
   * Helper function
   */
  public function hutSelectMenu($hut_code,$show_menu=True){
    
    // Remove already used huts from the selection
    $excludes = array(); 
    foreach($this->getMembers() as $lm_id=>$r){
      $hut_id = $this->getMemberValue($this->getValue('lm_id'),'lm_key');
      if ($hut_id != $r['lm_key']) $excludes[] = $r['lm_key'];
    }
    
    // Select those huts which are "valid" from today for another year (or during the event time period)
    if (is_object($this->e)) list($from,$to) = array($this->e->getValue('e_start'),$this->e->getValue('e_end'));
    else                     list($from,$to) = array(b_time::noon(), b_time::noon()+365*24*3600);
    
    locateAndInclude('bForm_vm_Hut');
    $huts = bForm_vm_Hut::getHutsByCode($hut_code,$from,$to,$excludes);
    
    if($show_menu)  $this->f->selectMenu('lm_key',$huts);
    else            return $huts;
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
    //    $huts = $this->hutSelectMenu(Null,True);
    //    if (!empty($huts)) 
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

    //    $this->adjust_nRooms($this->r['lm_id'],$this->r['lm_key'],False);
    $this->r['_hut_id'] = x(sprintf("a href='%s'",b_url::same(sprintf("?function=bHolder_vm_Rooms::list_hut&arg_once=%s&arg2_once=%s",$this->r['lm_key'],$this->e->ID))),
			    $this->getHutInfo($this->r['lm_key']));
    
    $_number  = count($this->get_occupiedRooms($this->r['lm_key']));
    $this->r['_rate']   = $this->r['lm_option'];
    $this->r['_number'] = (empty($_number)
			   ? ' '
			   : ' '.$_number);
  }
  
  /*
   * Update list of accommodation options (or available huts) for the given event
   */
  public function update(){
    $this->dbg();    

    // Is it the first entry?
    $list_empty = !$this->getMembers();
    
    locateAndInclude('b_checkBox.inc');
    foreach(b_checkBox::$ON as $on) $s_isON[] = "hut_status = '$on'";
    foreach(array(
		  // first select the huts where participants already accommodated
		  'hut_id,hut_capacity'.(($this->parent_ID == myOrg_ID)?'':',e_end') => 'FROM abs_visits '.
		  ' LEFT JOIN abs_leases ON v_id = lease_vid '. 
		  ' LEFT JOIN abs_rooms ON a_id = lease_aid '.
		  ' LEFT JOIN abs_huts ON hut_id = a_hutid '. 
		  (($this->parent_ID == myOrg_ID)
		   ? ' WHERE (v_type = "'.VISIT_TYPE_COLLABORATION.'" OR v_type = "'.VISIT_TYPE_OTHER.'")'
		   : ' LEFT JOIN abs_events ON e_id = v_eid WHERE v_eid = "'.$this->parent_ID.'"'),
		  
		  // then get the default huts and include them in the event list
		  'hut_id,hut_capacity' => 'FROM abs_huts WHERE ('.join(' OR ',$s_isON).') '
		  ) as $items=>$query){
      $event_completed = False;
      $q = myPear_db()->query("SELECT $items $query AND hut_code != '".HUT_BI."' GROUP BY hut_id");
      while($r = myPear_db()->next_record($q)){
	$lm_id = $this->addMember(array('lm_key'=>$r['hut_id']));
	if (!upgraded_rooms_specifications){
	  $m = $this->getMember($lm_id);
	  if (empty($m['nrooms'])) $this->updateMember(array('nrooms'=>$r['hut_capacity']),$lm_id);
	}
	if(!empty($r['e_end']) && ($r['e_end'] < time())) $event_completed = True;
      }
      // get the default huts only when the list is empty!
      if (!$list_empty || $event_completed) break;
    }

    // (re)build rent prices for the selected accommodations
    $this->build_dynamic_packables();
  }
  
  /*
   * Get "rooms" for the build-in options
   */
  function get_BI($return_a_code=False){
    $reply = array();
    $q = myPear_db()->query("SELECT * FROM abs_huts LEFT JOIN abs_rooms ON a_hutid = hut_id WHERE hut_code = '".HUT_BI."'");
    while($r = myPear_db()->next_record($q)){
      if ($return_a_code){
	locateAndInclude('bForm_vm_Lease');
	$reply[$r['a_id']] = bForm_vm_Lease::_hut_code($r);
      //$reply[$r['a_id']] = $r['a_code'];
      }else{
	$reply[$r['a_id']] = $r['a_name'];
      }
    }
    return $reply;  
  }


  /*
   * Check the pool and get a list of available rooms in the hut for the given dates & event 
   *  public function get_occupiedRooms($hut_id=Null,$lease_start=Null,$lease_end=Null,$v_host_avid=Null){
   */
  public static function get_availableRooms($accommodationOptions,$rec,$hasAccommodation){

    list($start,$end) = (empty($rec['lease_start'])
				     ? array($rec['v_start'],    $rec['v_end'])
				     : array($rec['lease_start'],$rec['lease_end']));
    b_debug::xxx(b_time::period($start,$end));

    // Chase for the associated event ID, since VM::_MENU()-> context 
    // might give a wrong answer for the search results.
    if (!is_object($accommodationOptions) && ($e_id = @$rec['v_eid'])){
      b_debug::xxx("Recover e_id = $e_id");
      $accommodationOptions = VM_accommodationOptions($e_id);
    }
    
    $huts = array();
    $org_context = !is_object($accommodationOptions);
    if ($org_context){
      // Check ALL available rooms
      foreach(self::get_allHuts() as $hut_id=>$hut_name) $huts[] = $hut_id;
      b_debug::xxx("org_context !");
    }else{
      // Use only those "huts" which are selected by the administration, i.e. listed in vm_accommodationOptions
      foreach($accommodationOptions->getMembers() as $x=>$r) $huts[] = $r['lm_key'];
    }
    
    //Loop over all the huts for the event
    $rooms = $n = $fully_booked = array();
    foreach($huts as $hut_id){
      $hut = new bForm_vm_Hut($hut_id);
      // Loop over all the rooms 
      $q_r = myPear_db()->qquery(sprintf("SELECT a_id,a_price,a_name,a_quantity,a_capacity,a_quantity FROM abs_rooms ".
					 " WHERE a_end >= %s ".
					 " AND ( a_start <= %s OR a_start = 0 OR a_start IS NULL) ".
					 " ORDER BY CONCAT(a_capacity) ",
					 $start,$end),cnf_dev);
      while ($r_room = myPear_db()->next_record($q_r)){
	// Room ID
	$id = sprintf("%s %s&#91;%s&#93;",$hut->name(),$r_room['a_name'],$r_room['a_capacity']);
	$rooms[$id] = 0;
	
	// How many rooms of this type have we for the event?
	$a_quantity = ($org_context
		       ? $r_room['a_quantity']
		       : $accommodationOptions->get_a_value($r_room['a_id'],$hut->ID,'a_quantity'));
	if (empty($a_quantity)) b_debug::traceBack();
	if (0)if (empty($a_quantity)) b_debug::internalError("??? Strange, empty a_quantity",array('accommodationOptions'=>($org_context?'':$accommodationOptions->__toString()),
												   'org_context'=>$org_context,
												   'a_quantity'=>$a_quantity,
												   'r_room'=>$r_room,
												   'rooms'=>$rooms,
												   'n'=>$n));
	$n[$id] = array($hut->ID,$r_room['a_id'],$a_quantity);
	
	// Loop over leases for the room
	$q = myPear_db()->qquery($sql=sprintf("SELECT lease_id,lease_option FROM abs_leases ".
					      " WHERE lease_aid = %s ".
					      " AND lease_start <= $end ".
					      " AND lease_end   >= $start ",
					      $r_room['a_id']),True);
	while ($r = myPear_db()->next_record($q)){
	  $rooms[$id]++;
	  if (bForm_vm_Lease::shared_room_is_full($r) || (($r_room['a_capacity'] == 1) && !empty($r['lease_id']))){
	    @$fully_booked[$id]++;
	  }
	}
      }
    }

    /*
    // debug...
    b_debug::print_r($rooms,'available rooms');
    b_debug::print_r($fully_booked,'fully booked rooms');
    */

    // Prepare the reply
    if ($hasAccommodation) $reply[LODGING_RM] = LODGING_RM_TXT;
    if (!$org_context) foreach($accommodationOptions->get_BI() as $k=>$v) $reply[$k] = $v;
    
    foreach($rooms as $id=>$count){
      list($hut_id,$a_id,$a_quantity) = $n[$id];
      if ($a_quantity <= 0)                       $drop = 'not yet requested';
      elseif (@$fully_booked[$id] >= $a_quantity) $drop = 'fully booked';
      else                                        $drop = False;
      if ($drop && ($rooms[$id] > 0)) { 
	b_debug::xxx(b_fmt::redText("IGNORE $id - $drop, the room was booked ".$rooms[$id]." times (???)"));
	// Update the options
	$a_quantity = $rooms[$id];
	if (!$org_context) $accommodationOptions->set_a_value($a_quantity,$a_id,$hut_id,'a_quantity');
	$drop = False;
      }
      if ($drop){
	b_debug::xxx("Cancel $id - $drop");
      }else{
	$reply[$a_id] = $id;
	$n_free = $a_quantity - (int)@$fully_booked[$id];
	b_debug::xxx(b_fmt::redText("ACCEPT $n_free/$a_quantity $id"));
      }
    }
    b_debug::xxx(join(',&nbsp;',array_values($reply)));
    return $reply;
  }
      
  public function get_occupiedRooms($hut_id=Null,$lease_start=Null,$lease_end=Null,$v_host_avid=Null){
    
    $where = array("hut_code != '".HUT_BI."'");
    if ($this->parent_ID != myOrg_ID)   $where[] = 'e_id = '.$this->parent_ID;
    if (!empty($hut_id)) $where[] = "hut_id = $hut_id";
    if (!empty($lease_start) && !empty($lease_end)){
      $where[] = "lease_end >= $lease_end ";
      $where[] = "lease_start <= $lease_start";
    }
    if (!empty($v_host_avid)) $where[] = "v_host_avid = $v_host_avid";
    
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
    
