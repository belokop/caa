<?php
/*
 * checkBox pane
 */
class b_checkBox{

  /*
   *
   */
  public static $ON = array('on','yes');
  public static $OFF= array('off','no','0');

  private static $cb_ID = '_checkbox_';
  private static $debug = False;

  /*
   * We need to distinguish between multiple check-boxes
   */
  private static $prefix = '';
  public static function set_prefix($prefix){
    self::$prefix = $prefix;
    self::$cb_ID  = $prefix . '_checkbox_';
  }

  private static function id($id) { 
    return self::$cb_ID . str_replace(self::$cb_ID,'',str_replace('_expect','',$id));
  }
  
  public static function create($ids=array(),$separator='<br/>',$onClick='default',$classes=''){
    static $createCounter = 0; 
    if ($onClick === 'default') $onClick = "onclick='submit()' onchange='submit()'";

    if (!empty($ids)){
      $boxes = array();
      foreach ($ids as $id=>$text) {
	$_ID = self::id($id);
	$isOn= self::isOn($id);
	if (!empty(self::$prefix))  $boxes[] = "<input name='".self::$prefix."' value='b_checkBox' type='hidden'/>";
	$boxes[] =
	  "<input name='".$_ID."' type='checkbox' $isOn $onClick />$text".
	  "<input name='".$_ID."_expect' type='hidden' value='".self::$OFF[0]."' />";
      }
      return x("div class='align_left only_online".($classes ? " $classes" : "")."'",
	       x($onClick?"form name='selectBox".(++$createCounter)."' method='post'":'',
		 join($separator,$boxes)));
    }   
  }   
  
  /*
   * Return array of the selected checkboxes
   */
  public static function whatIsOn($prefix='') {
    $reply = array();
    if (!empty($prefix))  b_checkBox::set_prefix($prefix);
    foreach(preg_grep('/^'.self::$cb_ID.'/',array_keys($_POST)) as $k){
      if (strpos($k,'_expect') !== False) continue;
      $id = str_replace(self::$cb_ID,'',$k);
      if (self::isOn($id)) $reply[] = $id;
    }
    return $reply;
  }

  public static function isOn($id) {
    $_ID = self::id($id);
    if (!strstr($id,'_expect') && !empty($_POST[$_ID.'_expect']) && in_array($_POST[$_ID.'_expect'],self::$OFF)){
      if (empty($_POST[$_ID]))  self::set($id,self::$OFF[0]);
    }
    $reply = (bool)b_cnf::get($_ID);
    b_debug::xxx($reply,array(2,self::$debug));
    return ($reply ? 'checked' : '');
  }

  /*
   * Set the checkbox value
   */
  public static function set($id,$value=Null){
    $_ID = self::id($id);
    $_POST[$_ID.'_expect'] = self::$OFF[0];
    if ($value == self::$OFF[0]){
      unset($_POST[$_ID]);
    }else{
      $value = self::$ON[0];
      $_POST[$_ID] = $value;
    }
    b_cnf::set($_ID,$value);
  }
}
