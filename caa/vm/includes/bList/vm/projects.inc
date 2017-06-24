<?php
/*
 * List of financial projects for the organization.
 * The projects are identified by the "project code", 
 * and (optionally) by the human-readable "project name".
 * Usually projects are valid for 1 financial year.
 *
 * Record structure:
 * lm_key    - project name, human readable 
 * lm_value  - project code
 * lm_status - 'RO' or e_id (comma separated if multiple events are feeded from the same project)
 * p_budget  - budget allocated for the project
 * p_year    - (optional) year when the project is valid
 */
locateAndInclude('bList_vm');
locateAndInclude('b_checkBox');

define('project_builtin' , 'Built-in');
define('project_basic'   , 'Basic');

class bList_vm_projects extends bList_vm{

  protected $isReady = True;
  protected $orderBy ='lm_value';

  function __construct($org_id){

    parent::__construct(myOrg_ID);

    // Create a few built-in projects for:
    // - renting out apartments
    // - own accommodation
    // - break in the visit
    foreach(array(VISIT_TYPE_RENT, LODGING_OA, LODGING_BR) as $project){
      if (!$this->getMember(array('lm_key'=>$project))){
	$this->updateMember(array('lm_status'=>'RO',
				  'lm_value' =>VM::$description[$project]['d']),
			    $this->addMember(array('lm_key'=>$project)),
			    False);
      }
    }
    $VM_show_old_projects = b_cnf::get('VM_show_old_projects');
    $this->extra_buttons[] = b_btn::submit(($VM_show_old_projects?'Hide':'Show')." old projects",
					   b_url::same('?VM_show_old_projects='.($VM_show_old_projects?'0':'1')),
					   $confirm=False);
  }

  /*
   *
   */
  public static function e2project_id($e_id){
    $ids = array();
    $q = myPear_db()->qquery("SELECT lm_id FROM zzz_list_members LEFT join zzz_lists ON l_id=lm_lid WHERE l_class = '".__class__."' AND lm_status = '$e_id'",cnf_dev);
    while($r=myPear_db()->next_record($q)){
      $ids[] = $r['lm_id'];
    }
    sort($ids);
    $reply = join(',',$ids);
    b_debug::_debug($reply,cnf_dev);
    return $reply;
  }
  
  protected function isMemberDeletable(){
    return False;
  }

  function isWritable(){
    return VM_access()->mayDoAccounting() || parent::isWritable();
  }

  protected function unpack($buffer=array(), $updates=array()){
    $buffer = parent::unpack($buffer,$updates);
    if (!empty($buffer['e_name'])) $name = date('M Y - ',$buffer['e_start']) . $buffer['e_name'];
    else                           $name = $buffer['lm_key'];
    $buffer['project'] = sprintf('%s : %s', $buffer['lm_value'], $name); 
    return $buffer;
  }

  /*
   * Get project number for the event
   */
  private $getProjectID_cache = Null;
  function getProjectID($e_id){
    if ($this->getProjectID_cache === Null){
      $this->getProjectID_cache = array();
      foreach ($this->getMembers() as $lm_id=>$r){ 
	foreach(explode(',',$r['lm_status']) as $id){
	  $this->getProjectID_cache[$id] = $r['lm_id'];
	}
      }
    }
    return @$this->getProjectID_cache[$e_id];
  }

  /*
   * Get project name from the ID
   */
  function getProjectName($lm_id){
    if (empty($lm_id)){
      $reply = array(0,'?');
    }else{
      $m = $this->getMember($lm_id);
      $reply = array($m['lm_value'],$m['lm_key']);
    }
    //b_debug::print_r($reply);
    return $reply;
 }

  /*
   * Get built-in (hence read-only) projects
   */
  private $get_projects_cache = Null;
  function get_projects($ROonly = True){
    if ($this->get_projects_cache === Null){
      $this->get_projects_cache = array();
      foreach ($this->getMembers() as $id=>$r){
	$this->get_projects_cache['all'][$r['lm_id']] = $r['lm_key'];
	if ($r['lm_status'] == 'RO') $this->get_projects_cache['RO'][$r['lm_id']] = $r['lm_key'];
      }
    } 

    if ($ROonly) $reply = $this->get_projects_cache['RO'];
    else         $reply = $this->get_projects_cache['all'];
    return $reply;
  }

  protected function message_addMember($r){
    $this->dbg($r,cnf_dev);
    return parent::message_addMember($r);
  }

  protected function getFormAdd(){
    $this->bForm(array('active project?'=>'lm_option',
		       'project name'   =>'lm_key',
		       $this->memberTitle()=>'lm_value'));
    $this->f->checkBox('lm_option');
    $this->f->textHidden('lm_key','lm_key'.date('ms',time()));
    $this->f->textField('lm_value');
    $lm_value = @$this->formDB_member['lm_value'];
    $this->f->defineVariable('event','lm_status');
    $this->f->selectMenu('lm_status',$this->get_events_selector());
    $this->f->closeForm();
  }

