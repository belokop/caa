<?php
/*
 * Member format:
 *  lm_key    - entry type, either 'conf' (conference) or 'cat' (category)
 *  lm_value  - entry ID, as it is in the agenda system
 *  lm_status - a flag saying is the event in the VM scope 
 */

// workaround
// vm_compat_update_36();
define('agenda_url','http://agenda.albanova.se/');

locateAndInclude('bList_vm');
class bList_vm_agendaEvents extends bList_vm{

  var $list_quering = array('lm_key'    =>'event type',
                            'lm_value'  =>'event code');
  var $list_showing = array('_event'    =>'associated event',
			    '_lm_value' =>'code',
			    '_lm_status'=>'VM<br/>event?');
  var $orderByDir = 'DESC';
  protected $packed_items= array('a_name',);

  protected $isReady  = True;

  function __construct(){
    parent::__construct(myOrg_ID);

    $VM_show_old_events = b_cnf::get('VM_show_old_events');
    $this->extra_buttons[] = b_btn::submit(($VM_show_old_events?'Hide':'Show')." old&out_of_scope events",
                                           b_url::same('?VM_show_old_events='.($VM_show_old_events?'0':'1')),
                                           $confirm=False);
  }
  
  /*
   * Return array($category_name,$confs), where $conf is:
   *   Array(
   *     [296] => Array(
   *             [title] => Physics of Distributed Information Systems (PhysDIS)
   *             [start] => 2008-05-05 )
   *     [322] => Array(
   *             [title] => Origins of Homochirality
   *             [start] => 2008-02-01 )
   *     ...
   */
  function getCategoryInfo($catID){
    static $cats = array();
    static $confs = array();
    
    if (empty($cats[$catID])){
      $url = sprintf("%s/tools/getConf.py?catinfo=%s",agenda_url,$catID);
      $xml = `curl $url`; 
      b_debug::var_dump($xml,"xml output");
      $xml = simplexml_load_string($xml);
      $json= json_encode($xml);
      
      foreach(json_decode($json,TRUE) as $categories){
	foreach ($categories as $k=>$v){
	  switch($k){
	  case 'id':
	    $id = $v;
	    break;
	  case 'name':
	    $cats[$id] = $v;
	    break;
	  case 'event':
	    $confs = array();
	    foreach($v as $data){
	      foreach(array('title','start','protected') as $item){
		if (!empty($data[$item])) $confs[$data['id']][$item] = $data[$item];
	      }
	    }
	  }
	}
      }
      ksort($confs,SORT_NUMERIC);
    }
    $reply = array($cats[$catID],$confs);
    return $reply;
  }

