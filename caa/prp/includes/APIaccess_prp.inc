<?php
locateAndInclude('APIaccess_ea');
class APIaccess_prp extends APIaccess_ea{

  public function __construct(){
    parent::__construct();
    $this->who_is_here(array('PRP_manager_here'  => array('rank'=> 'RANK_prp_manager',
							  'def' => 'EA_manager_here'),
			     'PRP_censor_here'   => array('rank'=> 'RANK_prp_censor',
							  'def' => 'PRP_manager_here'),
			     'PRP_submitter_here'=> array('rank'=> 'RANK_prp_submitter',
							  'def' => 'PRP_censor_here'),
			     ));
  }
  
  /*
   *
   */
  public function isManager(){
    return PRP_manager_here;
  }
  
  /*
   *
   */
  public function authorized(){
    if ($reply = parent::authorized()){
      $reply = PRP_submitter_here;
    }
    return $reply;
  }
  
  /*
   *
   */
  function getRank($av_id=Null){
    if (empty($av_id)) $av_id = @bAuth::$av->ID;
    if (($reply = parent::getRank($av_id)) == RANK__authenticated){
      if (PRP_submitters()->isMember($av_id)) $reply = RANK_prp_submitter;
      if (PRP_censors()->isMember($av_id)) $reply = RANK_prp_censor;
    }
    $this->dbg($reply);
    return (int)@$reply;
  }

  function mayUpdate($av_id=Null){
    return PRP_censor_here;
  }


}
