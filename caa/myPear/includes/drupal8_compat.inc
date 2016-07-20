<?php

/*
 * Tentative... Drupal8 wruppers
 */
if (!class_exists('D8',False)){
  
  class D8{
    
  /*
   * Extract current myPear menu tab
   */
    public static function current_tab(){
      
      require_once dirname(__FILE__).'/APImenu.inc';
      
      if (!empty($_REQUEST['q'])){  
	$q = explode('/',$_REQUEST['q']);
	$GLOBALS['myPear_current_module'] = array_shift($q);
	$tab_code= (int)array_pop($q);
	$GLOBALS['myPear_current_tab'] = (empty($tab_code)
					  ? $GLOBALS['myPear_current_module']
					  : APItabs_code2tab($tab_code,$GLOBALS['myPear_current_module']));
      }
    }
    

    public static function compat($q=''){
      static $known_modules = array('prp','ea','vm','wiw','lic','jam','hw','legacy','myPear');
      
      if (!class_exists('Drupal',False)) return;
      
      // Tidy up the url, remove double&leading slashes
      list($url,$QUERY_STRING) = explode('?',$_SERVER['REQUEST_URI']);
      $u = array();
      foreach(explode('/',$url) as $i) if (!empty($i)) $u[] = $i;
      
      if (empty($_REQUEST['q'])){
	//
	// build $_REQUEST['q'] (make wrupper compatibility with D6/D7) 
	//
	if (in_array($u[0],$known_modules)){
	  $_REQUEST['q'] = $_GET['q'] = implode('/',$u);
	  
	  // Rebuild $_SERVER['QUERY_STRING'] by adding 'q'
	  if (empty($QUERY_STRING)) $QUERY_STRING = '';
	  $_SERVER['QUERY_STRING'] = $QUERY_STRING . '&q=' . $_GET['q'];
	}
      }elseif(empty($u)){
	//
	// Workaround for the old style URL.
	// To be done in .htaccess 
	//
	return $q;
	$url = sprintf('http%s://%s/%s?%s',
		       ($_SERVER['SERVER_PORT'] == 80 ? '' : 's'),
		       $_SERVER['HTTP_HOST'],
		       $_GET['q'],
		       $QUERY_STRING);
	self::dbg($url);
	if (class_exists('b_url'))b_url::header($url);
	else D8::dbg('shit...');
      }
      return '';
    }
    
    public static function dbg($text='',$instance=Null,$level=2,$type='info'){
      if (defined('cnf_dev') && cnf_dev)
	drupal_set_message(
			   preg_replace('/(Drupal|Symfony)[a-zA-Z\\\\]*\\\\/','',
					sprintf("%s: <em>%s</em>",
						self::__($level, $instance),
						(is_bool($text) 
						 ? var_export($text,True) 
						 : (is_array($text)
						  ? D8_joinX($text,False)
						    : (is_object($text)
						       ? 'object('.get_class($text).')'
						       : $text))))),
			   $type,
			   $keep_duplications=False);
    }
    
    public static function __($flags=array(),$class=Null){
      static $inside = 0;    if ($inside++) die("recursion in b_cnf::__($flags)");
      $includeClass = True;
      $includeLine  = False;
      
      // Parse input arguments
      if (!is_array($flags)) $flags = array($flags);
      foreach($flags as $item){
	if (is_bool($item))      $includeClass = $item;
	elseif ($item==='l')     $includeLine  = True;
	elseif (is_object($item))$clas = 'class('.get_class($item).')';
	else                     $level        = (int)$item;
      }
      if (empty($level)) $level = 1;
      
      // Get the class identifiers (name, ID, mode)
      if (is_object($class)){
	$classArgs = array();
	if ($id=@$class->ID)         $classArgs[] = $id;
	$clas = get_class($class) . "(".join(',',$classArgs).")";      
      }elseif (is_string($class) && !empty($class)){
	$clas = $class;
      }
      
      $calls = debug_backtrace(false);
      while (True){
	if (isset($calls[$level])){
	  $call = $calls[$level];
	  break;
	}else{
	  $level--;
	}
      }
      
      if (@$call['type']=='::') $includeClass = True;
      if (@$call['type']=='->' && !empty($clas) && $clas!='b_debug')   $call['class'] = $clas;
      
      if (!is_array($call['args'])) $call['args'] = array();
      $args = array();
      foreach($call['args'] as $a) {
	$classArgs = array();
	if (is_object($a) && ($id=@$a->ID))  $classArgs[] = $id;
	if (is_object($a) && ($id=@$a->MODE))$classArgs[] = $id;
	if     ($classArgs)      $args[] = get_class($a)."(".join(',',$classArgs).")";      
	elseif (is_object($a))   $args[] = get_class($a);      
	elseif (is_array($a))    $args[] = D8_joinX($a);      
	else                     $args[] = $a;
      }
      $reply = str_replace('[','',
			   str_replace(']','',
				       str_replace('bForm_','',join('',array((empty($call['class']) || !$includeClass ? '' : @$call['class'].@$call['type']),
									     @$call['function'], 
									     "(".join(',',$args).")",
									     ($includeLine ? ' line '.@$call['line'] : ''))))));
      $inside = 0;  
      return $reply;
    }
  }

  // Adjust the query string
  D8::compat();
  
  D8::dbg(__FILE__);

}


