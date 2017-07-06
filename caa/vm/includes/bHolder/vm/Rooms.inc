<?php
locateAndInclude('bHolder');
locateAndInclude('bHolder_vm');
class bHolder_vm_Rooms extends bHolder_vm{

  var $rent      = 0;
  var $usage     = 0;
  var $ndays     = 0;
  var $autoOpenHeader = False;
  var $e = Null;

  private $b_tabs_accOptions = Null;
  private $b_tabs_huts     = Null;

  private $current_accOption = '';
  private $current_hutID = '';
  private $current_hutCode = '';
  private $current_hut = Null;

  public static function list_hut($hut_id,$e_id=Null){
    if (empty($hut_id)) $hut_id = $_REQUEST['arg_once'];
    if (!empty($e_id)) $_REQUEST['e_id_once'] = $_GET['e_id_once'] = $e_id;
    $holder = new bHolder_vm_Rooms('list_hut',$hut_id);
    $holder->listEntries();
  }

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

    /*  
    // Hook the event if requested
    if (!empty($_GET['e_id_once'])){
    $this->e = myPear::getInstance_new('bForm_vm_Event',$_GET['e_id_once']);
    print x('h3',$this->e->name());
    }
    */
    if (VM::_MENU()->context() == 'org'){
      print x('h3',myOrg_name);
    }else{
      $this->e = VM::$e;
      print x('h3',$this->e->name());
    }
    