  /*
   * Scan the Agenda categories listed in "cat" record and update the VM events list
   */
  function syncEventsInCategories($verbose=cnf_dev){
    $toAdd = $toRemove = array();
    foreach($this->getMembers(array('lm_key' => 'cat')) as $catEntry){
      if (empty($catEntry['lm_value'])){
	if ($verbose) var_dump('????????????????',$catEntry);
	continue;
      }
      list($name,$confs) = $this->getCategoryInfo($catEntry['lm_value']);
      $sortedByDate = array();
      foreach($confs as $confId=>$conf){
	$r = $this->getMember(array('lm_key'=>'conf','lm_value'=>$confId));
	if (!empty($conf['protected'])){
	  // Protected conferences are ignored by VM
	  // Usually those are either cancelled or non-approved events
	    $conf['status'] = 'protected';
	    if (empty($r['lm_id'])){
	      $conf['act'] = 'IGNORE';
	    }else{
	      $conf['act'] = 'REMOVE';
	      $toRemove[$r['lm_id']] = $conf['title'];
	    }
	}else{
	  if (empty($r)){
	    // Conference to be added to the list for sync (optionally as non-VM object) 
	    $toAdd[] = (array('lm_status' => (b_time::txt2unix($conf['start'])+60*24*3600 > time() ? 'on' : 'off'),
			      'lm_key'    => 'conf',
			      'lm_value'  => $confId,
			      'a_name'    => bForm_vm_Event::_name(array('e_start'=>b_time::txt2unix($conf['start']),
									 'e_code' =>$confId,
									 'e_name' =>$conf['title']))));
	    $conf['url'] = x('a href='.sprintf("%s/conferenceDisplay.py?confId=%s",agenda_url,$confId),"conf $confId");
	    $conf['status'] = '?';
	    $conf['act'] = 'ADD';
	  }else{
	    $conf['status'] = (b_checkbox::isOff($r['lm_status'])
			       ? 'Not in scope'
			       :'OK');
	  }
	}
	$conf['id'] = $confId;
	$sortedByDate[$conf['start']] = $conf;
      }

      //
      // Show the events
      //
      if ($verbose){
	// b_debug::print_r($toAdd,'to Add','nosort');
	// b_debug::print_r($toRemove,'to Remove','nosort');
	ksort($sortedByDate);
	$t = new b_table_zebra(array('year'=>'','title'=>'','url'=>'','start'=>'','status'=>''));
	$t->tr("<h3>$name</h3>",array('colspan=5'));
	$year = 0;
	foreach($sortedByDate as $date=>$conf){
	  $y = date('Y',b_time::txt2unix($date));
	  if ($y != $year) $t->prt(array('year'=>$y));	$year = $y;
	  if (!empty($conf['act'])) $t->extraTD = array($conf['act']);
	  $t->prt($conf);
	}
	$t->close();
      } 
    }

    // Add & Remove the events
    if ($c = count($toRemove)) $act[] = "$c event(s) to be removed";
    if ($c = count($toAdd))    $act[] = "$c event(s) to be added";
    if (!empty($act)) myPear::MESSAGE(join(' and ',$act).' in the Agenda-sync list');
 
    foreach($toAdd as $args){
      myPear::MESSAGE("Adding event \"".$args['a_name']."\"");
      $this->addMember($args);
    }

    foreach ($toRemove as $lm_id=>$title){
      myPear::MESSAGE("Removing protected event \"$title\" from VM");
      $this->deleteMember($lm_id);
    }
  }

  private $new_item = array('new event');
  protected function message_addMember($r){
    if(@$r['lm_key'] === 'cat') $this->new_item = array('category',$r['lm_value']);
    myPear::MESSAGE('Adding '.join(' ',$this->new_item).' to the Agenda-sync list"');
  }

  protected function defaultTabForListing($tabs){
    foreach($tabs as $k=>$y){
      if ($y == date('Y',time())) return $k;
    }
    return count($tabs) - 1;
  }

