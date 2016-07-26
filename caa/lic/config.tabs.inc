<?php

function lic_getCategories(){
  global $regs;
  $tabs  = array('ALL');
  if (myPear_db()->tableExists('lic_soft')){
    $q=myPear_db()->query("SELECT * FROM lic_soft GROUP BY s_name ORDER BY s_name");
    while($r = myPear_db()->next_record($q)) {
      $regs = array('',$r['s_name'],'');
      if ($t = lic_tidySoft())  $tabs[] = $t;
    }
  }
  return array_unique($tabs);
}
