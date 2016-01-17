<?php

myPear_startup_queue('lic_updates');

function lic_updates(){
  if (bMaster()->register_module_update_script(LIC_MODULE,LIC_VERSION,__FILE__)){
    lic_updates_0();
    lic_updates_1();
  }
}

/*
 * Fix naming convension
 */
function lic_updates_0(){
  foreach(array('lic_server' => 'srv_id',
		'lic_event'  => 'e_id',
		'lic_host'   => 'h_id',
		'lic_soft'   => 's_id',
		'lic_user'   => 'u_id',
		  ) as $t=>$c){
    if (myPear_db()->columnExists($c,$t)){
      myPear_db()->debug = 1;
      myPear_db()->query("ALTER TABLE `$t` CHANGE `$c` `lic$c` INT( 128 ) NOT NULL AUTO_INCREMENT"); 
      myPear_db()->debug = 0;
      myPear_db()->reset_cache();
    }
  }
}

/*
 * Comsol changes the module name(s)
 */
function lic_updates_1(){
  $q = myPear_db()->query("SELECT lics_id from lic_soft WHERE s_name = 'comsol'");
  while($r = myPear_db()->next_record($q)){
    $lics_id_comsol = $r['lics_id'];
  }

  foreach(array('comsol','acdc','serial') as $s){
    unset($lics_id);
    $q = myPear_db()->qquery("SELECT lics_id,s_name from lic_soft WHERE s_name REGEXP '$s' AND s_name != 'comsol'",True);
    while($r = myPear_db()->next_record($q)){
      $lics_id = $r['lics_id'];
      $s_name  = $r['s_name'];
      if (empty($lics_id)) continue;

      $cmd = "DELETE FROM lic_soft WHERE lics_id = $lics_id";
      $x = myPear_db()->qquery($cmd,True);

      $cmd = "UPDATE lic_event SET e_sid = $lics_id_comsol WHERE e_sid = $lics_id";
      $x = myPear_db()->qquery($cmd,True);
    }
  }
}
