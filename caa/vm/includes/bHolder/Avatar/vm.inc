<?php
b_reg::load_module(VM_MODULE);
locateAndInclude('bHolder_Avatar');
class bHolder_Avatar_vm extends bHolder_Avatar{
  
  protected $left_join = array('LEFT JOIN abs_visits ON v_avid = av_id');
  
  /*
   *
   */
  function query($only_my_avatars=True){
    if ($only_my_avatars && empty($this->av)){
      $w = array("v_id > 0");
      if (!empty(b_reg_bUnit::$_bUnits[VM_MODULE])){
	foreach(array_keys(b_reg_bUnit::$_bUnits[VM_MODULE]) as $rank) { $w[] = "u_rank = $rank"; }
      }
      $this->where[] = ' ( ' . implode(' OR ',$w) . ' ) ';
    }
    return parent::query($only_my_avatars);
  }
  
  /*
   *
   */
  function format() {
    $this->dbg('');
    parent::format();
    
    $ranks = array();
    locateAndInclude('bForm_vm_Event');
    foreach(bUnit::get_unitsMembership(array_merge(array(myOrg_ID),
						   bForm_vm_Event::getEvents()),
				       array(RANK_vm_manager,
					     RANK_vm_endorser,
					     RANK_vm_organizer,
					     ),
				       $this->rec['av_id']) as $av_id=>$rrs){
      foreach($rrs as $k=>$r) $ranks[] = $r['u_rank'];
    }
    
    // Add visits information
    locateAndInclude('bForm_vm_Visit');
    foreach(array(RANK_vm_visitor => count(bForm_vm_Visit::getVisits($this->rec['av_id']))
		  ) as $rank=>$condition){
      if ($condition > 0) $ranks[] = $rank;
    }
    
    // Loop thru the roles
    foreach(array_unique($ranks) as $rank){
      $icon = $url = Null;
      switch($rank){
      case RANK_vm_visitor:
      case RANK_vm_registrant:
	if (empty($url)) $url = b_url::same('?av_id='.$this->rec['av_id'].'&holder=vm_Visitors');      
      case RANK_vm_endorser:  
	if (empty($url)) $url = b_url::same("?ed_bList=bUnit_vm_endorses"); 
      case RANK_vm_organizer:
	if (empty($url)) $url = b_url::same('?tab_force=tab_vm_adm_unitOrganizers&avid_once='.$this->rec['av_id']); 
	$icon = b_reg_bUnit::get_icon($rank,VM_MODULE);
	break;
      default:
      }
      if (!empty($icon)){
	$this->t->extraTD = array_diff($this->t->extraTD,array($icon));
	if(!$url)$this->t->extraTD[] = $icon;
	else     $this->t->extraTD[] = b_btn::link($icon,$url);
      }
    }
  }
}
