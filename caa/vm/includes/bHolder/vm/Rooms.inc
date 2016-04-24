<?php
locateAndInclude('bHolder_vm');
class bHolder_vm_Rooms extends bHolder_vm{

  var $cleanCost = 0;
  var $rent      = 0;
  var $usage     = 0;
  var $ndays     = 0;
  var $autoOpenHeader = False;

  private $b_tabs_hutCodes = Null;
  private   $tabs_hutCodes = array();
  private $b_tabs_huts     = Null;
  private   $tabs_huts     = array();

  private $current_hutCode = '';
  private $current_hut = '';

  public function __construct($doing='list',$title='All apartments'){
    $this->dbg();
    parent::__construct();
    $this->title = bText::_($title);
    $this->doing = $doing;

    // Delete a room if requested
    if ($a_id = @$_GET['delete_once']){
      $room = new bForm_vm_Room($a_id);
      myPear::MESSAGE("Deleting unused room ".x('em',$room->name()));
      myPear::getInstance_new('bForm_vm_Room',$a_id,'delete');
      unset($room);
    }

    switch ($this->doing){
    case 'usage':
      $this->rf = b_time::noon(0, 1, 1);
      $this->rt = b_time::noon(0,12,31);
      $this->title .= ' '.b_time::period($this->rf,$this->rt);
      break;

    case 'list':
      //
      // Create accommodation option tabs - hut codes (aka accommodation options)
      foreach(VM_hutCodes()->getMembers() as $lm_id=>$r){
	$this->tabs_hutCodes[$r['lm_value']] = $r['lm_key'];
      }

      // Since we will read only the rooms for a given tab (see functions "rejectRecord" & "query")
      // lets point on the selected tab from the beginning.
      locateAndInclude('b_tabs');
      $this->b_tabs_hutCodes = new b_tabs_myPear(array_keys($this->tabs_hutCodes));
      foreach(array_keys($this->tabs_hutCodes) as $tab)	if ($this->b_tabs_hutCodes->body()) break;

      $hutCodes = array_values($this->tabs_hutCodes);
      $this->current_hutCode = $hutCodes[$this->b_tabs_hutCodes->active_tab()];

      //
      // Create the second level tabs - huts themselves
      $q = self::_queryAps(" WHERE hut_code = '".$this->current_hutCode."' GROUP BY hut_id ");
      while($r = myPear_db()->next_record($q)) $this->tabs_huts[$r['hut_name']] = $r['hut_id'];
      
      $this->b_tabs_huts = new b_tabs_myPear(array_keys($this->tabs_huts));
      foreach(array_keys($this->tabs_huts) as $tab) if ($this->b_tabs_huts->body()) break;

      $huts = array_values($this->tabs_huts);
      $this->current_hut = $huts[$this->b_tabs_huts->active_tab()];

      // Initialise tables
      $this->t = new b_table_zebra($this->header());
    }
  }
  
  /*
   *
   */
  function query(){
    switch ($this->doing){
    case 'usage':
      $w[] = "(a_status = '".AP_AUTO_ALLOCATABLE."' OR a_name REGEXP 'Nordita')";
      $w[] = "a_start <= $this->rt";
      $w[] ="(a_end   >= $this->rf OR a_end = 0)";
      $q = myPear_db()->query("SELECT * FROM abs_rooms WHERE ".join(' AND ',$w).
				" ORDER BY $_GET[sBy] ".@$_GET['sByDir']); 
      return $q;
      
    case 'list':
      // Skip all the records which do not correspond to the "active tab"
      return self::_queryAps(" WHERE hut_code = '$this->current_hutCode' AND hut_id = '$this->current_hut'");
      
    case 'list2':
      return myPear_db()->query("SELECT * FROM abs_huts ORDER BY hut_name");
    }
  }
    
