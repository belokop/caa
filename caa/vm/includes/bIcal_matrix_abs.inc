<?php
locateAndInclude('b_checkBox');
locateAndInclude('bIcal_matrix');
class bIcal_matrix_abs extends bIcal_matrix{
  
  var $see_day9= True;
  var $seeAgenda      = True;
  var $imposedEntries = array();
  var $a_id           = array();

  public static function _show($start,$end,$imposedEntries=array(),$a_ids=array()){
    $m = new bIcal_matrix_abs();
    $m->showLegend = False;
    $m->day1 = $start;
    $m->day9 = $end;
    $m->imposedEntries = $imposedEntries;
    $m->a_id = $a_ids;
    $m->show();
  }
  
  function __construct(){
    parent::__construct();
    $_GET['sBy'] = 'CONCAT(hut_code,hut_name,a_capacity)';
  }
  
  function show($matrix=Null){    

    MSG::H1("Who Lives Where");
    if (is_object(VM::$e))      print x('h3','Who lives where?');
    
    if (!b_checkBox::whatIsOn())    b_checkBox::set('seeAccodated','on');

    $count = array('Clochards'=>0, 'EA'=>array()); 
    $noacc = '(lease_aid=0 OR lease_aid IS NULL)';
    $this->mTenants = $mPrograms = $onlyThoseAps = $negates = array();
    if ($this->day1 < b_time::txt2unix('2000-01-01'))  $this->day1 = time()      - 15*24*3600;
    if ($this->day9 < b_time::txt2unix('2000-01-01'))  $this->day9 = $this->day1 + 33*24*3600;
    $this->day9 = min($this->day9,$this->day1 + $this->nDaysToShow*24*3600);
    $this->dbg(date('Y-M-D H:i', $this->day1).' --> '.date('Y-M-D H:i', $this->day9),cnf_dev);
	     
	     $haveTenants = 0;
    $where = array("v_end   >=$this->day1",
		   "v_start <=$this->day9");
    if (is_object(VM::$e)) $where[] = "v_eid=".VM::$e->ID; // && (VM::$e->isOrganizer() 
	
    /*
     * Show agenda entries if needed
     */    
    if ($this->seeAgenda){  
      $this->dbg('Checking agenda entries',cnf_dev);
      $q = myPear_db()->qquery("SELECT e_start,e_end,e_name FROM abs_events WHERE ".
				 join(' AND ',array("e_code > 0", 
						    "e_end  >=$this->day1",
						    "e_start<=$this->day9")).
				 " ORDER BY e_start ASC");
      
      while ($r = myPear_db()->next_record($q))
	if (b_time::inInterval($r['e_start'],$r['e_end'],$this->day1,$this->day9))
	  $mPrograms[0][] = array('day1' => b_time::txt2unix(date('Y-m-d',$r['e_start']),'08:00'),
				  'day9' => b_time::txt2unix(date('Y-m-d',$r['e_end'])  ,'20:00'),
				  'text' => $r['e_name'],
				  'tips' => x('strong',nbsp(myOrg_name.' program')).'<br/>'.nbsp($r['e_name']),
				  'clas' => $this->getRandomColor($r['e_name']),
				  'name' => '  '.strToUpper(myOrg_name).' EVENTS',
				  'styl' => 'strong',
				  'url'  => 'em');
    }
    
    /*
     * Check the 'want to see only those' apartments list
     */
    if (!empty($this->a_id)){
      $this->noNavig = True;
      $this->hideCheckBox = True; 
      foreach ($this->a_id as $a)  if ((int)$a) $onlyThoseAps[] = "a_id=$a";
      $this->dbg("Drawing selection ".join(', ',$onlyThoseAps),cnf_dev);
      $q = VM_query(array_merge($onlyThoseAps,$where));
    }else{
      if ($this->hideCheckBox){
	b_checkBox::set('seeClochards','off');
	b_checkBox::set(LODGING_OA,'off');
      }
      
      /*
       * Les clochards
       */
      $this->dbg('Counting les clochards',cnf_dev);
      $negates[] = "NOT $noacc";
      $clochards_query = VM_query(array_merge($where,array($noacc)));
      if ($n = myPear_db()->num_rows($clochards_query)){
	$this->dbg($n,cnf_dev);
	$count["Clochards"] = $n;
	$haveTenants += $n;
      }else{
	b_checkBox::set('seeClochards','off');
      }
      
      /*
       * External accommodation
       */
      locateAndInclude('bHolder_vm_Rooms');
      $query_EA = array();
      $this->dbg('Counting '.LODGING_OA_TXT,cnf_dev);
      $negates[] = bHolder_vm_Rooms::reject_self_accommodated();
      $oa = str_ireplace(' NOT ',' ',bHolder_vm_Rooms::reject_self_accommodated());
      $query_EA[LODGING_OA] = VM_query(array_merge($where,array($oa)));
      if ($n = myPear_db()->num_rows($query_EA[LODGING_OA])){
	$this->dbg($n,cnf_dev);
	$count["EA"][LODGING_OA] = $n; 
	$haveTenants += $n;
      }else{
	b_checkBox::set(LODGING_OA,'off'); 
      }
      
      /*
       * Normal accommodation
       */
      $this->dbg('Accommodated tenants',cnf_dev);
      $where_accommodated = array("lease_start <= ".$this->day9,
				  "lease_end   >= ".$this->day1);
      if (is_object(VM::$e)) $where_accommodated[] = "v_eid=".VM::$e->ID; // && (VM::$e->isOrganizer() 
      $queary_accommodated = VM_query(array_merge($where_accommodated,$negates));
      $haveTenants += ($nAccomodated = myPear_db()->num_rows($queary_accommodated));
      
      /*
       * fill the entries according the selection boxes
       */
      if(1 || b_checkBox::isOn('seeAccodated')){
	while($r = myPear_db()->next_record($queary_accommodated)) $this->fillEntry($r);
      }
      
      foreach($query_EA as $type=>$q){
	if(b_checkBox::isOn($type)){
	  while($r = myPear_db()->next_record($q))               $this->fillEntry($r);
	}
      } 

      if (b_checkBox::isOn('seeClochards')){
	while($r = myPear_db()->next_record($clochards_query))   $this->fillEntry($r);
      }
    }
    
    
    /*
     * fill the optional "external" entries (passed as the argument) 
     */
    foreach ($this->imposedEntries as $k=>$entry){
      $this->fillEntry($entry);
    }
    
    if ($haveTenants && !$this->hideCheckBox)
      if (empty($onlyThoseAps) && !is_object($this->tClass)){
	$boxes = array();
	if ($nAccomodated > 0)             $boxes['seeAccodated'] = "See $nAccomodated accommodated tenants";
	if ($count["Clochards"] > 0)       $boxes['seeClochards'] = "See ".$count["Clochards"]." homeless tenants";
	foreach($count["EA"] as $type=>$n) $boxes[LODGING_OA] = "See $n tenants ".LODGING_OA_TXT;
	//$boxes['seeEmpty'] = "See TBD empty apartments";
	if ($boxes) $this->footer[] = b_checkBox::create($boxes,'<br/>');
      }
    $this->showHeader();
    $this->showBody($mPrograms,True);
    $this->showBody($this->mTenants);
    $this->showFooter();
  }
  
