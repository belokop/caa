<?php
locateAndInclude('bCover');
locateAndInclude('bForm_vm_Lease');
locateAndInclude('bHolder_vm_Apartments');
locateAndInclude('bHolder_vm_Offices');
class bCover_vm extends bCover {

  var $id    = 'v_code';
  var $start = 'v_start';
  var $end   = 'v_end';
  var $showMatrix = False;
  var $maxClochards = 50;
  var $day1 = NULL;
  var $day9 = NULL;
  var $summary = array();  
  public $leases = array();
  public $leases_current = array();

  /*
   * Accommodate Offices and/or Apartments for 1 year period,
   * and the co-tenants for the period of stay
   */
  public static function autoAllocate($whatToAllocate,$leases=array(),$leases_current=array()){
    $b = new bCover_vm($whatToAllocate);
    switch($whatToAllocate){
    case 'Apartment':
    case 'Office':
      $b->showMatrix = False;
      $b->day1 = b_time::noon() -   7*86400;
      $b->day9 = b_time::noon() + 370*86400; 
      break;

    case 'Cotenants':
      $b->showMatrix = False;
      $b->leases = $leases;
      $b->leases_current = $leases_current;
      $b->day1 = +CONST_eternity_t);
      $b->day9 = -CONST_eternity_t);
      foreach(array('leases','leases_current') as $a){
	foreach($$a as $lease_id){
	  $info  = bForm_vm_Lease::_leaseInfo($lease_id);
	  $b->day1 = min($b->day1,$info['lease_start']);
	  $b->day9 = max($b->day9,$info['lease_end']);
	}
      }
    }
    $b->allocate();
  }
      
  public function __construct($whatToAllocate='Apartment'){
    static $input = array('Apartment'=> array('input_ir'=>'lease_aid',
					      'input_ID'=>'a_id',
					      'input_nm'=>'a_name',
					      'input_d1'=>'a_start',
					      'input_d9'=>'a_end',
					      'v_id'    => 'lease_id',
					      'v_start' => 'lease_start',
					      'v_end'   => 'lease_end',
					      ),
			  'Office'   => array('input_ir'=>'v_oid',
					      'input_ID'=>'o_id',
					      'input_nm'=>'o_name',
					      'input_d1'=>'o_start',
					      'input_d9'=>'o_end',
					      'v_id'    => 'v_id',
					      'v_start' => 'v_start',
					      'v_end'   => 'v_end',
					      ),
			  'Cotenants'=>array('input_ir'=>'lease_aid',
					     'input_ID'=>'a_id',
					     'input_nm'=>'a_name',
					     'input_d1'=>'empty',
					     'input_d9'=>'empty',
					     'v_id'    => 'lease_id',
					     'v_start' => 'lease_start',
					     'v_end'   => 'lease_end',
					     ));
    
    $this->whatToAllocate  = $whatToAllocate;
    $this->whatToAllocateLC= strToLower($whatToAllocate);
    foreach ($input[$this->whatToAllocate] as $k=>$v) define($k,$v);
  }

  protected function quality(){
    switch($this->whatToAllocate){
    case 'Apartment':
    case 'Office':
      return parent::quality($solution);
    case 'Cotenants':
      $this->dbg();
      return array(-count($solution),1);
    }
  }

  public function query($where){
    switch($this->whatToAllocate){
    case 'Apartment':
      $_GET['sBy'] = 'a_name';
      return VM_query($where);

    case 'Office':
      $_GET['sBy'] = 'o_name';
      return VM_query($where,"LEFT JOIN abs_offices ON o_id = v_oid");

    case 'Cotenants':
      b_debug::print_r($where);
      return bForm_vm_Lease::queryLease(array_merge($this->leases,$this->leases_current),$this->debug);
    }
  }
  
  function allocate(){
    define('TCODE',7); // color code for the current tenants
    
    // locate the allocation requests (aka the clochards)
    $_GET['sBy'] = $this->start;
    $q = $this->query(array('('.input_ir.' IS NULL OR '.input_ir.' = 0)',
			    "v_start <= $this->day9",
			    "v_end   >= $this->day1"));
    $clochards = array();
    if (empty($_SESSION['bCover_vm_dejaVu'])) $_SESSION['bCover_vm_dejaVu'] = array();
    while ($r = myPear_db()->next_record($q)) {
      switch($this->whatToAllocate){
      case 'Apartment': 
	if ($r['lease_aid'] == VM::$OA->ID){
	  $this->summary[LODGING_OA_TXT][] = $r['a_name'];
	  break;
	}
      case 'Office':
	if (!in_array($r['v_id'],$_SESSION['bCover_vm_dejaVu']) && (@$n++ < $this->maxClochards)){
	  $clochards[] = $r; 
	  $requests[] = $r['v_id'];
	  $this->summary['allocation request(s):'][] = $r['v_id'];
	}
	break;
      case 'Cotenants':
	$this->summary['cotenants:'][] = $r['v_id'];
	break;
      }
    }
    
    $this->buildConstraints();
    myPear::H1(bText::_('Allocating '.$this->whatToAllocateLC.'s').' '.b_time::period($this->timeMn,$this->timeMx),'noTranslate');
    
    if (empty($clochards)){ 
      print x('i','No new allocation requests for this time period');
      $this->solution = $this->constraints;
    }else{

      /*
       * Find the solution for the allocation
       */
      $this->_init($clochards);
      $this->solve();
      
      /*
       * Link solution to the Office/Apartment objects.
       * Note, the offices allocation is done on "desk level", i.e.
       * the constrains are just multiplied by the number of desks in the office. 
       */
      foreach ($this->solution as $sol_id=>$visits){
	switch($this->whatToAllocate){
	case 'Office':
	  $sol = myPear::getInstance('bForm_vm_Office',(int)($sol_id/100),'strict');
	  if ($sol && ($sol->getValue('o_capacity') < ($sol_id%100))) $sol = 0;
	  break;
	  
	case 'Apartment':
	  $sol = myPear::getInstance('bForm_vm_Apartment',$sol_id,'strict');
	  break;

	case 'Cotenants':
	  break;
	}
	foreach ($visits as $n=>$visit){
	  if ($v=$this->getVisit(@$visit['v_id'])){
	    if ($sol){ 
	      $v->setResource($this->whatToAllocate,$sol->ID,$visit);
	      if (in_array($v->ID,$requests))$this->summary[x('strong',b_fmt::redText('successful allocations: ','green'))][] = $v->ID;
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
      if (!empty($clochards))	myPear::H2('Allocation for '.date('Y-m-d',$this->timeMn).' <-> '.date('Y-m-d',$this->timeMx));
      $m = new bIcal_matrix(time());
      $m->see_day9 = True;
      $m->day1 = $this->timeMn - 2*86400;
      $m->day9 = $this->timeMx + 3*86400;
      $m->d1   = $this->start;
      $m->d9   = $this->end;
      $m->noNavig = true;
      $m->show($this->solution);
    }
  }
  
  private function getVisit($id){
    $this->dbg();
    if (!empty($id)){
      switch($this->whatToAllocate){
      case 'Office':    return myPear::getInstance('bForm_vm_Visit',$id);
      case 'Apartment': return bForm_vm_Lease::visit($id);
      case 'Cotenants': b_debug::internalError("should not come here");
      }
    }
  }
  
  function printSummary(){
    $t = new b_table();
    foreach ($this->summary as $k=>$v)
      if (is_array($v)){
	$n = count($v);
	foreach ($v as $o_id) {
	  if ((int)$o_id){
	    $o = myPear::getInstance(myPear::getInstanceFromID($o_id),$o_id);
	    if ($o instanceof bForm_vm_Visit){
	      $t->tro(); 
	      $t->td($n,'class="align_right"'); $n = '';
	      $t->td(array($k,
			   $o->name(),
			   x('i',$o->period())));
	      //$t->td($o->comment($this->whatToAllocate));
	      $t->trc(); 
	      $k = '';
	    }elseif ($o instanceof bForm_vm_Apartment){
	      $t->tr(x('i',$o->period()),$o->name());
	    }
	  }
	}
      }else{ 
	$t->tr(array($k,$v),'class="align_right"');
      }
    $t->close();
  }
  
  function buildConstraints(){
    /* 
     * note, the 'externalAccs' are NOT considered here
     */
    switch($this->whatToAllocate){
    case 'Office':    $query = bHolder_vm_Offices::queryOffices(); break;
    case 'Apartment': $query = bHolder_vm_Apartments::_queryAps(); break;
    case 'Cotenants': $query = bForm_vm_Lease::queryLease($this->leases,$this->debug); break;
    }
    while ($r=myPear_db()->next_record($query)) {
      if (isset($r['o_status']) && ($r['o_status'] != OFFICE_AUTO_ALLOCATABLE)) continue;
      if (isset($r['a_status']) && ($r['a_status'] !=     AP_AUTO_ALLOCATABLE)) continue;
      if (empty($r[input_d1])) $r[input_d1] = rent0;
      if (empty($r[input_d9])) $r[input_d9] = rent9;
      if (b_time::inInterval($this->day1, $this->day9,
			     $r[input_d1],$r[input_d9])){
	/*
	 * Constrain based on the place availability
	 */
	if (    $r[input_d1]>$this->day1 && 
	        $r[input_d9]<$this->day9) $constrain = array('v_start' => $r[input_d1],
							     'v_end'   => $r[input_d9],
							     'name'    => 'not available');
	elseif ($r[input_d1]>$this->day1) $constrain = array('v_start' => $this->day1,
							     'v_end'   => $r[input_d1],
							     'name'    => 'not available');
	elseif ($r[input_d9]<$this->day9) $constrain = array('v_start' => $r[input_d9],
							     'v_end'   => $this->day9,
							     'name'    => 'not available');
	else                                  $constrain = array();
	
	$this->constraints[$r[input_ID]] = array();
	if (empty($constrain)){ 
	  if (empty($this->summary[$id = $this->whatToAllocateLC.'s for this time period'])) $this->summary[$id] = 0;
	  $this->summary[$id]++;
	}else{
	  $this->constraints[$r[input_ID]][] = $constrain;
	  $this->summary[$this->whatToAllocateLC.'s partly available:'][] = $r[input_ID];
	}
      }
    }

    // put all the existing tenants as Constraints
    switch($this->whatToAllocate){
    case 'Apartment': 
      $where[] = bHolder_vm_Apartments::rejectExtApp(); 
    case 'Office':  
      $where[] = input_ir." > 0";
      $q = $this->query($where);
      break;

    case 'Cotenants': 
      if (empty($this->leases_current)) $q = Null;
      else $q = bForm_vm_Lease::queryLease($this->leases_current,$this->debug);
      $q = Null;
      break;
    }
    if (!empty($q)) while ($r=myPear_db()->next_record($q)) {
	if (is_array(@$this->constraints[@$r[input_ID]])){
	  if (b_time::inInterval($this->day1, $this->day9, $r[v_start], $r[v_end])){
	    $constrain = array('v_id'    => $r[v_id],
			       'v_start' => $r[v_start],
			       'v_end'   => $r[v_end],
			       );
	    $this->constraints[$r[input_ID]][] = $constrain;
	    $this->setMinMaxTime($constrain);
	    if (empty($this->summary['visits already OK'])) $this->summary['visits already OK'] = 0;
	    $this->summary['visits already OK']++;
	    //$this->summary['visits already OK'][] = $r['v_id'];
	  }
	}
      }
    
    /*
     * Restructure the constrains, create "Desks" from the "Offices".
     * Number of desks is the office capacity.
     */
    if ($this->whatToAllocate == 'Office'){
      foreach ($this->constraints as $o_id=>$constraint){
	$o = myPear::getInstance('bForm_vm_Office',$o_id);
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
    return array(-9999,'not allocated');
    $index = bMaster()->currentValue + 1;
    if ($this->whatToAllocate == 'Office') $index = $index * 100;
    return array($index, 'NOT ALLOCATED');
  }
}
