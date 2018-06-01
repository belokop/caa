<?php

// Workaround
// vm_compat_update_32();

/*
 * Query VM database
 */
function VM_query($where=array(1),$join_rooms=(' LEFT JOIN abs_leases ON v_id = lease_vid '
					       .'LEFT JOIN abs_rooms ON a_id = lease_aid '
					       .'LEFT JOIN abs_huts ON hut_id = a_hutid '
					       // .'LEFT JOIN abs_expenses ON v_id = exp_vid '
					       // .'LEFT JOIN abs_offices  ON v_oid = o_id'
					       )){
  b_debug::_debug();
  $query = "SELECT av_id,av_lastname,av_firstname,av_city,av_institute,av_email,av_residentship "
    .",v_id,v_type,v_policy,v_host_avid,v_start,v_end,v_avid,v_status,v_acc_wish,v_eid,v_projectid,v_accompaning,v_gsf "
    .",e_id,e_policy,e_name,e_start,e_end,e_v_policy "
    .(strpos($join_rooms,'abs_rooms')  === False ? "" : ",a_id,a_name,a_capacity ")
    .(strpos($join_rooms,'abs_huts')   === False ? "" : ",hut_id,hut_name,hut_name,hut_code")
    .(strpos($join_rooms,'abs_leases') === False ? "" : ",lease_id,lease_start,lease_end,lease_option,lease_aid ")
    .(strpos($join_rooms,'abs_offices')=== False ? "" : ",o_id,o_name,o_capacity,o_phone ")
    .(strpos($join_rooms,'abs_expenses')===False ? "" : ",exp_closed,exp_currency,exp_id,exp_living,exp_other,exp_owner_avid,exp_perdiem,exp_travel,exp_vid")
    ." FROM abs_visits "
    ." LEFT JOIN abs_events  ON v_eid = e_id "
    ." LEFT JOIN zzz_avatars ON v_avid = av_id "
    ." $join_rooms ";

  // extract sql tables used
  $tables = array('abs_visits','abs_events','zzz_avatars');
  foreach(preg_split('/LEFT *JOIN| FROM /i',preg_replace('/.* FROM /i','',$query)) as $join){
    $table = trim(preg_replace('/ .*/','',trim($join)));
    if (!empty($table)) $tables[] = $table;
  }

  // check validity of ORDER BY  
  if (@$_GET['sBy'] === 'sBy_lists'){
    $_GET['sBy'] = "v_status ASC, hut_code DESC, hut_name ASC, a_capacity ASC, av_lastname ASC";
    $_GET['sByDir'] = " ";
  }else{      
    $was = $orderBy = explode(',',str_ireplace(array('concat','(',')'),'',@$_GET['sBy']));
    foreach($orderBy as $f){
      $f = trim($f); 
      $ok = False;
      foreach($tables as $t){
	if (myPear_db()->columnExists($f,$t)) { $ok = True; break; }
      }
      if (!$ok){
	b_debug::_debug("cancel sBy=$f");
	unset($orderBy[$f]);
      }
    }
    // remove "concat", it does crazy things...
    $_GET['sBy'] = join(',',$orderBy);
  }    

  // do the query
  $query = $query." WHERE ".preg_replace("/=\s?is null/i",' IS NULL',join(' AND ',$where)).(($sBy=trim($_GET['sBy'])) ? " ORDER BY $sBy ".@$_GET['sByDir'] : "");;
  $q = myPear_db()->qquery($query,cnf_dev);
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
    MSG::WARNING('Ignoring request to cancel the visit');
  }else{
    $v = loader::getInstance_new('bForm_vm_Visit',$v_id,'fatal');
    $v->delete();
  }
}

/*
 *
 */
function vm_edit_accommodation($v_id=Null){
  if (empty($v_id)) $v_id = @$_GET['v_id'];
  if (empty($v_id)){
    MSG::WARNING('Ignoring request to edit the accommodation');
  }else{
    require_once dirname(__FILE__).'/vm_editAccommodation.inc';
  }
}
