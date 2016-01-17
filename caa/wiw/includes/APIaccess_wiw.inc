<?php
locateAndInclude('APIaccess');

function WIW_access(){
  static $_ACCESS = Null;
  if ($_ACCESS === Null) $_ACCESS = new APIaccess_wiw();
  return $_ACCESS;
}

class APIaccess_wiw  extends  APIaccess{
  
  /*
   * Inherit the EA access list, add WIW administration
   */
  function __construct(){

    locateAndInclude('wiw_updates');
    locateAndInclude('APIaccess_ea');
    EA_access();

    $this->who_is_here(array('WIW_manager_here' => array('rank'=> 'RANK_wiw_manager',
							 'def' => 'EA_manager_here'),
			     'WIW_admin_here'   => array('rank'=> 'RANK_wiw_admin',
							 'def' => 'WIW_manager_here')));
  }
  
}
