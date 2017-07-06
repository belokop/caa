<?php
locateAndInclude('APIaccess_ea');

function WIW_access(){
  static $_ACCESS = Null;
  if ($_ACCESS === Null){
    if (function_exists('wiw_init')) wiw_init();
    $_ACCESS = new APIaccess_wiw();
  }
  return $_ACCESS;
}

class APIaccess_wiw  extends  APIaccess_ea{
  
  /*
   * Inherit the EA access list, add WIW administration
   */
  function __construct(){

    locateAndInclude('wiw_updates');
    locateAndInclude('APIaccess_ea');
    EA_access();

    $this->who_is_here(array('WIW_manager_here' => array('rank'=> 'RANK_wiw_manager',
							 'def' => 'EA_ORG_manager_here'),
			     'WIW_admin_here'   => array('rank'=> 'RANK_wiw_admin',
							 'def' => 'WIW_manager_here')));
  }
  
  /*
   *
   */
  function getRank($av_id=Null){
    if (empty($av_id)) $av_id = @bAuth::$av->ID;
    if (($reply = parent::getRank($av_id)) >= RANK__authenticated){
      if (WIW_managers()->isMember($av_id))    $reply = RANK_wiw_manager;
    }
    $this->dbg($reply,cnf_dev);
    return (int)@$reply;
  }
}
