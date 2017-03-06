<?php
locateAndInclude('bUnit_ea');

class bUnit_ea_admins extends bUnit_ea{

  function __construct($parent_ID,$rank){
    parent::__construct(myOrg_ID,RANK__ea_admin);
  }
}