function D8_joinX($array,$simple_output=True){
  if (empty($array)){
    $a = array();
  }else{ 
    $a_keys = array_keys($array);
    $is_asso = !($a_keys[0] === 0 &&
		 $a_keys[count($a_keys)-1] === count($a_keys)-1);
    $a = ($is_asso
	  ? array_keys($array)
	  : $array);
  }
  if ($simple_output || !$is_asso){
    return 'array('.join(',',$a).')';
  }else{
    $reply = array();
    foreach($array as $k=>$v){
      if (is_object($v))   $v = "object(".get_class($v).")";
      elseif (is_array($v))$v = D8_joinX($v); // $v = "array(...)";
      elseif (is_bool($v)) $v = var_export($v,True);
      $reply[] = "$k=$v";
    }
    return join(', ',$reply);
  }
}

  
if (class_exists('Drupal',False)){
  /*
   * module_list 
   */
  if (!function_exists('module_list')){
    function module_list(){
      $modules = \Drupal::moduleHandler()->getModuleList();
      return array_keys($modules);
    }
  }

  /*
   * variable_get / variable_set / variable_del
   */
  if (!function_exists('variable_get')){
    function variable_get($var,$default=Null){
      return (\Drupal::state()->get($var) ?: $default);
    }
    function variable_set($var,$value){
      \Drupal::state()->set($var, $value);
    }
    function variable_del($var){
      \Drupal::state()->delete($var);
    }
  }

  /*
   * Set the 'clean_url' flag
   */
  variable_set('clean_url',1);
  
  /*
   * drupal_add_js / drupal_add_css
   */
  if (!function_exists('drupal_add_js')){
    function drupal_add_js($arg1='',$arg2=''){
      //      \D8::dbg(sprintf("CALLING OBSOLETE %s(%s,%s)",__FUNCTION__,$arg1,$arg2));
    }
  }
  if (!function_exists('drupal_add_css')){
    function drupal_add_css($arg1='',$arg2=''){
      //      \D8::dbg(sprintf("CALLING OBSOLETE %s(%s,%s)",__FUNCTION__,$arg1,$arg2));
    }
  }
  
  /*
   * Load the D8 Controller
   */
  if (function_exists('drupal_get_path')){
    require_once drupal_get_path('module','myPear') . '/Controller.inc';
    require_once drupal_get_path('module','myPear') . '/includes/myPear_classes.inc';
    foreach(module_list() as $m){
      $f = drupal_get_path('module',$m) . "/${m}.custom_hooks.inc";
      if (is_file($f)) require_once $f; 
    }
  }
}