    switch ($this->doing){
    case 'usage':
      $this->rf = b_time::noon(0, 1, 1);
      $this->rt = b_time::noon(0,12,31);
      $this->title .= ' '.b_time::period($this->rf,$this->rt);
      break;

    case 'list':
      //
      // Create accommodation option tabs (aka hut codes)
      foreach(VM_hutCodes()->getMembers() as $lm_id=>$r){
	$accOptions[$r['lm_value']] = $r['lm_key'];
      }

      // Since we will read only the rooms for the given tab (see functions "rejectRecord" & "query")
      // lets point on the selected tab from the beginning.
      locateAndInclude('b_tabs');
      $this->b_tabs_accOptions = new b_tabs_myPear(array_keys($accOptions));
      foreach(array_keys($accOptions) as $tab){
	if ($this->b_tabs_accOptions->body()) break; 
      }

      $hutCodes = array_values($accOptions);
      $this->current_accOption = $hutCodes[$this->b_tabs_accOptions->active_tab()];

      //
      // Create the second level tabs - rooms in the hut
      $q = self::_queryAps(" WHERE hut_code = '".$this->current_accOption."' GROUP BY hut_id ");
      while($r = myPear_db()->next_record($q)) $huts[$r['hut_name']] = $r['hut_id'];
      $this->b_tabs_huts = new b_tabs_myPear(array_keys($huts));

      if (empty($huts)){
	b_debug::traceBack('??? empty tabs_huts (i.e. no huts in the accommodation option)');
      }else{
	foreach(array_keys($huts) as $tab) if ($this->b_tabs_huts->body()) break;
	
	$huts = array_values($huts);
	$this->current_hutID = $huts[$this->b_tabs_huts->active_tab()];
      }
      // Initialise tables
      $this->new_t();
      break;

    case 'list_hut':
      // Initialise tables
      $this->new_t();

      $this->current_hutID = $title;
      $this->current_hut = new bForm_vm_Hut($this->current_hutID);
      myPear::H1($this->current_hut->name(),'noTranslate');
    }
  }
  
  /*
   *
   */
  function query(){
    switch ($this->doing){
    case 'usage':
      $w[] = "(a_status = '".ROOM_AUTO_ALLOCATABLE."' OR a_name REGEXP 'Nordita')";
      $w[] = "a_start <= $this->rt";
      $w[] ="(a_end   >= $this->rf OR a_end = 0)";
      $q = myPear_db()->query("SELECT hut_name,hut_id,a_id,a_start,a_end,a_name,a_comment,a_quantity,a_price FROM abs_rooms WHERE ".join(' AND ',$w).
				" ORDER BY $_GET[sBy] ".@$_GET['sByDir']); 
      return $q;
      
    case 'list':
      // Skip all the records which do not correspond to the "active tab"
      $where[] = "hut_code = '$this->current_accOption'";
    case 'list_hut':
      $where[] = "hut_id = '$this->current_hutID'";
      return self::_queryAps(" WHERE ".join(' AND ',$where));
      
    case 'list2':
      return myPear_db()->query("SELECT hut_name,hut_id,hut_code FROM abs_huts ORDER BY hut_name");
    }
  }
    
  function header(){
    switch ($this->doing){
    case 'usage': 
      $header = array('a_name'      => 'name',
		      'a_price'     => '<br/>price',
		      'n_nosort'    => 'N',
		      'usage_nosort'=> 'usage',
		      'rent_nosort' => 'rent<br/>cost',
		      ); 
      break;
      
    case 'list':  
    case 'list2':
    case 'list_hut':
      $header = array('a_name'    => 'Name',
		      'a_price'   => 'Price',
		      'a_quantity'=> 'N rooms<br/>available',
		      '_rooms'    => 'N rooms<br/>',
		      '_tenants'  => 'N tenants<br/>accomodated',
		      //'a_area'    => 'Area<br/>(kvm)',
		      //'a_capacity'=> 'Capacity<br/>(beds)',
		      //'a_status'  => 'Status',
		      //'a_comment' => '',
		      '_sharable' => '',
		      ); 
      break;
    }

    if (!@$header[@$_GET['sBy']]) $_GET['sBy'] = 'a_name'; 
    foreach ($header as $k=>$v) {
      if ($k!='a_name') $this->th_attr[$k] = "style='text-align:right;'";
    }
    if (VM::_MENU()->context() !== 'event'){
      unset($header['_rooms'],$header['_tenants']);
    }else{
      $header['_rooms'] = 'N rooms<br/>used/available';
      unset($header['a_quantity']);
    }
    return $header; 
  }

  /*
   *
   */  
  function format() {
    static $ok = 0;
    foreach($this->header() as $k=>$v) if ($k!=='a_name') $this->t->css[$k]['align'] = 'align_right';
    foreach(array('a_comment') as $k) $this->t->class[$k] = array('silverText');

    $this->a = myPear::getInstance_new('bForm_vm_Room', @$this->rec['a_id'],array('strict'));
    if (empty($this->a)){
      if (empty($ok)) myPear::ERROR("There is no rooms yet in \"".$this->rec['hut_name']."\", please add the first one");
      $this->rec = array();
      return;
    }
    $ok++;

    $this->rec['a_name'] = bForm_vm_Room::_a_name($this->rec,False);
    if ($this->rec['a_comment']) $this->rec['a_name'] .= '<br/>'.x('em class="smaller-text"',$this->rec['a_comment']);

    if (empty($this->rec['n_nosort'])) 	  $this->rec['n_nosort'] = 0;
    if (isset($this->rec['a_start'])) if ($this->rec['a_start'] <= myPear_day0) unset($this->rec['a_start']); 

    if (isset($this->rec['a_end'])){
      if ($this->rec['a_end'] < time()){
	foreach(array_keys($this->header()) as $k) $this->t->class[$k] = array('overstrike','silverText');
      }
      if ($this->rec['a_end']   >= myPear_day9) unset($this->rec['a_end']); 
    }

    // How many rooms, price, etc.
    if (VM::_MENU()->context() == 'event'){
      $room = new bForm_vm_Room($this->rec['a_id']);
      foreach(bList_vm_accommodationOptions::$whatIsSaved as $item){
	$this->rec[$item] = $room->getValue($item);
	//VM_accommodationOptions($this->e)->get_a_value($this->rec['a_id'],$this->rec['hut_id'],$item);
      }
      list($this->rec['_tenants'],
	   $_rooms) = bForm_vm_Lease::_room_usage($this->rec['a_id']);
      $a_quantity = VM_accommodationOptions(VM::$e)->get_a_value($room->ID,$room->getValue('a_hutid'),'a_quantity');
      if ($a_quantity == VM_quantity2request) $a_quantity = VM_quantity2request_text;
      $this->rec['_rooms'] = ($_rooms > 0
			      ? sprintf("%s/%s",$_rooms,$a_quantity)
			      : $a_quantity);
    }
    if ($this->rec['a_quantity'] == VM_quantity2request) $this->rec['a_quantity'] = VM_quantity2request_text;
    $this->rec['a_price'] = (strpos($this->rec['a_price'],'=') === False
			     ? b_fmt::escape(b_fmt::money($this->rec['a_price']))
			     : str_replace(',','<br/>',$this->rec['a_price']));
    
    $usage = 0;

    $this->rec['_sharable'] = (bForm_vm_Room::_mightBeSharable($this->rec)
			       ? 'Sharable'
			       : '');
    $extraTD = $this->t->extraTD[] = '';

    switch ($this->doing){
    case 'usage':
      $day1 = max($this->rf,$this->a->getValue('a_start',1));
      $day9 = min($this->rt,$this->a->getValue('a_end',  1));
      $rent = bForm_vm_Room::_daily_price($this->a->formDB,$this->e) * b_time::diff($day9,$day1);
      $this->rec['rent_nosort'] = b_fmt::escape(b_fmt::money($rent));
      $this->rent += $rent;

      if ($this->a->getValue('a_end',1) >= $this->rt) unset($this->rec['a_end']);

      $q = myPear_db()->query("SELECT lease_end,lease_start FROM abs_leases WHERE lease_start <= $this->rt AND lease_end >= $this->rf AND lease_aid="
			      .$this->a->ID." ORDER BY lease_start");
      while($r=myPear_db()->next_record($q)){
	$this->rec['n_nosort']++;
	$usage += b_time::diff($r['lease_end'],$r['lease_start']);
      }
      $this->usage += $usage;
      $this->ndays += b_time::diff($day9,$day1);
      $this->rec['usage_nosort'] = ((int)($usage*100/b_time::diff($day9,$day1))).'%';
      $this->t->extraTD[] = x('i',b_time::period(@$this->rec['a_start'],@$this->rec['a_end'])); 
      break;
      
    case 'list':
    case 'list_hut':
      $this->t->extraTD[] = $extraTD;
      $this->format_record_axi($this->rec);
      break;
      
    case 'list2':
      $this->t->tr();
      
      $button = bIcons()->get('i-edit');
      $this->t->tr(x("a href='".b_url::same("?form=vm_Hut&id=".$this->rec['hut_id'])."'",$this->rec['hut_name']),
		   array('colspan='.(1+count($this->header()))));
      
      $this->t->counter = 0;
      $q = self::_queryAps(" WHERE hut_id = ".$this->rec['hut_id']);
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
			  ? bIcons()->getButton(array('l'=>b_url::same("?form=vm_Room&id=".$this->a->ID.
								       (empty($this->e)?'':"&e_id_once=".$this->e->ID."&arg2_once=".$this->e->ID)),
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
    case 'list_hut':
      
    // Close the tabs
      bIcons()->reset();
      if (!empty($this->b_tabs_huts))       $this->b_tabs_huts->close();
      if (!empty($this->b_tabs_accOptions)) $this->b_tabs_accOptions->close();
      
      // Optionally draw edit buttons at the bottom
      $edit_buttons = array();
      if (!empty($this->current_hutID) && VM_hutCodes()->isWritable($this->current_hutCode)){
	if (empty($this->current_hut)) $this->current_hut = new bForm_vm_Hut($this->current_hutID);
	
	if (empty($this->e))            $edit_buttons[] = b_btn::submit('add new room to '.$this->current_hut->name(),
									b_url::same('?id=new&form=vm_Room&clean_once=act_code,act_id&a_hutid='.$this->current_hutID));
	if (VM_hutCodes()->isAddable()) $edit_buttons[] = b_btn::submit("add new ".VM_hutCodes()->memberTitle(),
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
      $reply = ($this->current_accOption != $this->rec['hut_code']);
      break;
      
    default:
      $reply = False;
    }
    if (!$reply) $this->current_hutCode = $this->rec['hut_code'];
    $this->dbg($reply);
    return $reply;
  }

  public static function reject_self_accommodated(){
    return sprintf("a_id NOT IN (%s,%s)",VM::$OA->ID,VM::$BR->ID);
  }
  
  /*
   *
   */
  public static function _queryAps($where='',$sBy=''){
    if (!myPear_db()->columnExists(@$_GET['sBy'],'abs_rooms')) {$_GET['sBy'] = 'a_capacity'; $_GET['sByDir'] = 'ASC';}
    $_GET['sBy'] = 'a_capacity';
    $q = myPear_db()->qquery("SELECT hut_name,hut_id,hut_code,a_id,a_start,a_end,a_name,a_comment,a_quantity,a_price,a_capacity FROM abs_huts ".
			     " LEFT JOIN abs_rooms ON hut_id = a_hutid ".
			     $where.
			     " ORDER BY CONCAT(hut_name,$_GET[sBy]) ".@$_GET['sByDir'],cnf_dev);
    return $q;
  }
}
