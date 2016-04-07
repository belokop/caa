<?php
locateAndInclude('bHolder_ea');
class bHolder_vm extends bHolder_ea{

  function __construct(){
    return parent::__construct();
  }
  
  function isWritable(){
    $reply = VM_administrator_here;
    $this->dbg($reply,$this->debug || cnf_show_w);
    return $reply;
  }

}
