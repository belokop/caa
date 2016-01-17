<?php
/*
 * Root bForm for the jam module
 */


/*
 * Load the debug helper
 */
if (cnf_dev) locateAndInclude('bForm_debug.inc');

locateAndInclude('bForm');
class bForm_jam extends bForm{
  
  function  __construct($id,$mode='RO'){
    parent::__construct($id,$mode,True);
    $this->defineVariable(array('JAM_posID' => 'Current position'), True);
  }

  /*
   * getValue
   */
  function getValue($name,$isIs=True,$dMBE=True){
    return parent::getValue($name,True,True);
  }

  /*
   * JAM does not need comprehensive block structure, so make it simple...
   */
  function formBlock($block_ID,$blockTitle, $fields, $button=array(),$extraTD=''){

    // Use the title hash as the id
    $block_ID = $this->blockID_hash($blockTitle);

    if ($button==='auto'){
      $button = array('l' => $this->sameURL_but(array('mode'=>'RW')),
                      'd' => 'auto');
    }elseif(empty($button)){
      $button = array();
    }elseif(is_array($button)){
    }else{
      b_debug::internalError('?');
    }
    return parent::formBlock($block_ID,$blockTitle, $fields, $button);
  }

  /*
   * The object status consists of 2 parts
   * - status which is set explicitly by administrator (like "position is in preparation")
   * - status which is set by the software (like "the application form contains errors) 
   */
  protected function statusBlock($status_field){

    if ($this->MODE == 'RO'){
      // In the RO mode show the list icons
      $allStatusIcons = join('',self::_allStatusIcons($this->formDB,$status_field));
      $this->formDB[$status_field] = $allStatusIcons;
      $this->getValue_reset($status_field);
      $this->textField($status_field);
    }else{
      $list = array();
      $valid_status = False;
      foreach(JAM::$statuses as $id=>$s){
	if (@$s['f'] == $status_field){
	  $condition = $this->_call_bForm_method(@$s['if']);
	  if ((($code = $s['c']) > 9) && $condition){
	    if ($this->getValue($status_field) == $code) $valid_status = True;
	    $list[$code] = $s;
	  }      
	}      
      }      
      if ($valid_status)      $list[1] = array('c'=>'','d'=>'reset status');
      else $this->getValue_reset($status_field,'');
      ksort($list);
      $this->selectMenu($status_field,$this->selectMenuFromStatus($status_field,$list));
    }
  }

  /*
   * Get status from "<x>_status" & "<x>_status_auto" fields
   */
  public static $__statuses = array();
  public static function __isStatus($id,$formDB,$status_field){

    self::$__statuses = array();

    $code = JAM::$statuses[$id]['c'];
    self::$__statuses[] = ((@$formDB[$status_field] == $code) ? $id : '');
    
    if (!empty($formDB[$status_field.'_auto'])){
      foreach(unserialize($formDB[$status_field.'_auto']) as $f) self::$__statuses[] = $f;
    }

    $reply = in_array($id,self::$__statuses);
    // myPear::DEBUG(__METHOD__."($id, statuses=(".join(',',self::$__statuses).")) = ".(int)$reply);
    return $reply;
  }

  /*
   * Combine all the statuses into an array
   */
  public static function _allStatusIcons($formDB,$status_field){

    $icons = array();
    $mainStatus_icon = '';
    foreach(array_keys(JAM::$statuses) as $id){
      if (@JAM::$statuses[$id]['f'] !== $status_field) continue;
      $icon = (self::__isStatus($id,$formDB,$status_field)
		? bIcons()->get(JAM::$statuses[$id]) 
		: '');
      
      if (JAM::$statuses[$id]['c'] > 9){
	if (!empty($icon))  $mainStatus_icon = $icon;
      }else{
	$icons[] = $icon;
      }
    }
    array_unshift($icons,$mainStatus_icon);
    
    // myPear::DEBUG(__METHOD__."($status_field) = ".join('',$icons));
    return $icons;
  }

  /*
   * JAM does not need comprehensive block structure, so make it simple...
   */
  protected function formBlock_isVisible($block_ID,$fields=array()){
    if ($reply = !empty($fields)){
      $reply = empty($_GET['block_once']) || ($block_ID == $_GET['block_once']); 
    }
    return $reply;
  }

  protected function blockID_hash($block_ID){
    return b_crypt::hash($block_ID);
  }
}
