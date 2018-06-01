<?php
/*
 * Special form to be used with bList
 */
locateAndInclude('bForm');

class bForm_lists extends bForm {

  public static $_dbg_color = 'grText';
  public static $_debug = False; 

  var $modalRW_enabled = True;

  // The bList class which is served by this class
  public $bList = Null;

  private $z_inversed;
  private $z_variables;
  function __construct(&$bList,$variables=array(),$inversed=False){
    $this->bList = $bList;
    $this->z_variables = $variables;
    $this->z_inversed  = $inversed;
    parent::__construct('new','RW',False);
  }

  /*
   *
   */
  function defineVariables(){
    $this->defineVariable($this->z_variables,$this->z_inversed);
    $this->defineVariable(array('memberID' => 'memberID'),True);
    foreach($this->bList->formDB_member as $k=>$v) $this->formDB[$k] = $v;
    foreach($this->bList->isRO          as $k=>$v) $this->isRO[$k]   = $v;
  }

  /*
   * Get the missing fields from the list.
   * We do not really care about the exact content of the missing fields, hence just reply YES / NO
   */
  protected function vitalFieldMissing($updates=array()){
    if ($this->bList->getFormAdd_OK())  $reply = array();
    else                                $reply = array(1);
    $this->dbg(!empty($reply));
    return $reply;
  }

  /*
   * Get uploadInit_path from the "parent" bList class 
   */
  function uploadInit_path($module_path=array(),$module=Null){
    return $this->bList->uploadInit_path($module_path,$module);
  }

  /*
   *
   */
  function valid_fileType($name,$extensions,$display_name=''){
    return $this->bList->valid_fileType($name,$display_name);
  }
  
  /*
   * construct a file name to be uploaded
   */
  function uploadedFile_name($name,$argsP=array(),$ext=True){
    $reply = $this->bList->uploadedFile_name($name,$argsP,$ext);
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Blist add / edit dialog
   */
  public $displayErrors_NO = False;
  protected function displayErrors_preflight(){
    
    // get the eventual report from the lists
    $this->bList->getFormAdd_OK();

    // Do not bother with the error messages for the not-yet-created records
    if (@$_POST['lm_id'] !== 'new'){
      $lm_id = $this->bList->getValue($this->bList->TABLEID_member);
      foreach(array_keys($this->bList->list_quering) as $k){
	if ($this->bList->getMemberValue($lm_id,$k)) { @$something_exists++; break; }
      }
    }
    
    $this->dbg('something_exists: '.(empty($something_exists)?"NO":"$something_exists list member(s)"));
    if ($this->displayErrors_NO || empty($something_exists)) $errors = array();
    else                                                     $errors = parent::displayErrors_preflight();
    return $errors;
  }
  
  /*
   * Print bList IDs
   */
  function closeForm($submit_button=btn_update,$extraButtons=array()){
    if (cnf_show_st || cnf_dev){
      $f = '_debug_'.get_class($this);
      $this->isRO[$f] = True;
      $this->getFormRow($f,b_fmt::redText($this->bList->__toString()));
    }
    parent::closeForm($submit_button,$extraButtons);
  }

  /*
   *
   */
  protected  function  getValue_validate_empty($k,&$value){
    $err_msg = parent::getValue_validate_empty($k, $value);
    return $err_msg;
  }

  function __toString(){
    return get_class($this) . '()';
  }
}
