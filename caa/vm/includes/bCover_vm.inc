<?php
locateAndInclude('bCover');
locateAndInclude('bForm_vm_Lease');
locateAndInclude('bHolder_vm_Rooms');
locateAndInclude('bHolder_vm_Offices');
class bCover_vm extends bCover {

  var $id    = 'v_code';
  var $start = 'v_start';
  var $end   = 'v_end';
  var $showMatrix = True;
  var $maxClochards = 99;
  var $day1 = NULL;
  var $day9 = NULL;
  var $summary = array();  
  var $summary2= array();  
  var $e = Null;
  public $leases = array();
  public $leases_current = array();

  /*
   *
   */
  public static function wipeAllocation($whatToWipe,$event){
    static $ids = array('Offices'   => 'v_oid',
			'Apartment' => '??',
			'Cotenants' => '??');
    // Remove all office allocations for the event 
    myPear_db()->qquery("UPDATE abs_visits SET ".$ids[$whatToWipe]." = NULL WHERE v_eid = ".$event->ID,cnf_dev);
    myPear::MESSAGE("Wipe office allocations for ".x('"',x('em',$event->name(True,40))));
    
    // Force automatic office assignment for the staff members
    if ($whatToWipe == 'Offices') self::allocateOfficesForStaff($event->ID);
  }

  /*
   * Accommodate Offices and/or Apartments for 1 year period,
   * and the co-tenants for the period of stay
   */
  public static function autoAllocate($whatToAllocate,$event,$leases=array(),$leases_current=array()){

    // Allocate resources for the period of event
    $b = new bCover_vm($whatToAllocate);
    $b->e = $event;
    $b->day1  = $b->e->getValue('e_start');
    $b->day9  = $b->e->getValue('e_end');

    switch($whatToAllocate){
    case 'Apartment':
    case 'Cotenants':
      $b->leases = $leases;
      $b->leases_current = $leases_current;
      foreach(array('leases','leases_current') as $a){
	foreach($$a as $lease_id){
	  $info  = bForm_vm_Lease::_leaseInfo($lease_id);
	  $b->day1 = min($b->day1,$info['leasday1']);
	  $b->day9 = max($b->day9,$info['leasday9']);
	}
      }
      break;

    case 'Offices':
      self::allocateOfficesForStaff($b->e->ID);
      break;
      
    default:
      b_debug::internalError('??? unexpected arguments');
    }
    $b->allocate();
  }

  /*
   * Force automatic office assignment for the staff members
   */
  private static function allocateOfficesForStaff($e_id){
    $ea = new bUnit_ea_members();
    foreach($ea->getMembers_byStatus() as $av_id=>$rec){
      // b_debug::print_r($rec);
      myPear::getInstance_new('bForm_vm_Visit',array('v_avid'=>$av_id,'v_eid'=>$e_id),'strict');
    }
  }
      
  /*
   *
   */
  public function __construct($whatToAllocate='Apartment'){
    static $input = array('Apartment'=> array('v_roomID' =>'lease_aid',
					      'room_ID'  =>'a_id',
					      'room_name'=>'a_name',
					      'room_d1'  =>'a_start',
					      'room_d9'  =>'a_end',
					      'v_id'     => 'lease_id',
					      'v_start'  => 'leasday1',
					      'v_end'    => 'leasday9',
					      ),
			  'Offices'   => array('v_roomID' =>'v_oid',
					      'room_ID'  =>'o_id',
					      'room_name'=>'o_name',
					      'room_d1'  =>'o_start',
					      'room_d9'  =>'o_end',
					      'v_id'     => 'v_id',
					      'v_start'  => 'v_start',
					      'v_end'    => 'v_end',
					      ),
			  'Cotenants'=>array('v_roomID'  =>'lease_aid',
					     'room_ID'   =>'a_id',
					     'room_name' =>'a_name',
					     'room_d1'   =>'empty',
					     'room_d9'   =>'empty',
					     'v_id'      => 'lease_id',
					     'v_start'   => 'leasday1',
					     'v_end'     => 'leasday9',
					     ));
    parent::__construct();
    $this->whatToAllocate  = $whatToAllocate;
    $this->whatToAllocateLC= strToLower($whatToAllocate);
    foreach ($input[$this->whatToAllocate] as $k=>$v) define($k,$v);
  }

