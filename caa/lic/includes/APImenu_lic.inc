<?php
locateAndInclude('APImenu');

class APImenu_lic extends APImenu{

  var $q      = LIC_MODULE;

  function __construct(){
    locateAndInclude('lic_updates.inc');
    parent::__construct();
    $this->static_menu = array('ALL'=>array());
    $this->tabWeight['ALL'] = -999;
    foreach (lic_getCategories() as $tab)       $this->static_menu[$tab] = array();
  }

  public function access($arg1=Null,$arg2=Null){
    switch($arg1){
    case 'bred':  $reply = False;              break;
    case 'ideas': $reply = superUser_here; break;
    default:      $reply = bAuth::authenticated();
    }
    if (b_reg::$current_module !== LIC_MODULE)  $reply = False;
    b_debug::xxx($reply,array('A'));
    return $reply;
  }
}

function _lic_title_callback($text,$arg=Null){
  // do not translate software name 
  switch($text){
  case 'title':  return bText::_(array('key' =>"LIC_MENU_1",   'item'=>'FLEXlm statistics'));
  case 'ALL':    return bText::_(array('key' =>"LIC_MENU_ALL", 'item'=>'ALL'));
  default:       return $text;
  }
}