  function header(){
    switch ($this->doing){
    case 'usage': $header = array('a_name'      => 'name',
				  'a_price'     => 'monthly<br/>rent',
				  'n_nosort'    => 'N',
				  'usage_nosort'=> 'usage',
				  'rent_nosort' => 'rent<br/>cost',
				  'clean_nosort'=> 'cleaning<br/>cost',
				  ); break;
      
    case 'list2':
    case 'list':  $header = array('a_name'    => 'name',
				  'a_comment' => 'comment',
				  'a_price'   => 'monthly<br/>rent',
			        //'a_price_cleaning'  => 'cleaning<br/>fee',
				  'a_area'    => 'area<br/>(kvm)',
				  'a_capacity'=> 'capacity<br/>(beds)',
				  //'a_status'  => 'status',
				  ); break;
    }

    if (!@$header[@$_GET['sBy']]) $_GET['sBy'] = 'a_name'; 
    foreach ($header as $k=>$v) {
      if ($k!='a_name') 	$this->th_attr[$k] = "style='text-align:right;'";
    }
    return $header; 
  }

  /*
   *
   */  
  function format() {

    $this->a = myPear::getInstance_new('bForm_vm_Room', @$this->rec['a_id'],'strict');

    if (empty($this->rec['n_nosort'])) 	  $this->rec['n_nosort'] = 0;
    if (isset($this->rec['a_start'])) if ($this->rec['a_start'] <= rent0) unset($this->rec['a_start']); 

    if (isset($this->rec['a_end'])){
      if ($this->rec['a_end'] < time()){
	foreach(array_keys($this->header()) as $k) $this->t->class[$k] = array('overstrike','opaqueText');
      }
      if ($this->rec['a_end']   >= rent9) unset($this->rec['a_end']); 
    }

    $this->rec['a_price'] = (strpos($this->rec['a_price'],'=') === False
			     ? b_fmt::escape(b_fmt::money(@$this->rec['a_price']))
			     : str_replace(',','<br/>',$this->rec['a_price']));
    
    $cleanCost = $usage = 0;
    
    $s = @$this->rec['a_status'];

    if     ($s == AP_SHARABLE)           $extraTD = $this->t->extraTD[] = bIcons()->get(array('i'=>'bb-2users',  'd'=>$s));
    elseif ($s == AP_AUTO_ALLOCATABLE) $extraTD = $this->t->extraTD[] = bIcons()->get(array('i'=>'i-pc_edit','d'=>$s));
    else                               $extraTD = $this->t->extraTD[] = '';
    
    switch ($this->doing){
    case 'usage':
      $day1 = max($this->rf,$this->a->getValue('a_start',1));
      $day9 = min($this->rt,$this->a->getValue('a_end',  1));
      $rent = bForm_vm_Room::_dailyRent($this->a->formDB) * b_time::diff($day9,$day1);
      $this->rec['rent_nosort'] = b_fmt::escape(b_fmt::money($rent));
      $this->rent += $rent;

      if ($this->a->getValue('a_end',1) >= $this->rt) unset($this->rec['a_end']);
      //if (!$this->rec['a_end']) $this->rec['a_end'] = date('Y-m-d',$day9);

      $q = myPear_db()->query("SELECT * FROM abs_leases WHERE lease_start <= $this->rt AND lease_end >= $this->rf AND lease_aid="
			      .$this->a->ID." ORDER BY lease_start");
      while($r=myPear_db()->next_record($q)){
	$cleanCost += $this->a->fee();
	$this->rec['n_nosort']++;
	$usage += b_time::diff($r['lease_end'],$r['lease_start']);
      }
      $this->usage += $usage;
      $this->ndays += b_time::diff($day9,$day1);
      $this->cleanCost += $cleanCost;
      $this->rec['clean_nosort'] = b_fmt::escape(b_fmt::money($cleanCost));
      $this->rec['usage_nosort'] = ((int)($usage*100/b_time::diff($day9,$day1))).'%';
      $this->t->extraTD[] = x('i',b_time::period(@$this->rec['a_start'],@$this->rec['a_end'])); 
      break;
      
    case 'list':
      $this->t->extraTD[] = $extraTD;
      $this->format_record_axi($this->rec);
      break;
      
    case 'list2':
      $q = self::_queryAps(" WHERE hut_id = ".$this->rec['hut_id']);
      $this->t->tr();
      
      $button = bIcons()->get('i-edit');
      $this->t->tr(x("a href='".b_url::same("?form=vm_Hut&id=".$this->rec['hut_id'])."'",$this->rec['hut_name']),
		   array('colspan='.(1+count($this->header()))));
      
      $this->t->counter = 0;
      while($r = myPear_db()->next_record($q)) {
	$this->t->extraTD = array();
	$this->a = myPear::getInstance_new('bForm_vm_Room',$r['a_id'],'fatal');
	$this->format_record_axi($r);
	$this->t->prt($r);
      }
      $this->rec = array();
    }
  }

