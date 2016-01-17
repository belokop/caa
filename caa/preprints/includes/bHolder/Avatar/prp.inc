<?php
b_reg::load_module(PRP_MODULE);
locateAndInclude('bHolder_Avatar');
class bHolder_Avatar_prp extends bHolder_Avatar{

  protected $left_join = array('LEFT JOIN prp_list ON prp_avid = av_id');


  /*
   *
   */
  function __construct(){
    parent::__construct();
    if (!($av_id = @$_GET['av_id'])){
      $this->where[] = 'prp_id > 0';
    }
  }
  
}