  /*
   *
   */
  protected function quality(){
    switch($this->whatToAllocate){
    case 'Offices':
      return parent::quality();
      $quality = 0;
      static $kjkjkj=0;$kjkjkj++;
      $test = array();
      foreach($this->solution as $o_id=>$visits){
	$quality += count($visits);
	$x=array_keys($visits);	if ($kjkjkj == 3) b_debug::print_r($x,$o_id);
      }
      $this->dbg($quality);
      return $quality;
      return array(-count($this->solution),1);
    case 'Apartment':
    case 'Cotenants':
      $this->dbg();
      return array(-count($this->solution),1);
    }
  }

  /*
   *
   */
  protected function clashing($ar1, $ar2) {
    switch($this->whatToAllocate){
    case 'Cotenants':
    case 'Apartment':
      return parent::clashing($ar1, $ar2);
    case 'Offices':
      return b_time::inInterval(b_time::oa_start($ar1[$this->start]),b_time::oa_end($ar1[$this->end]),
				b_time::oa_start($ar2[$this->start]),b_time::oa_end($ar2[$this->end]));
    }
  }

  /*
   *
   */
  public function query($where){
    switch($this->whatToAllocate){
    case 'Apartment':
      $_GET['sBy'] = 'a_name';
      return VM_query($where);

    case 'Offices':
      $_GET['sBy'] = 'o_name';
      return VM_query($where,"LEFT JOIN abs_offices ON o_id = v_oid");

    case 'Cotenants':
      return bForm_vm_Lease::queryLease(array_merge($this->leases,$this->leases_current),$this->debug);
    default:
      b_debug::internalError('? unexpected invocation');
    }
  }
  
  /*
   *
   */
  function allocate(){
    define('TCODE',7); // color code for the current tenants
    
    // locate the allocation requests (aka the clochards)
    $_GET['sBy'] = $this->start;

    unset($_SESSION['bCover_vm_dejaVu']);
    if (empty($_SESSION['bCover_vm_dejaVu'])) $_SESSION['bCover_vm_dejaVu'] = array();

    $where = array('('.v_roomID.' IS NULL OR '.v_roomID.' = 0)',
		   "v_status = '".STATUS_YES."'",
		   "v_start <= ".$this->day9,
		   "v_end   >= ".$this->day1,
		   "v_eid = ".$this->e->ID);
    $q = $this->query($where);
    $clochards = array();
    while ($r = myPear_db()->next_record($q)) {
      switch($this->whatToAllocate){
      case 'Apartment': 
	if ($r['lease_aid'] == VM::$OA->ID){
	  $this->summary[LODGING_OA_TXT][] = $r['a_name'];
	  break;
	}
      case 'Offices':
	if (!in_array($r['v_id'],$_SESSION['bCover_vm_dejaVu']) && (@$n++ < $this->maxClochards)){
	  // Skip visitors with flag "no office provided"
	  if (bForm_vm_Visit::_getPolicy(VM_V_provideOffice,$r)){
	    $clochards[] = $r; 
	    $requests[] = $r['v_id'];
	    $this->summary['allocation request(s):'][] = $r['v_id'];
	  }else{
	    $this->dbg(bForm_Avatar::_fmtName('Lf',$r). ' is not offered an office');
	  }
	}
	break;
      case 'Cotenants':
	$this->summary['cotenants:'][] = $r['v_id'];
	break;
      }
    }

    // Build the constraints
    $this->buildConstraints();
    
    if (empty($clochards)){ 
      print x('i','No new allocation requests for '.b_time::period($this->timeMn,$this->timeMx));
      $this->solution = $this->constraints;
    }else{

      // Find the solution for the allocation
      $this->_init($clochards);
      $this->solve();
      //      var_dump(array_keys($this->solution),'solution','nosort');
      
      /*
       * Link solution to the Office/Apartment objects.
       * Note, the offices allocation is done on "desk level", i.e.
       * the constrains are just multiplied by the number of desks in the office. 
       */
      $sols = array();
      foreach ($this->solution as $sol_id=>$visits){
	switch($this->whatToAllocate){
	case 'Offices':
	  $sol = myPear::getInstance_new('bForm_vm_Office',(int)($sol_id/100),'strict');
	  if ($sol && ($sol->getValue('o_capacity') < ($sol_id%100))) $sol = 0;
	  break;
	  
	case 'Apartment':
	  $sol = myPear::getInstance_new('bForm_vm_Room',$sol_id,'fatal');
	  break;
	  
	case 'Cotenants':
	  $sol = 0;
	  break;
	}
	$this->dbg("solution $sol_id --> ".$sol);

	foreach ($visits as $n=>$visit){
	  if ($v=$this->getVisit(@$visit['v_id'])){
	    if (is_object($sol)){ 
	      if (in_array($v->ID,$requests)){
		$v->setResource($this->whatToAllocate,$sol->ID,$visit);
		$this->summary2[x('strong',b_fmt::redText($this->whatToAllocateLC."s successfully allocated: ",'green'))][$sol->ID][] = $v->ID;
	      }else{
		// was already allocated
		//		$this->dbg(sprintf("Visitor %s got allocation, but it was not requested",$v->av->fmtName('fl')));
	      }
	    }else{
	      $_SESSION['bCover_vm_dejaVu'][] = $v->ID; // small optimization
	      $this->summary[x('strong',b_fmt::redText('manual allocations needed:'))][] = $v->ID;
	    }
	  }
	}
      }
    }
    $this->printSummary();
    
    /*
     * draw solution matrix
     */
    if ($this->showMatrix) {
      if (!empty($clochards))myPear::H2('Allocation for '.b_time::period($this->timeMn,$this->timeMx));
      locateAndInclude('bIcal_matrix_offices');
      $m = new bIcal_matrix_offices();
      $m->noNavig_arrows = $m->noNavig_months = True;
      $m->show();
    }
  }
  
