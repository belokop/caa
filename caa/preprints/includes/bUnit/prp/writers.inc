<?php
locateAndInclude('bUnit_prp');
class bUnit_prp_writers extends bUnit_prp{

  function __construct($parent_id=myOrg_ID,$rank=RANK_prp_writer){
    parent::__construct(myOrg_ID,RANK_prp_writer);
    $this->bUnit_init();
  }
  
  /*
   * Include all the employers to the submitter unit, if not yet done
   */
  private function bUnit_init(){
    if (($this->rank == RANK_prp_writer) && !@$_SESSION[get_class($this)]++){
      $members = $this->get_unitMemberIDs();
      // Accept all the employers to to club
      foreach(myOrg($this->parent_ID)->eaMembers()->get_unitMemberIDs('e') as $av_id){
	if (!in_array($av_id,$members)) $this->add_unitMember($av_id);
      }
      
      // Accept those who has already submitted something
      $q = myPear_db()->query("SELECT prp_avid FROM prp_list WHERE prp_orgid = ".$this->parent_ID." GROUP BY prp_avid ORDER BY prp_avid");
      while($r = myPear_db()->next_record($q)){
	if (!in_array($r['prp_avid'],$members)) $this->add_unitMember($r['prp_avid']);
      }
      
      // Populate Alumni database with the submitters (?)
      $this->dbg();
    }
  }
  
  function isManager(){
    return PRP_censor_here;
  }

  protected function isUnitMemberDeletable(){
    locateAndInclude('bForm_prp');
    return b_cnf::check_conditions(array(False=>array('RO'    => !parent::isUnitMemberDeletable(),
						      'active'=>  bForm_prp::has_submittions($this->rec['av_id'])),
					 'default' => True),
				   $this);
  }
}
