<?php
locateAndInclude('bUnit');
class bUnit_prp extends bUnit{

  function __construct($parent_id=myOrg_ID,$rank){
    parent::__construct(myOrg_ID,$rank);
  }
}