  /*
   *
   */
  private function getVisit($v_id){
    if (!empty($v_id)){
      switch($this->whatToAllocate){
      case 'Offices':    return myPear::getInstance_new('bForm_vm_Visit',$v_id,array('nocache','fatal'));
      case 'Apartment': return bForm_vm_Lease::visit($v_id);
      case 'Cotenants': b_debug::internalError("should not come here");
      }
    }
  }
  
  /*
   *
   */
  function printSummary(){
    $t = new b_table();
    foreach ($this->summary as $k=>$v){
      if (is_array($v)){
	$n = count($v);
	foreach ($v as $x_id) {
	  if ((int)$x_id){
	    $o = myPear::getInstance_new(Null,$x_id,array('nocache','fatal'));
	    $period = method_exists($o,'period') ? $o->period() : ' ';
	    $o_name = '';
	    if ($xx_id = $o->getValue('v_oid',True,True)){
	      $off = myPear::getInstance_new(Null,$xx_id,'fatal');
	      $o_name = $off->name();
	    }
	    $t->tro(); 
	    $t->td($n,'class="align_right"'); $n = '';
	    $t->td(array($k,
			 $o->name(),
			 x('i',$o_name),
			 x('i',$period),
			 ));
	    //$t->td($o->comment($this->whatToAllocate));
	    $t->trc(); 
	    $k = '';
	  }else{
	    var_dump("??? strange ID = $x_id");
	  }
	}
      }else{ 
	$t->tr(array($k,$v),'class="align_right"');
      }
    }
    $t->close();

    $o_name_was = '?';
    $t = new b_table();
    ksort($this->summary2);
    foreach($this->summary2 as $header=>$aps){
      ksort($aps);
      $h = count(array_keys($aps))." $header";
      foreach($aps as $x_id=>$visits){
	$o = myPear::getInstance_new(Null,$x_id,'fatal');
	$o_name = $o->name(); 
	foreach($visits as $v_id){
	  $v = myPear::getInstance_new(Null,$v_id,array('nocache','fatal'));
	  $t->tr(array($h,
		       $o_name,
		       $v->name(),
		       $v->period(),
		       ));
	  $h = $o_name = '';
	}
      }
    }
    $t->close();
  }
  
