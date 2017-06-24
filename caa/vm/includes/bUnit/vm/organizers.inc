<?php
/*
 * Unit of event organizers.
 * - the unit parent is an event,
 * - the unit rank is 'RANK_vm_organizer'
 */
locateAndInclude('bUnit_vm');
class bUnit_vm_organizers extends bUnit_vm{

  var $e = Null; 

  /*
   *
   */
  function  __construct($e_id,$rank=RANK_vm_organizer){
    parent::__construct($e_id,RANK_vm_organizer);
    $this->e = myPear::getInstance_new('bForm_vm_Event',$e_id,'fatal');
  }
  
  /*
   * This unit is not supposed to be edited manually,
   * it is created from the Indico records
   */
  function isWritable(){
    return False;
  } 

  function name($arg=Null){
    return (is_object($this->e)
	    ? '"'.$this->e->name(True,40).'" organizers'
	    : '');
  }

  function getForm(){
    myPear::H1($this->getValue('l_name'),'reset');
    print @$this->subtitle;

    $q = myPear_db()->qquery(sprintf("SELECT e_id,e_name,u_id,u_parent,u_rank,um_avid FROM %s ".
				     " LEFT JOIN %s ON um_uid = u_id ".
				     " LEFT JOIN abs_events ON e_id = u_parent ".
				     " WHERE u_parent = %s AND u_rank = ".RANK_vm_organizer.
				     " ORDER BY e_end DESC",$this->TABLE_member,$this->TABLE,$this->parent_ID),
			     cnf_dev);
    
    myPear::H1($this->getValue('l_name'));

    while ($r = myPear_db()->next_record($q)) {
      if ($r['e_id'] != @$e_id){
	if (is_object(@$t)){ 
	  $t->close(); 
	}
	$t = new b_table_zebra(array('name'=>'','inst'=>''));
	$t->noSort = True;
	$t->showLineCounter = False;
      }
      $e_id = $r['e_id'];
      if ($av = myPear::getInstance_new('bForm_Avatar_vm',$r['um_avid'],array('fatal','strict'))){
	$av->defineVariables();
	$r['inst']  = x('i',b_fmt::strcut($av->getValue('av_institute',1),25));
	$r['name']  = $av->fmtName('fl');
	$r['name']  = bJS()->popup($r['name'],"?id=$av->ID&form=Avatar");
	if (VM_administrator_here){
	  $t->extraTD[] = bIcons()->getButton(array('d'=>'Send the password',
						    'i'=>'i-mailBlue',
						    'l'=>b_url::same("?av_id=$r[um_avid]&e_id=$e_id&mo_once=yes")));
	}
	$t->prt($r);
      }
    }
    if (!empty($t)){
      $t->close(); 
    }
  }

  /*
   *
   *
   */
  function getOrganizers(){
    $reply = array();
    foreach($this->getMembers() as $lm_id=>$r) $reply[] = $r['av_id'];
    return $reply;
  }

  protected function hookJOIN(){
    return join(' ',array(parent::hookJOIN(),
			  ' LEFT JOIN abs_events ON e_id = u_parent '));
  }
  
  /*
   * Get the list of events managed by the organizer
   * 
   * @param $av_id  - organizer ID
   * @param $events - output array (must exist)
   * $param $return_eCode - if set, the output array index will be 'e_code', 
   *                        otherwise 'e_id'
   *        Note, the old events are dropped if 'return_eCode' == True 
   */
  public static function getEvents($av_id=0, $events=array(), $return_eCode=False){
    static $my_events = array();
    $id = ($return_eCode ? 'e_code' : 'e_id');
    if (!isset($my_events[$av_id][$id])){
      $where = array("u_rank = '".RANK_vm_organizer."'");
      if ($av_id) $where[] = "um_avid = '$av_id'";
      $my_events[$av_id]['e_code'] = $my_events[$av_id]['e_id'] = array();
      $q = myPear_db()->query("SELECT e_id,e_code,e_name,e_start,e_end FROM zzz_unit_members ".
			      " LEFT JOIN zzz_units ON um_uid = u_id ".
			      " LEFT JOIN abs_events ON e_id = u_parent ".
			      " WHERE  ".join(' AND ',$where).
			      " ORDER BY e_start DESC");
      while ($r = myPear_db()->next_record($q)) {
	$name = b_time::strftime('%h %Y',$r['e_start']).' - '.b_fmt::strcut($r['e_name'],43,'...');
	$my_events[$av_id]['e_id'][$r['e_id']] = $name;
	if ($r['e_end'] > time()-AUTO_EXPIRATION*86400)  $my_events[$av_id]['e_code'][$r['e_code']] = $name;
      }
    }
    $reply = $events;
    foreach($my_events[$av_id][$id] as $k=>$v) $reply[$k] = $v;
    return $reply;
  }
  
  /*
   * Similar to getEvents (see above) with different invocation scheme
   */
  public static $e_index = 0;
  public static function get_all_events($av_id=0){
    $events = array();
    $where[] = "u_rank = '".RANK_vm_organizer."'";
    if (!empty($av_id)) $where[] = "um_avid = '$av_id'";

    $q = myPear_db()->query("SELECT e_id,e_start FROM zzz_units ".
			    " LEFT JOIN zzz_unit_members ON um_uid = u_id ".
			    " LEFT JOIN abs_events ON e_id = u_parent ".
			    " WHERE ".join(' AND ',$where).
			    " GROUP BY u_id ORDER BY e_start DESC");
    
    $closest_ev = 99999999999;
    while ($r = myPear_db()->next_record($q)) {
      if ($e_id = ((int)$r['e_id'])){
	$events[] = $e_id;
	$d = b_time::diff(time(),$r['e_start']);
	if (abs($d) < $closest_ev) list($closest_ev,self::$e_index) = array(abs($d), count($events)-1);
      }
    }
    return $events;
  }
}