  /*
   *
   */
  function fillEntry($r){
    $r['name'] = bForm_Avatar::_fmtName('Lf',$r);
    $app = (empty($r['a_name'])
	    ? 'Homeless'
	    : bForm_vm_Room::_a_name($r,True,False,True,False));

    if ( (int)$r['lease_start'] <9999) $r['lease_start'] = $r['v_start'];
    if ( (int)$r['lease_end']   <9999) $r['lease_end']   = $r['v_end'];
    $cotenans = VM_cotenants()->get_cotenants($r['lease_id'],False,'names');
    $entry=array('day1' => $r['lease_start'],
		 'day9' => $r['lease_end'],
		 'text' => x("a href='".b_url::same("?function=vm_edit_accommodation&resetcache_once=1&lease_id=$r[lease_id]&v_id=$r[v_id]&action_once=".btn_edit_accommodation)."'",$r['name']),
		 'clas' => $this->getRandomColor(vm_colorLegend($r)),
		 'name' => (True
			    ? $app
			    : bJS()->popup($app,b_url::same("?mode=RO&form=vm_Room&id=$r[a_id]"))),
		 'tips' => nbsp(preg_replace('/<br .><br .>/','<br/>',
					     join('<br/>',array(x('strong',$r['name']).
								($cotenans ? ', cotenant '.join(', ',$cotenans) : '').
								(($x=$r['v_accompaning'])?" (me + $x)":""),
								@$r['e_code']<0 ? @$r['e_name'] : '',
								x('em',@$r['av_institute']),
								x('em',b_time::strftime('%e-%h',$r['v_start']).' <-> '.b_time::strftime('%e-%h',$r['v_end'])))))));
    $this->mTenants[$r['a_id']][] = $entry;
    // if ($r['v_accompaning'])     $this->mTenants[$r['a_id']][] = $entry;
  }
}
