<?php
locateAndInclude('bForm_Avatar_ea');
class bForm_Avatar_wiw extends bForm_Avatar_ea{
  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }

  protected function getFormBody(){
    $this->dbg(' ',True);
    $this->whoAmIBlock();
    $this->contactsBlock();
  }

  protected function displayErrors_preflight(){
    $errors = parent::displayErrors_preflight();
    if ($this->MODE == 'RO')  $errors = array();
    return $errors;
  }
}

