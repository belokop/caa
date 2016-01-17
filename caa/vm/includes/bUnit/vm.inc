<?php
locateAndInclude('bUnit');

class bUnit_vm extends bUnit{

  public $need_reminder = False;

  /*
   *
   */
  function  __construct($parent_ID,$rank){

    // We might be called before the "official start"
    start_VM();

    if (empty($parent_ID))    $parent_ID = myOrg_ID;
    if ($this->need_reminder) $this->packed_items[] = 'reminder';
    parent::__construct($parent_ID,$rank);
  }

  /*
   *
   */
  function isWritable(){
    return VM_manager_here;
  }

  /*
   *
   */  
  function isAdmin(){
    return VM_booker_here;
  }
}
