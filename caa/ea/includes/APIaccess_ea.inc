<?php
locateAndInclude('APIaccess');
class APIaccess_ea extends APIaccess{

  public function __construct(){
    locateAndInclude('ea_updates.inc');
    parent::__construct();
  }
  
  function isManager(){
    return EA_manager_here;    
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
  function getRank(){
    static $reply = Null;
    if ($reply === Null){
      $reply = parent::getRank();
      if ($reply == RANK__authenticated){
	locateAndInclude('bUnit');
	foreach(bUnit::get_unitsMembership(myOrg_ID,
					   array(RANK__ea_manager,
						 RANK__ea_admin,
						 RANK__ea_member),
					   bAuth::$av->ID) as $av_id=>$rr){
	  foreach($rr as $r)  $reply = max($reply,$r['u_rank']);
	}
      }
    }
    return $reply;
  }

  function mayUpdate($av_id=Null){
    return EA_administrator_here;
  }
}