  private function format_record_axi(&$rec){
    if (!is_object($this->a)) return; 
    $this->t->extraTD[]= ($this->a->isWritable()
			  ? bIcons()->getButton(array('l'=>b_url::same("?form=vm_Room&id=".$this->a->ID),
						      'd'=>'modify',
						      'i'=>'i-edit'))
			  :'');
    
    $this->t->extraTD[]=(!$this->a->isUsed() && $this->a->isWritable()
			 ? bIcons()->getButton(array('l'=>b_url::same("?delete_once=".$this->a->ID),
						     'd'=>btn_deleteAp,
						     'i'=>'i-drop'))
			 :'');
    $this->t->extraTD[] = x('i',b_time::period(@$rec['a_start'],@$rec['a_end'])); 
  }
  
  function listEntries_summary(){
    switch ($this->doing){
    case 'list': 
      bIcons()->reset();
      $this->b_tabs_huts->close();
      $this->b_tabs_hutCodes->close();

      // Optionally draw edit buttons at the bottom
      $edit_buttons = array();
      if (VM_hutCodes()->isWritable() && VM_hutCodes()->isAddable()) {
	$hut = new bForm_vm_Hut($this->current_hut);
	
	$edit_buttons[] = b_btn::submit('add new room to '.$hut->name(),
					b_url::same('?id=new&form=vm_Room&clean_once=act_code,act_id&a_hutid='.$hut->ID));
	$edit_buttons[] = b_btn::submit("add new ".VM_hutCodes()->memberTitle(),
					VM_hutCodes()->get_action('add'),
					$confirm=True);
      }
      $this->t->tr(b_btn::UL_buttons($edit_buttons),'colspan=6');
    }
  }

  function getAllEvents(){
    $this->dbg(); 
    $q = $this->query();
    $list = array();
    while ($r=myPear_db()->next_record($q)) $list[]=$r['a_id'];
    return $list;
  }                                                                                                                                                                                               

  /*
   *
   */
  function rejectRecord(){ 

    switch ($this->doing){
    case 'list':
      static $jjj = 0;
      // Skip all the records which do not correspond to the "active tab"
      $reply = ($this->current_hutCode != $this->rec['hut_code']);
      break;
      
    default:
      $reply = False;
    }
    $this->dbg($reply);
    return $reply;
  }

  public static function rejectExtApp(){
    return "a_id != ".VM::$OA->ID;
  }
  
  /*
   *
   */
  public static function _queryAps($where=''){
    if (!myPear_db()->columnExists(@$_GET['sBy'],'abs_rooms')) $_GET['sBy'] = 'a_name';
    $q = myPear_db()->query("SELECT * FROM abs_huts ".
			    " LEFT JOIN abs_rooms ON hut_id = a_hutid ".
			    $where.
			    " ORDER BY hut_name,$_GET[sBy] ".@$_GET['sByDir']);
    return $q;
  }
}