  function getFormAdd_OK(){
    if ($reply = parent::getFormAdd_OK()){
      if(@$_POST['lm_key'] === 'cat'){
	list($_POST['a_name'],$confs) = $this->getCategoryInfo($_POST['lm_value']); 
	$this->new_item = array('category',x('"',$_POST['a_name']));
      }elseif(@$_POST['lm_key'] === 'conf'){
	$confId = $_POST['lm_value'];
	VM_agenda()->sync(array("conf=$confId&show_reg=all"));
	if (is_object($ev = myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$confId),'strict'))){
	  $_POST['a_name'] = $ev->name();
	  $this->new_item = array(x('"',$_POST['a_name']));
	}elseif(cnf_dev){
	  // Mistyped event code?
	  $this->dbg("??? can't create bForm_vm_Event(e_code='$confId')",True);
	}
      }else{
	$reply = False;
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  private function is_ourEvent($rec){
    $reply = !b_checkBox::isOff(@$rec['lm_status']);
    return $reply;
  }

  /*
   * Do not show old/non-active events by default
   */
  protected function record_rejected($r=Null) {
    if (empty($r)) $r = $this->r;
    $to_show = b_cnf::get('VM_show_old_events') || ($r['lm_key'] == 'cat');
    if (!$to_show && $this->is_ourEvent($r)){
      if (is_object($e = myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$r['lm_value']),'strict'))){
	if (date('Y',$e->getValue('e_start',1)) > date('Y',time()) - VM_OLD_PROJECTS) $to_show = True;
      }
    }
    return !$to_show;
  }

  function isWritable(){
    $reply = VM_program_coordinator_here;
    return $reply;
  }

  /*
   *
   */
  static function eventOutOfScope($e_code){
    if (is_object($e_code)) $e_code = $e_code->getValue('e_code',1);
    if (is_object(VM_agendaEvents())){
      $rec  = VM_agendaEvents()->getMember(array('lm_key'=>'conf','lm_value'=>$e_code));
      return !VM_agendaEvents()->is_ourEvent($rec);
    }else{
      return False;
    }
  }

  /*
   *
   */
  protected function getFormAdd(){
    $this->bForm(array('event type' => 'lm_key',
		       'event code' => 'lm_value',
		       'process by VM?-SKIP-<br/>'.
		       '<i>no accommodation if unchecked<br/>'=>'lm_status'));
    $this->f->selectMenu('lm_key',array('conf'=>'conference','cat'=>'category','default'=>'conf'));
    $this->f->textField('lm_value');
    $this->f->checkBox('lm_status');
    $this->f->closeForm();
  }
  
  protected function hookJOIN(){
    $reply = array(parent::hookJOIN());
    if (!$this->nojoin) array_unshift($reply," LEFT JOIN abs_events ON e_code = lm_value ");
    return join(' ',$reply);
  }
  
  private static $tabs = array();
  private static $cat = 'Categories';

  protected function getTabsForListing(){
    foreach($this->getMembers() as $id=>$r){
      if($r['lm_key']=='cat'){
	$tab = self::$cat;
      }elseif($e=myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$r['lm_value']),'strict')){
	if ($this->record_rejected($r)) continue;
	$tab = date('Y',$e->getValue('e_start'));
      }elseif(cnf_dev){
	$this->dbg("??? can't create bForm_vm_Event(e_code='$r[lm_value]')",True);
	b_debug::print_r($r,"lm_id=$id");
      }
      @self::$tabs[$tab]++;
    }
    ksort(self::$tabs);
    return array_keys(self::$tabs);
  }

  /*
   *
   */
  protected $nojoin = False;
  protected function getMembers_forListing($tab_number=Null){
    $keys = array_keys(self::$tabs);

    if ($keys[$tab_number] == self::$cat){
      // All Categories
      $this->nojoin = True; // Avoid hooking events with the catID  
      $reply = $this->getMembers(array('lm_key'=>'cat'));
    }else{
      // Events for this year      
      $this->nojoin = False; 
      $this->sortArgs = " GROUP BY lm_value ORDER BY e_start ";
      $this->getMembers_where = array('lm_key  = "conf"',
				      'e_start >= '.b_time::noon($keys[$tab_number]-1,12,31),
				      'e_start <= '.b_time::noon($keys[$tab_number],  12,31),
				      );
      $reply = $this->getMembers();
    }
    return $reply;
  }
  
  /*
   *
   */
  protected function format($secondCall=False){
    $this->r['_lm_value'] = $this->r['lm_value'];
    $this->r['_lm_status']= '';
    if($this->r['lm_key']=='cat'){
      if (empty($this->r['a_name'])){
	list($this->r['a_name'],$confs) = $this->getCategoryInfo($this->r['lm_value']); 
      }
      $this->r['_event'] = bJS()->modal_iframe(VM_agenda()->getCategoryURL($this->r['lm_value']),
					       x('strong',"Category \"".$this->r['a_name']."\""));
    }elseif($e=myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$this->r['lm_value']),'strict')){
      $this->r['_event']    = bJS()->modal_iframe(VM_agenda()->getEventURL($e->getValue('e_code')),$e->name(True,80));
      $this->r['_lm_status']= ($this->is_ourEvent($this->r)
			       ? bIcons()->get('i-ok')
			       : bIcons()->get('i-notok-small'));
    }else{
      if ($secondCall){
	$this->r['_event'] = b_fmt::redText("Unknown event code '".$this->r['lm_value']."'");
      }else{
	// Auto-sync the newly accepted events
	VM_agenda()->sync(array("conf=".$this->r['lm_value']."&show_reg=all"));
	$this->format(True); 
      }
    }
    $this->r['_lm_value'] = x('i',$this->r['lm_key'].' '. $this->r['lm_value']);
  }
}
