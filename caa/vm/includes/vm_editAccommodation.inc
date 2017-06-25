<?php
b_debug::xxx(__FILE__);
if (!empty($_REQUEST['id'])) myPear::WARNING_T("_GET[id] is not empty, but is ignored...");

locateAndInclude('bIcal_matrix_abs');

//
// Instantiate the Visit and (may be) the Lease
//
$lease_id = (int)$_REQUEST['lease_id'];

// See the action
if (strpos(@$_GET['block_once'],VM_visit_lease)===0) $action=btn_edit_accommodation;
else                                                 $action=@$_GET['action_once'];

$v = myPear::getInstance_new('bForm_vm_Visit',(int)$_REQUEST['v_id'],'strict');
$v->adjust_dates();

if (!(int)$v->ID){
  myPear::WARNING(bIcons()->get('32-noRight')."No visit to handle, please start again");
}else{
  
  //
  // edit & split accommodation
  //
  ob_start();
  if ($action){
    
    myPear::WARNING_T(bIcons()->get('32-zoo')." Doing $action");
    print myPear::H1($v->name(),'reset');
    
    switch($action){
    case txt_deleteVisit:
      $v->delete();	   
      break;
      
    case btn_deleteBooking:	
      $v->deleteBooking($lease_id);	
      $v->getForm();
      break;

    case btn_splitLease: 	 
      locateAndInclude('bForm_vm_Lease');
      $l = new bForm_vm_Lease($lease_id);
      $lease_id = $l->split();
      
    case btn_edit_accommodation: 
      $v->getFormOpen();
      foreach($v->get_allLeases(True) as $l){
	$l->isEmbedded($v);
	$l->MODE = ($l->ID == $lease_id ? 'RW' : 'RO');
	$l->not_isWritable_point = ($l->ID != $lease_id);
	$l->debug = True;
	$l->getForm();
      }
      $v->closeForm();
      break;
      
    default: 
      b_debug::internalError("unknown button '$action'");
    }
  }
  $editing_dialog = ob_get_contents();
  ob_end_clean();
  
  //
  // Show the acoommodation matrix
  VM_cotenants();

  $imposedEntries = $v->readCompleteDefRecord();
  foreach($v->get_allLeases(True) as $l){
    foreach(VM_cotenants()->get_cotenants($l->ID) as $lm_id=>$co_leases){
      foreach($co_leases as $co_lease_ID) $imposedEntries[] = bForm_vm_Lease::_leaseInfo($co_lease_ID);
    }
  }
  bIcal_matrix_abs::_show($v->getValue('v_start')-3*86400,
			  $v->getValue('v_end')  +3*86400,
			  $imposedEntries,
			  ($id=b_cnf::getValue('a_id') ? explode(',',$id) : array(99999999)));
  print '<hr/>' . $editing_dialog;
}

function bIcal_matrix_abs_spec($start,$end,$imposedEntries=array(),$a_ids=array(99999999)){
 locateAndInclude('bIcal_matrix_abs');
  $m = new bIcal_matrix_abs();
  $m->showLegend = False;
  $m->day1 = $start;
  $m->day9 = $end;
  $m->imposedEntries = $imposedEntries;
  $m->a_id = $a_ids;
  $m->show();
}