  protected function header($arg=Null){
    return array('lm_value'  =>'project<br/>number',
		 'event_name'=>'',
		 'lm_option' =>'active?');
  }

  function is_projectActive($rec){
    $reply = (b_checkBox::isOff($rec['lm_option']) ? '' : 'checked');
    return $reply;
  }
  
  function set_projectActivity($id,$value=True){
    myPear_db()->query("UPDATE zzz_list_members SET lm_option = '".($value?'yes':'no')."' WHERE lm_id = $id");
  }

  private static $tabs = array();
  protected function getTabsForListing(){

    $this->sortArgs = " ORDER BY lm_status ";
    foreach($this->getMembers() as $id=>$r){
      if ($this->record_rejected($r)) continue;
      
      unset($tab);
      foreach($this->extract_events($r) as $e) $tab = date('Y',$e->getValue('e_start'));
      
      if (!isset($tab) && ($r['lm_status'] != 'RO')) $tab = project_basic;
      if ( isset($tab)) @self::$tabs[$tab]++;
    }
    krsort(self::$tabs);
    return array_keys(self::$tabs);
  }

  protected function defaultTabForListing($tabs){
    foreach($tabs as $k=>$y){
      if ($y == date('Y',time())) return $k;
    }
    return count($tabs) - 1;
  }

  protected function hookJOIN(){
    return join(' ',array(" LEFT JOIN abs_events ON e_id = lm_status ",
                          parent::hookJOIN()));
  }

  protected function getMembers_forListing($tab_number=Null){
    
    $keys = array_keys(self::$tabs);
    switch($what = $keys[$tab_number]){
    case project_basic:
      $this->getMembers_where = array(" (e_id IS NULL AND lm_status != 'RO') ");
      return $this->getMembers();
      break;
      
    case project_builtin:
      return $this->getMembers(array('lm_status'=>'RO'));

    default:
      $this->sortArgs = " ORDER BY e_start ";
      $this->getMembers_where = array('e_start >= '.b_time::noon($what, 1, 1),
                                      'e_start <= '.b_time::noon($what,12,31),
                                      );
      return $this->getMembers();
    }
  }
  
  /*
   * Do not show old/non-active projects by default
   */
  protected function record_rejected($r=Null) {
    if (empty($r)) $r = $this->r;
    $to_show = b_cnf::get('VM_show_old_projects');
    if (!$to_show){
      if ($this->is_projectActive($r)){
	if ($events = $this->extract_events($r)){
	  foreach($events as $e){
	    if (date('Y',$e->getValue('e_start',1)) > date('Y',time()) - VM_OLD_PROJECTS) $to_show = True;
	  }
	}else{
	  $to_show = True;
	}
      }
    }
    return !$to_show;
  }

  /*
   * Helper - extract event(s) year/agenda_code for the project
   */  
  private function extract_events($r){
    $dejaVu = 0;
    static $events = array();

    if ($dejaVu != $r['lm_id']) $events = array();
    $dejaVu = $r['lm_id'];

    if (empty($events)){
      foreach(explode(',',$r['lm_status']) as $id){
	if (myPear::is_int($id)){
	  if (is_object($e = myPear::getInstance_new('bForm_vm_Event',$id,'strict'))) $events[] = $e; 
	}elseif(cnf_dev){
	  // Should not happend...
	}
      }
    }
    return $events;
  }

  protected function format(){
    $event_urls = array();
    foreach($this->extract_events($this->r) as $e){
      // Check that the event associated with the project is in agenda sync list,
      // add it if it is not
      VM_agendaEvents()->addMember(array('lm_key'=>'conf','lm_value'=>$e->getValue('e_code',1)));
      // Provide click to the event
      $event_urls[] = bJS()->modal_iframe(VM_agenda()->getEventURL($e->getValue('e_code',1)),
					  b_fmt::strcut($e->name(),60,'...'));
    }
    $this->r['event_name'] = join('<br/>',$event_urls);
    
    if (empty($this->r['event_name'])){
      if ($this->r['lm_status'] == 'RO'){
	  $this->r['event_name'] = $this->r['lm_key']. ' - '. x('i',$this->r['lm_value']);
	  $this->r['lm_value'] = 'built-in';
      }else{
	$this->r['event_name'] = x('i',$this->r['lm_key']);
      }
    }

    $this->r['lm_status'] = x('i',$this->r['lm_status']);
    $this->r['lm_option'] = "<input type='checkbox' name='b".$this->r['lm_id']."' disabled ".$this->is_projectActive($this->r).'/>'; 

    $this->r['lm_value'] = b_fmt::longString($this->r['lm_value'].'&nbsp;',15,False);
    $this->t->css['lm_value']['align'] = 'align_left';
  }

   private function get_events_selector(){
     locateAndInclude('bForm_vm_Event');
     $select = array();
     $q = myPear_db()->query("SELECT e_id FROM abs_events ORDER BY e_start DESC");
     while($r=myPear_db()->next_record($q)){
       $e = new bForm_vm_Event($r['e_id']);
       $e->defineVariables();
       $list[$e->ID] = $e->name();
     }
     return  $list;
   }
   
}

