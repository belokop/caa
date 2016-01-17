<?php
locateAndInclude('bForm');
locateAndInclude('bForm_Avatar');

/*
 * Load the debug helper
 */
if (cnf_dev) locateAndInclude('bForm_debug.inc');

class bForm_vm extends bForm {
  
  function __construct($id, $mode='RO') {
    parent::__construct($id,$mode);
    start_VM();
  }

  function isManager(){
    return VM_manager_here;
  }

  private $isWritable_vm = Null;
  function isWritable(){
    bTiming()->cpu(__FUNCTION__);
    if ($this->isWritable_vm === Null) $this->isWritable_vm = (bool)VM_manager_here || parent::isWritable();
    $this->dbg($this->isWritable_vm);
    bTiming()->cpu();
    return $this->isWritable_vm;
  }
}
