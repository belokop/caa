<?php
/*
 * Censors are preprint submitters which approve / disaprove the preprints
 */
locateAndInclude('bUnit_prp');
class bUnit_prp_censors extends bUnit_prp{
  //  function __construct($parent_ID=myOrg_ID,$rank=RANK_prp_censor){
  function __construct(){
    parent::__construct(myOrg_ID,RANK_prp_censor);
  }
  
  protected function isUnitMemberDeletable(){
    return $this->isManager();
  }

  function isManager(){
    return PRP_manager_here;
  }
}
