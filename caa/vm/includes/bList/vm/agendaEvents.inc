<?php
/*
 * Member format:
 *  lm_key    - entry type, either 'conf' (conference) or 'cat' (category)
 *  lm_value  - entry ID, as it is in the agenda system
 *  lm_status - a flag saying is the event in the VM scope 
 */

// workaround
// vm_compat_update_36();
define('agenda_url','http://bgenda.albanova.se/');

locateAndInclude('bList_vm');
class bList_vm_agendaEvents extends bList_vm{

  var $debug=True;
  var $list_quering = array('lm_key'   =>'event type',
                            'lm_value' =>'event code');
  var $list_showing = array('event'    =>'associated event',
			    'lm_value' =>'code',
			    'lm_status'=>'VM<br/>event?');
  var $orderByDir = 'DESC';

  protected $isReady  = True;

  function __construct(){
    parent::__construct(myOrg_ID);
    $this->getEventsForCategory();
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
      $xml = simplexml_load_string(`curl $url`); 
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
      //      print'<pre>';    print_r($confs);exit;
    }
    return array($cats[$catID],$confs);
  }

  function getEventsForCategory(){
    foreach($this->getMembers(array('lm_key' => 'cat')) as $catEntry){
      list($name,$confs) = $this->getCategoryInfo($catEntry['lm_value']);
      $sortedByDate = array();
      $cccccc = 0;
      foreach($confs as $id=>$conf){
	if ($r = $this->getMember(array('lm_key'=>'conf','lm_value'=>"$id"))){
	  $conf['status'] = (empty($r['lm_status']) || (strToLower($r['lm_status'])=='no')
			     ? 'X'
			     :'YES');
	}else{
	  $conf['url'] = x('a href='.sprintf("%s/conferenceDisplay.py?confId=%s",agenda_url,$id),"conf $id");
	  $conf['status'] = '?';
	}
	if (!empty($conf['protected'])) $conf['status'] = 'protected';
	$conf['id'] = $id;
	$sortedByDate[$conf['start']] = $conf;
	if ((int)$id == 5552) var_dump($conf);
      }
      ksort($sortedByDate);
      $t = new b_table_zebra(array('year'=>'','title'=>'','url'=>'','start'=>'','status'=>''));
      $t->tr("<h3>$name</h3>",array('colspan=5'));
      $year = 0;
      foreach($sortedByDate as $date=>$conf){
	$y = date('Y',b_time::txt2unix($date));
	if ($y != $year) $t->prt(array('year'=>$y));	$year = $y;
	$t->prt($conf);
      }
      $t->close();
    } 
  }

  private $new_item = array();
  protected function message_addMember($r){
    myPear::MESSAGE('Adding '.join(' ',$this->new_item).' to the "Agenda-sync" list"');
  }

  protected function defaultTabForListing($tabs){
    foreach($tabs as $k=>$y){
      if ($y == date('Y',time())) return $k;
    }
    return count($tabs) - 1;
  }

  function getFormAdd_OK(){
    if ($reply = parent::getFormAdd_OK()){
      if(empty($_POST['lm_key'])){
	$reply = False;
      }else{
	VM_agenda()->sync(array("conf=".($conf=$_POST['lm_value'])."&show_reg=all"));
	// Mistyped event code?
	if (is_object($ev = myPear::getInstance('bForm_vm_Event',"e_code=$conf",array('strict')))){
	  $this->new_item = array($ev->name());
	}
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  function isWritable(){
    $reply = VM_booker_here;
    return $reply;
  }

  /*
   *
   */
  static function eventOutOfScope($e_code){
    static $scopes = array();
    if (!isset($scopes[$e_code])){
      $q = myPear_db()->query("SELECT lm_status,lm_status FROM zzz_list_members WHERE lm_value='$e_code' AND lm_key = 'conf'");
      while($r=myPear_db()->next_record($q))    $scopes[$e_code] = empty($r['lm_status']) || ($r['lm_status'] === 'no');
    }
    $reply = (bool)@$scopes[$e_code];
    return $reply;
  }

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
    return join(' ',array(" LEFT JOIN abs_events ON e_code = lm_value ",
			  parent::hookJOIN()));
  }
  
  private static $tabs = array();
  private static $cat = 'Categories';

  protected function getTabsForListing(){
    foreach($this->getMembers() as $id=>$r){
      if($r['lm_key']=='cat'){
	$tab = self::$cat;
      }elseif($e=myPear::getInstance('bForm_vm_Event',"e_code=".$r['lm_value'],'strict')){
	$tab = date('Y',$e->getValue('e_start'));
      }
      @self::$tabs[$tab]++;
    }
    ksort(self::$tabs);
    return array_keys(self::$tabs);
  }

  /*
   *
   */
  protected function getMembers_forListing($tab_number=Null){
    $keys = array_keys(self::$tabs);
    switch($year = $keys[$tab_number]){
    case self::$cat:
      return $this->getMembers(array('lm_key'=>'cat'));
      break;

    default:
      $this->sortArgs = " GROUP BY lm_value ORDER BY e_start ";
      $this->getMembers_where = array('e_start >= '.b_time::noon($year-1,12,31),
				      'e_start <= '.b_time::noon($year,  12,31),
				      );
      return $this->getMembers();
    }
  }
  
  /*
   *
   */
  protected function format($secondCall=False){
    if($this->r['lm_key']=='cat'){
      $this->r['lm_status'] = '';
      list($catName,$confs) = $this->getCategoryInfo($this->r['lm_value']); 
     $this->r['event'] = bJS()->modal_iframe(VM_agenda()->getCategoryURL($this->r['lm_value']),
					     x('strong',"Category \"$catName\""));
    }elseif($e=myPear::getInstance('bForm_vm_Event',"e_code=".$this->r['lm_value'],'strict')){
      $this->r['event'] = bJS()->modal_iframe(VM_agenda()->getEventURL($e->getValue('e_code')),$e->name(True,80));
      
      if (self::eventOutOfScope($this->r['lm_value'])) $this->r['lm_status'] = bIcons()->get('i-notok-small');
      else                                             $this->r['lm_status'] = bIcons()->get('i-ok');
    }else{
      if ($secondCall){
	$this->r['event'] = b_fmt::redText("Unknown event code '".$this->r['lm_value']."'");
      }else{
	// Auto-sync the newly accepted events
	VM_agenda()->sync(array("conf=".$this->r['lm_value']."&show_reg=all"));
	$this->format(True); 
      }
    }
    $this->r['lm_value'] = x('i',$this->r['lm_key'].' '. $this->r['lm_value']);
  }
}
