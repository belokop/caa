<?php
locateAndInclude('APIaccess');
class APIaccess_ea extends APIaccess{

  public function __construct(){
    locateAndInclude('ea_updates.inc');
    parent::__construct();
  }
  
  function isManager(){
    return EA_ORG_manager_here;    
  }
  
  /*
   * Do not authorize strangers to scan the employees database
   */
  public function authorized(){
    if ($reply = parent::authorized()){
      $reply = ($this->getRank() >= RANK__ea_member);
    }
    return $reply;
  }

  /*
   * Get the EA rank
   */
  function getRank($av_id=Null){
    if (empty($av_id)) $av_id = @bAuth::$av->ID;

    static $reply = array();
    if (!isset($reply[$av_id])){
      $reply[$av_id] = parent::getRank($av_id);
      if ($reply[$av_id] == RANK__authenticated){
	locateAndInclude('bUnit');
	foreach(bUnit::get_unitsMembership(myOrg_ID,
					   array(RANK__ea_org_manager,
						 RANK__ea_admin,
						 RANK__ea_member),
					   $av_id) as $id=>$rr){
	  foreach($rr as $r)  $reply[$av_id] = max($reply[$av_id],$r['u_rank']);
	}
      }
    }
    return $reply[$av_id];
  }

  function mayUpdate($av_id=Null){
    return EA_administrator_here;
  }
}
