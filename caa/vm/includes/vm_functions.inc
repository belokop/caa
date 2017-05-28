<?php

// Workaround
// vm_compat_update_32();

/*
 * Query VM database
 */
function VM_query($where,$join_rooms='',$debug=False){
  if (empty($where)) $where = array(1);
  
  if (empty($join_rooms))  $join_rooms = ''
			     .'LEFT JOIN abs_leases ON lease_vid = v_id '
			     .'LEFT JOIN abs_rooms ON a_id = lease_aid '
			     .'LEFT JOIN abs_huts ON a_hutid = hut_id '
			     .'LEFT JOIN abs_expenses ON v_id = exp_vid '
			     // .'LEFT JOIN abs_offices  ON v_oid = o_id'
			     ;
  $query = "SELECT *,av_lastname AS name FROM abs_visits "
    ." LEFT JOIN abs_events  ON v_eid = e_id "
    ." LEFT JOIN zzz_avatars ON v_avid = av_id "
    ." $join_rooms ";
  
  foreach(explode(',',str_ireplace(array('concat','(',')'),'',@$_GET['sBy'])) as $f){
    $ok = False;
    foreach(array('abs_leases',
		  'abs_expenses',
		  'abs_rooms',
		  'abs_visits',
		  'abs_events',
		  'abs_huts',
		  'zzz_avatars') as $t){
      if (myPear_db()->columnExists($f,$t) && (strpos($query,$t) !== False)) $ok = True;
    }
    if (!$ok){
      if(cnf_dev) myPear::DEBUG(__FUNCTION__."() - cancel sBy=$f");
      $_GET['sBy'] = 'av_lastname';
    }
  }
  $q = myPear_db()->qquery($sql = $query." WHERE ".preg_replace("/=\s?is null/i",' IS NULL',join(' AND ',$where))
			   ." ORDER BY $_GET[sBy] ".@$_GET['sByDir'],
			   $debug||cnf_dev);
  $GLOBALS['VM_query_num_rows'] = myPear_db()->num_rows($q);
  return $q; 
}

function VM_query_num_rows(){
  return @$GLOBALS['VM_query_num_rows'];
}

/*
 * Agenda class wrapper
 */
function VM_agenda(){
  static $instance = Null;
  if (empty($instance)){
    locateAndInclude('agenda_vm');
    start_VM();
    $instance = new agenda_vm();
  }
  return $instance;
}

/*
 *
 */
function vm_colorLegend($r){
  
  if ($e=@$r['e_name']){
    $colorLegend = $e;
  }elseif(show_guests_by_budget){
    if ($c=@$r['v_projectid']){
      locateAndInclude('bForm_vm_Visit');
      $colorLegend = bForm_vm_Visit::projectName($c,$compact=True);
    }else{
      $colorLegend = 'project X';
    }
    if (preg_match('/^[0-9]*$/',$colorLegend)) $colorLegend = "project $colorLegend";
  }else{
    $colorLegend = @$r['v_group'];
    if (empty($colorLegend)) $colorLegend = 'group X';
  }
  return $colorLegend;
}

/*
 * Canceling the visit
 */
function vm_cancel_visit($v_id=Null){
  if (empty($v_id)) $v_id = @$_GET['v_id'];
  if (empty($v_id)){
    myPear::WARNING('Ignoring request to cancel the visit');
  }else{
    $v = myPear::getInstance_new('bForm_vm_Visit',$v_id,'fatal');
    $v->delete();
  }
}

/*
 *
 */
function vm_edit_accommodation($v_id=Null){
  if (empty($v_id)) $v_id = @$_GET['v_id'];
  if (empty($v_id)){
    myPear::WARNING('Ignoring request to edit the accommodation');
  }else{
    require_once dirname(__FILE__).'/vm_editAccommodation.inc';
  }
}
