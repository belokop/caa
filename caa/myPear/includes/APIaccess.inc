<?php
class APIaccess{
  
  public static $cache  = Null;
  public     $roles      = array();
  protected  $RANK_privs = array();
  var        $debug  = False;

  public function __construct(){
    //    locateAndInclude('b_cache');
    //    APIaccess::$cache = new b_cache_S(get_class($this));
    $this->who_is_here(array('EA_manager_here' => array('rank'=> 'RANK__ea_manager',
                                                        'def' => 'superUser_here'),
                             'EA_administrator_here'   => array('rank'=> 'RANK__ea_admin',
                                                        'def' => 'EA_manager_here'),
			     'EA_member_here'  => array('IF'  => myOrg()->eaMembers()->isMember()),
			     ));
  }
  
  /*
   * Define "who is here"
   */  
  function who_is_here($WIH){
    foreach($WIH as $_here => $data){
      if (!defined($_here)){
	if (is_object(bAuth::$av)){
	  if (!empty($data['rank']) && !defined($data['rank'])) b_debug::internalError("$data[rank] is not defined yet");
	  $rank = @constant($data['rank']);
	  if (!defined($_here)){
	    if   (!empty($data['IF']))  define($_here,($this->getRank() == $rank) || (bool)$data['IF']);
	    elseif(empty($data['def'])) define($_here,($this->getRank() == $rank));
	    else                        define($_here,($this->getRank() == $rank) || @constant($data['def']));
	  }
	}else{
	  define($_here,False);
	}
	if (cnf_show_acl) myPear::DEBUG(get_class($this).": $_here = ".(constant($_here) ? 'YES' : 'NO'),'force');
      }
    }
  }
  
  public function isManager(){
    return (defined('superUser_here')
	    ? superUser_here
	    : True);
  }

  /*
   * Module dependent authorization
   */
  public function authorized(){
    $reply = bAuth::authenticated();
    return $reply;
  }

  /*
   * Get the title of a highest rank
   */
  public function getTitle(){
    locateAndInclude('b_registry');
    $reply = b_reg_bUnit::get_title(myPear_access()->getRank());
    return $reply;
  }

  /*
   * Get the Avatar rank
   */
  public function getRank(){
    if (!myPear::$isReady){
      myPear_startup_queue(__function__);
    }else{
      if (superUser_here){
	$reply = RANK__superuser;
      }else{    
	$reply = (bAuth::authenticated()
		  ? RANK__authenticated
		  : RANK__anonymous);
      }
      return $reply;
    }
  }

  public function isRank($rank){
    return $this->getRank() >= $rank;
  }
  
  public function show(){
    locateAndInclude('bUnit');
    myPear::WARNING_T("TBD");
  }

  public function dbg($id=Null,$YesNo='not set'){
    if (func_num_args() == 1) b_debug::xxx($id,array(3,'AA',$this));
    else                      b_debug::xxx(($YesNo==='not set'?'&nbsp;':(empty($YesNo) ? 'NO' : $YesNo)),array(3,'AA',$id));
  }
}
