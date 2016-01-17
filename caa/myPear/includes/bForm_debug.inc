<?php
/*
 * to have some have fun during the boring debugging...
 */
if (empty($_FILES)) $_FILES = array();
if (empty($_POST))  $_POST = array();
if (!function_exists('bForm_massage_POST')){

  function bForm_massage_POST(){

    static $ybstore = '/Users/yb/tmp/apps';
    
    $id = @$_POST['_post_bForm_ID_once'];
    if (!is_dir($ybstore) || empty($id)) return;
    
    // re-fill _POST with the debug data
    $_POST_pre_massaged = $_POST;
    foreach  (array_keys($_POST) as $k) $_REQUEST[$k] = $_POST[$k] = bForm_massage_POST_exec($k,$_POST[$k]);
    if ($_POST != $_POST_pre_massaged) b_debug::print_r($_POST,"_POST massaged");
    
    // Fill _FILES with the debug data
    if (!empty($_FILES) && (int)$id){
      static $files = array('ap_cv_file'     => 'f-cv.pdf',
			    'ap_er_file'     => 'f-jobRecord.pdf',
			    'ap_pl_file'     => 'f-publications.pdf',
			    'ap_rp_file'     => 'f-resPlan.pdf',
			    'rec_letter_file'=> 'f-recommendation.pdf',
			    'prp_local'      => 'f-preprint.pdf',
			    );
      
      myPear::getInstanceFromID($id);
      if (myPear::getInstanceFromID($id,'bForm_jam_Position')){
	$_FILES_pre_massaged = $_FILES;
	$q = myPear_db()->qquery("SELECT * FROM ".myPear::$_getInstanceFromID['t']." WHERE ".myPear::$_getInstanceFromID['i']." = $id",cnf_dev);
	while($r = myPear_db()->next_record($q)) $formDB = $r;
	foreach($_FILES as $name=>$data){
	  if (!empty($files[$name]) && ($data['error'] == 4)){
	    if (!empty($formDB[$name])) unset($_FILES[$name]);
	    else $_FILES[$name] = array('name'     => $files[$name],
					'type'     => 'application/force-download',
					'tmp_name' => "$ybstore/".$files[$name],
					'error'    => 0,
					'size'     => 8442);
	  }
	}
	if ($_FILES != $_FILES_pre_massaged) b_debug::print_r($_FILES,'_FILES massaged');
      }else{
	myPear::DEBUG("can't hook id=$id");
	b_debug::traceBack();
      }
    }
  }
  
  function bForm_massage_POST_exec($k,$v=Null){
    static $autofill_on = False;
    static $names     = array('Sam','John','Hans','Sven','Serg','Suleiman', 'Ostap', 'Bert');
    static $namesp    = array('ovich','son','sson','off','ish ab Hassan','er');
    static $countries = array('IL','NO','RU','ES','FI','VG','NA','IS','IR');
    static $insts     = array('Santa Barbara','Brighten beach','Star-bucks college','Urbana Champaign',
			      'МФТИ','Каретный','Sree Sevugan', 'Annamalai college');
    
    $v_was = $v;
    if ($v === $k)                    $autofill_on = True; 
    if ($v === 'example@example.com') $autofill_on = True; 
    if ($autofill_on){
      $v = trim($v);
      $k = trim($k);
      if ($v === 'NULL') $v = Null;
      if ($v === $k){
	switch($v){
	case 'v_host_avid':          $v = 'hvzm'; break;
	case 'v_total_expenses':     $v = (int)$_POST['v_travel_expenses'] + (int)$_POST['v_living_expenses']; break;
	case 'r_home':               $v = sprintf("Institute Box%03d",rand(1,999)); break;
	default:
	  if     (stripos($k,'firstname')!==False)     $v = $names[rand(0,count($names)-1)];
	  elseif (stripos($k,'lastname') !==False)     $v = $names[rand(0,count($names)-1)].$namesp[rand(0,count($namesp)-1)];
	  elseif (stripos($k,'instit')   !==False)     $v = $insts[rand(0,count($insts)-1)];
	  elseif (stripos($k,'place')    !==False)     $v = $insts[rand(0,count($insts)-1)];
	  elseif (stripos($k,'position') !==False)     $v = 'the best position';
	  elseif (stripos($k,'country')  !==False)     $v = $countries[rand(0,count($countries)-1)];
	  elseif (stripos($k,'ship')     !==False)     $v = $countries[rand(0,count($countries)-1)];
	  elseif (stripos($k,'salutatio')!==False)     $v = 'Prof.';
	  elseif (stripos($k,'_expenses')!==False)     $v = "" . rand(1000,10000);
	  elseif (stripos($k,'_phone')   !==False)     $v = "" . rand(1000,9999);
	  elseif (stripos($k,'_start')   !==False)     $v = b_time::noon() - rand(1,2)*86400;
	  elseif (stripos($k,'_end')     !==False)     $v = b_time::noon() + rand(10,20)*86400;
	  elseif (stripos($k,'_comment') !==False)     $v = 'Mauris mauris ante, blandit et, ultrices a... ';
	  if ($v == 'v_date_start')                    $v = date('Y-m-d',b_time::noon());
	}
	$v = preg_replace("/.*_(lecture|fax|http|country|institute|office|place|comment)_.*/",'\\1',$v);
      }elseif($v=='example@example.com'){
	$v = sprintf("%s%06d@physto.se",preg_replace("/^([a-z]*)_.*$/",'\\1',$k), rand(1,99999)); 
      }elseif(empty($v) && preg_match('/_year/',$k)){
	$v = (string)(date('Y',time())-rand(1,6));
      }
    }
    if ($v_was !== $v) b_debug::xxx($v);
    return $v;
  }

  bForm_massage_POST();

}