  /*
   *
   */
  function buildConstraints(){
    /* 
     * note, the 'externalAccs' are NOT considered here
     */
    switch($this->whatToAllocate){
    case 'Offices':   $query = bHolder_vm_Offices::queryOffices("o_status = 'visitors'"); break;
    case 'Apartment': $query = bHolder_vm_Rooms::_queryAps(); break;
    case 'Cotenants': $query = bForm_vm_Lease::queryLease($this->leases,$this->debug); break;
    }
    while ($r=myPear_db()->next_record($query)) {
      //      if (isset($r['o_status']) && ($r['o_status'] != OFFICE_AUTO_ALLOCATABLE)) continue;
      //      if (isset($r['a_status']) && ($r['a_status'] !=     AP_AUTO_ALLOCATABLE)) continue;
      if (empty($r[room_d1])) $r[room_d1] = myPear_day0;
      if (empty($r[room_d9])) $r[room_d9] = myPear_day9;
      if (b_time::inInterval($this->day1, $this->day9,
			     $r[room_d1],$r[room_d9])){

	// Check that the office/room is available during the visit period,
	// build a constrain if not.
	$constrain = array();
	if (    $r[room_d1]>$this->day1 && 
	        $r[room_d9]<$this->day9) $constrain[] = array('v_start' => $r[room_d1],
							      'v_end'   => $r[room_d9],
							      'name'    => 'not available');
	elseif ($r[room_d1]>$this->day1) $constrain[] = array('v_start' => $this->day1,
							      'v_end'   => $r[room_d1],
							      'name'    => 'not available');
	elseif ($r[room_d9]<$this->day9) $constrain[] = array('v_start' => $r[room_d9],
							      'v_end'   => $this->day9,
							      'name'    => 'not available');
	elseif (False){ // Not sure...
	  // Check that the office/room is NOT occupied during the visit period
	  // by someone who is NOT participating in the event,
	  // build a constrain in case of clashes.
	  $where = array("v_eid != ".$this->e->ID,
			 "v_oid  = ".$r[room_ID],
			 "v_start<=".$this->e->getValue('e_end'),
			 "v_end  >=".$this->e->getValue('e_start'));
	  $q = myPear_db()->qquery("SELECT av_id,av_firstname,av_lastname,v_start,v_end FROM abs_visits ".
				   " LEFT JOIN zzz_avatars ON av_id = v_avid WHERE ".
				   join(' AND ',$where),cnf_dev); 
	  while ($rr=myPear_db()->next_record($q)) {
	    $constrain[] = array('v_start' => $rr['v_start'],
				 'v_end'   => $rr['v_end'],
				 'name'    => bForm_Avatar::_fmtName('fl',$rr));
	  }	  
	  b_debug::print_r($constrain,'constrain from visits');
	}
	
	if (empty($constrain)){ 
	  $this->constraints[$r[room_ID]] = array();
	  // $this->summary[$this->whatToAllocateLC.'s available:'][] = $r[room_ID];
	}else{
	  foreach($constrain as $c) $this->constraints[$r[room_ID]][] = $c;
	  $this->summary[$this->whatToAllocateLC.'s partly available:'][] = $r[room_ID];
	}
      }
    }

    // put all the existing tenants as Constraints
    $where = array();
    switch($this->whatToAllocate){
    case 'Apartment': 
      $where[] = bHolder_vm_Rooms::rejectExtApp(); 
    case 'Offices':  
      $where[] =  v_roomID." > 0";
      $where[] = "v_eid = ".$this->e->ID;
      $q = $this->query($where);
      break;
    case 'Cotenants': 
      $q = (empty($this->leases_current)
	    ? Null
	    : bForm_vm_Lease::queryLease($this->leases_current,$this->debug));
      $q = Null;
      break;
    }
    if (!empty($q)) while ($r=myPear_db()->next_record($q)) {
	if (is_array(@$this->constraints[@$r[room_ID]])){
	  if (b_time::inInterval($this->day1, $this->day9, $r[v_start], $r[v_end])){
	    $constrain = array('v_id'    => $r[v_id],
			       'v_start' => $r[v_start],
			       'v_end'   => $r[v_end],
			       );
	    $this->constraints[$r[room_ID]][] = $constrain;
	    $this->setMinMaxTime($constrain);
	    $this->summary2[$this->whatToAllocateLC.'s were already allocated:'][$r[room_ID]][] = $r[v_id];
	  }
	}
      }
    
    /*
     * Restructure the constrains, create "Desks" from the "Offices".
     * Number of desks is the office capacity.
     */
    // b_debug::print_r($this->constraints,'this->constraints','nosort');
    if ($this->whatToAllocate == 'Offices'){
      // Limit office allocation by the event period
      // $this->timeMn = $this->e->getValue('e_start');
      // $this->timeMx = $this->e->getValue('e_end');

      foreach ($this->constraints as $o_id=>$constraint){
	$o = myPear::getInstance_new('bForm_vm_Office',$o_id,'fatal');
	for($n=1; $n<=$o->getValue('o_capacity'); $n++) $this->constraints[$o_id*100 + $n] = $constraint;
	unset($this->constraints[$o_id]);
	b_debug::_('bCover_vm',$o->name(),$o->getValue('o_capacity').' constraints');
      }
    }
  }
  
  /*
   * get a unique index for the non-allocated sample, 
   * there should be no class corresponding to this index
   */
  function pivotIndex(){
    return array(-9999, 'NOT ALLOCATED');
    $index = bMaster()->currentValue + 1;
    if ($this->whatToAllocate == 'Offices') $index = $index * 100;
    return array($index, 'NOT ALLOCATED');
  }
}
