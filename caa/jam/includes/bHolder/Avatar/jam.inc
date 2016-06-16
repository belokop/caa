<?php
b_reg::load_module(JAM_MODULE);
locateAndInclude('bHolder_Avatar');
class bHolder_Avatar_jam extends bHolder_Avatar{

  var $debug = False;
  protected $left_join = array(" LEFT JOIN jam_recommendations ON rec_avid = av_id",
			       " LEFT JOIN jam_applicants      ON ap_avid  = av_id");
  
  /*
   *
   */
  function query($only_my_avatars=True){
    if ($only_my_avatars){
      $w = array("ap_id  > 0",
		 "rec_id > 0");
      foreach(array_keys(b_reg_bUnit::$_bUnits[JAM_MODULE]) as $role){
	$w[] = "u_rank = $role";
      }
      $this->where[] = ' ( ' . implode(' OR ',$w) . ' ) ';
    }
    return parent::query($only_my_avatars);
  }

  /*
   *
   */
  function format() {
    locateAndInclude('bForm_jam_Recommendation');    
    parent::format();

    /*
    $this->dbg("show_icons=".var_export($this->show_icons,True),cnf_dev);
    if (bAuth::authenticated()){
      bForm_Avatar::set_context(bAuth::$av);
      $isApplicant = bAuth::$av->isApplicant($pos_id=False,$onlyActive=True);
      b_debug::var_dump($isApplicant);
      $this->dbg("isApplicant=".var_export(($x=!empty($isApplicant)),True),cnf_dev);
      if (!empty($isApplicant)) $this->show_icons = False;
    }
    $this->dbg("show_icons=".var_export($this->show_icons,True),cnf_dev);
    */
    
    // Applicants & Referees do not "fire" JAM_member_here    
    $ranks = array();
    if ($this->show_icons && JAM_member_here){    
      foreach(bUnit::get_unitsMembership(array_merge(array(myOrg_ID),
						     array_keys(bForm_jam_Position::getPositions(myOrg_ID))),
					 array(RANK_jam_manager,
					       RANK_jam_posadmin,
					       RANK_jam_secr,     
					       RANK_jam_SC1,
					       RANK_jam_SC2,      
					       RANK_jam_observer,
					       ),
					 $this->rec['av_id']) as $av_id=>$rrs){
	foreach($rrs as $k=>$r) $ranks[] = $r['u_rank'];
      }
     
      // Add referees & applicants
      foreach(array(RANK_jam_referee   => bForm_jam_Recommendation::getRecommendations('count', $this->rec['av_id']),
		    RANK_jam_applicant => count(bForm_jam_Applicant::get_apInfo($this->rec['av_id'])) > 0,
		    ) as $rank=>$condition){
	if ($condition > 0) $ranks[] = $rank;
      }
      
      // Loop thru the roles
      foreach(array_unique($ranks) as $rank){
	$url = Null;
	$icon = b_reg_bUnit::get_icon($rank,JAM_MODULE);
	switch($rank){    
	case RANK_jam_applicant:  
	  $url = b_url::same('?av_id='.$this->rec['av_id'].'&holder=jam_Applicant');  
	  break;
	case RANK_jam_referee:  
	  $url = b_url::same('?id='.$this->rec['av_id'].'&function=jam_listRecommendations'); 
	  break;
	case RANK_jam_posadmin: 
	case RANK_jam_secr:     
	case RANK_jam_SC1:      
	case RANK_jam_SC2:      
	case RANK_jam_observer: 
	default:	           
	  $icon = Null;
	}
	if (!empty($icon)){
	  $this->t->extraTD = array_diff($this->t->extraTD,array($icon));
	  if ($url) $this->t->extraTD[] = b_btn::link($icon,$url);
	  else      $this->t->extraTD[] = $icon;
	}
      }
    }
  }
}
