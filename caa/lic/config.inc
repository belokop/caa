<?php

$releaseDate = '2020-03-19';
define('LIC_VERSION','5.16');
define('LIC_MODULE'  , 'lic');

$GLOBALS['comsol_names'] = array('comsol','acdc','serial', 'waveoptics', 'clientserver',
				 );

b_cnf::variable_set('LIC_mathlmPort',16286);

b_reg::add_module(LIC_MODULE,
		  array('v' =>LIC_VERSION,
			'tm'=>'Licenses',
			'd' =>'Licenses Monitor',
			'r' =>$releaseDate,
			'c' =>__FILE__,
			'i' =>'i-invoice',
			
			'classes'=>array('lEvent' => array('t' =>'lic_event', 
							   'i' =>'lice_id'),
					 'lHost'  => array('t' =>'lic_host', 
							   'i' =>'lich_id'),
					 'lServer'=> array('t' =>'lic_server',
							   'i' =>'licsrv_id'),
					 'lSoft'  => array('t' =>'lic_soft', 
							   'i' =>'lics_id'),
					 'lUser'  => array('t' =>'lic_user',
							   'i' =>'licu_id')))
		  );

/*
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
*/

function lic_used_by_myOrg(){
  return array('nordita','fysikum');
}

function lic_tidySoft() {
  global $regs;
  if (@$regs[1] == 'FCompL' || 
      preg_match('/ccompl|fortran/',    @$regs[1]))  list($regs[1],$regs[2]) = array('fortran','intel');
  if (preg_match('/(maple)(\d+)/i',     @$regs[1],$rr))   $regs = $rr;
  if (preg_match('/^(mechanisms|motion_|id_|iges|step_)|assemblies|gateway|_modeling|draft|nx_/i',@$regs[1])) $regs[1] = 'ideas';
  if (preg_match('/ug_pcb_/i',                                                                    @$regs[1])) $regs[1] = 'ideas';
  if (preg_match('/^('.join('|',$GLOBALS['comsol_names']).')/i',@$regs[1]))                                   $regs[1] = 'comsol';
  if (preg_match('/0000000000/',        @$regs[1]))       $regs = array();
  if (preg_match('/^(i000|i233|ibfd5)/',@$regs[1]))       $regs = array();
  return  @$regs[1];
}

function lic_tidyHost($name) {
  return $name;
}

function lic_tidySrv($string,$port=False) {
  if ($port == b_cnf::get('LIC_mathlmPort')) {
    if (preg_match('/syslx53/',  $string))           return 'mathlm.nordita.org';
    if (preg_match('/(syslx52|fysik.su.se)/',$string)) return 'mathlm.fysik.su.se';
  }
  $a = preg_split('/,/',
		  preg_replace('/\b\d*\@/','',$string));
  sort($a);
  return join(',',$a);
}

class LIC extends myPear{
  static public function _MENU()  { 
    return loader::_fromCache('APImenu_lic'); 
  }

}
