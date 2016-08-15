<?php
if (function_exists('locateAndInclude')) locateAndInclude('APImenu');

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

class APImenu_lic extends APImenu{
  
  var $q = LIC_MODULE;

  function __construct(){
    locateAndInclude('lic_updates.inc');
    parent::__construct();
    $this->static_menu = array('ALL'=>array());
    foreach (lic_getCategories() as $tab)       $this->static_menu[$tab] = array();
    $this->tabWeight['ALL'] = -999;
    APItabs::define_tab('tab_lic','Licences usage');
    APItabs::define_tab('ALL',    'ALL');
    foreach(lic_getCategories() as $lic) APItabs::define_tab($lic,$lic);
  }

  public function access($arg1=Null,$arg2=Null){
    switch($arg1){
    case 'bred':  $reply = False;          break;
    case 'ideas': $reply = superUser_here; break;
    default:      $reply = bAuth::authenticated();
      $reply = True;
    }
    if (b_reg::$current_module !== LIC_MODULE)  $reply = False;
    b_debug::xxx($reply,array('A'));
    return $reply;
  }
}

function _lic_title_callback($text,$arg=Null,$returnText=False){
  return $text; 
 // do not translate software name 
  switch($text){
  case 'title':  return bText::_(array('key' =>"LIC_MENU_1",   'item'=>'FLEXlm usage'));
  case 'ALL':    return bText::_(array('key' =>"LIC_MENU_ALL", 'item'=>'ALL'));
  default:       return $text;
  }
}
