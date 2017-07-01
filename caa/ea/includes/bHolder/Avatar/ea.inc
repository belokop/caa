<?php
b_reg::load_module(EA_MODULE);
locateAndInclude('bHolder_Avatar');
class bHolder_Avatar_ea extends bHolder_Avatar{

  public $av_ids;

  /*
   * Constructor
   */
  function __construct(){
    parent::__construct();
    b_debug::xxx(' ');
  }
  
  /*
   *
   */
  protected function avatar_anker($av_id){
    $reply = sprintf("?function=ea_myPage&clean_once=>act_code,act_id&av_id=%s&q=%s",$av_id,EA_MODULE);
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * Query for the EA avatars, has an extra option to ask for the list of AV_IDs
   *
   * @param $only_my_avatars - if True, select only EA-related avatars, otherwise select all
   */
  function query($only_my_avatars=False) {
    $where = $this->where;
    if(!empty($this->av_ids)){
      foreach($this->av_ids as $av_id) $w[] = "av_id = '$av_id'";
      $where[] = '('.implode(' OR ',$w).')';
    }elseif($only_my_avatars){
      if (empty(b_reg_bUnit::$_bUnits[EA_MODULE])) b_reg_bUnit::$_bUnits[EA_MODULE] = array();
      foreach(array_keys(b_reg_bUnit::$_bUnits[EA_MODULE]) as $role) $w[] = "u_rank = $role"; 
      if (!empty($w)) $where[] = ' ( ' . implode(' OR ',$w) . ' ) ';
    }
    
    b_debug::xxx(sprintf('where=%s',join(',',$where)));
    $q = myPear_db()->qquery("SELECT av_photo,av_id,u_rank,av_institute,av_firstname,av_lastname,av_lastlogin,av_email,av_lastname AS name,av_email AS email ".
			     " FROM zzz_avatars ".
			     " LEFT JOIN zzz_unit_members    ON um_avid = av_id".
			     " LEFT JOIN zzz_units           ON um_uid = u_id ".
			     " WHERE ".implode(' AND ',$where).
			     " GROUP BY av_id ".
			     " ORDER BY av_lastname",cnf_dev);
    //    " ORDER BY $_GET[sBy] ".@$_GET['sByDir']);
    return $q;
  }
}
