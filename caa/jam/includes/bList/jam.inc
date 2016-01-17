<?php
locateAndInclude('bList');

class bList_jam extends bList{
  public function __construct($parent_ID){
    parent::__construct($parent_ID);
  }

  /*
   * Writeable by the position administrator (what about the secretary?)
   */
  function isManager(){
    $reply = JAM_posadmin_here;
    return $reply;
  }
}
