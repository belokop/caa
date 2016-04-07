<?php
/**
 *                          2010-05-30
 */
locateAndInclude('bForm_ea_Office');
class bForm_vm_Office extends bForm_ea_Office{

  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }

  function isWritable() { 
    $reply = (defined('VM_administrator_here') && VM_administrator_here ) || (defined('EA_administrator_here') && EA_administrator_here);
    return $reply;
  }
}
