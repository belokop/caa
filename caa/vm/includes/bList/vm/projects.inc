<?php
/*
 * List of financial projects for the organization.
 * The projects are identified by the "project code", 
 * and (optionally) by the human-readable "project name".
 *
 * Record structure:
 * lm_key    - project name, human readable 
 * lm_value  - project code
 * lm_status - 'RO' or e_id (comma separated if multiple events are feeded from the same project)
 * p_budget  - budget allocated for the project
 * p_year    - (optional) year when the project is valid
 */
locateAndInclude('bList_vm');

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

  protected function getFormAdd(){
    $this->bForm(array('active project?'=>'lm_option',
		       'project name'   =>'lm_key',
		       $this->memberTitle()=>'lm_value'));
    $this->f->checkBox('lm_option');
    $this->f->textField('lm_key');
    $this->f->textField('lm_value');
    if ((int)$this->formDB_member['lm_value'] || strstr($this->formDB_member['lm_value'],'VCB')){
      $this->f->defineVariable('event','lm_status');
      $this->f->selectMenu('lm_status',$this->getEvents());
    }
    $this->f->closeForm();
  }

  protected function header(){
    return array('lm_value'  =>'project<br/>number',
		 'event_name'=>'',
		 'lm_option' =>'active?');
  }

  function is_projectActive($rec){
    return (($rec['lm_option'] === 'no') ? '' : 'checked');
  }

  function set_projectActivity($id,$value=True){
    myPear_db()->query("UPDATE zzz_list_members SET lm_option = '".($value?'yes':'no')."' WHERE lm_id = $id");
  }

  private static $tabs = array();
  protected function getTabsForListing(){

    $this->sortArgs = " ORDER BY lm_status ";
    foreach($this->getMembers() as $id=>$r){
      unset($tab);
      foreach(explode(',',$r['lm_status']) as $id){
	if (!is_numeric($id) || (int)$id == 0) continue;
	if (is_object($e = myPear::getInstance('bForm_vm_Event',$id,'strict'))) $tab = date('Y',$e->getValue('e_start'));
      }
      if (!isset($tab)){
	if ($r['lm_status'] == 'RO')	continue;
	if ($r['lm_status'] == 'RO')	$tab = project_builtin;
	else	                        $tab = project_basic;
      }
      @self::$tabs[$tab]++;
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

  protected function format(){
    $this->r['event_name'] = '';
    $names = array();
    foreach(explode(',',$this->r['lm_status']) as $id){
      if (is_object($e = myPear::getInstance('bForm_vm_Event',$id,'strict'))){
	// Check that the project is in the agenda sync list
	if ($e_code = (int)$e->getValue('e_code',1)){
	  VM_agendaEvents()->addMember(array('lm_key'=>'conf','lm_value'=>$e_code));
	  $names[] = bJS()->modal_iframe("http://agenda.albanova.se/conferenceDisplay.py?confId=$e_code",
						 b_fmt::strcut($e->name(),60,'...'));
	}
      }
    }
    $this->r['event_name'] = join('<br/>',$names);
    
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

   private function getEvents(){
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

