<?php
/**
 * This class allows the customization of the forms,
 * the column descriptions is kept in the database and is editable.
 *
 * It is not finished yet, the editor is not yet written...
 */

locateAndInclude('bList');
define('FORM_DESCRPTOR','Forms descriptor');

class bList_formDescription extends bList{

  var $isRO = array('lm_key' => True);
  protected $isReady  = True;

  public function __construct($parent){
    parent::__construct($parent);
  }
  
  /*
   * Get description for the item, aka "questions to be answered",
   * shown on the left side of the form
   *
   * @param $id
   *   Field (column name) to be described
   * @param $default
   *   Default decsription of $id
   * @return
   *   Description from the database if it exists,
   *   the default value otherwise.
   *   If the description for the item $id does nor exist in the dabase, 
   *   a new record is created with the default description. 
   */ 
  function get($id,$default=''){

    if ($id == '_post_bForm_ID_once') return $default;

    $record = $this->getMembers(array('lm_key'=>$id));
    if (empty($record)){
      $this->addMember(array('lm_key'=>$id,'lm_value'=>$default));
      return $default;
    }else{
      foreach($record as $id=>$descr) $reply = $descr;
      if (empty($reply)) b_debug::internalError(b_debug::__());
      b_debug::xxx($reply['lm_value'],array('M',2, (bool)$this->debug));
      return $reply['lm_value'];
    }
  }

  public static function show_all(){
    $q = myPear_db()->query("SELECT * FROM zzz_lists WHERE 0 ORDER BY l_parent");
    while($r = myPear_db()->next_record($q)){
      if ($object = myPear::getInstanceFromID($r['l_parent'],'class')){
	print str_replace('bForm_','',get_class($object)) .' '. $object->name().'<br/>';
	$l = new bList_formDescription($r['l_parent']);
	$l->getForm();
      }else{
	b_debug::print_r($r,'not matched');
      }
      break;
    }
  }

  protected function format(){
    foreach(array('-BR-'=>'<br/>',
		  ) as $f=>$t){
      $this->r['lm_value'] = str_replace($f,$t,$this->r['lm_value']);
    }
  }

  /*
   *  widget($file,$text,$showTouteSuite=False,$showCloseButton=True)
   */
  function widget($text,$lm_key){
    $widget = array();
    $widget[] = "<center>"; // against all the rules... 
    $widget[] = x("form action='".b_url::same()."'",
		  x("textarea name='$lm_key' cols=30 rows=6",$this->get($lm_key)).
		  "<input type='submit'/>");
    $widget[] = "</center>"; // against all the rules... 
    return bJS()->modal_inline($text, join('',$widget), False, True);
  }
  
}
