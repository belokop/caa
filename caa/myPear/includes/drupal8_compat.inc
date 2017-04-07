<?php
/*
 * Tentative... Drupal8 wrappers
 */
if (!class_exists('D8',False)){
  
  class D8{
    
    /*
     * Extract the current myPear menu tab
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
      self::dbg(array('is_fp' => self::is_fp(),
		      'myPear_current_module'=>@$GLOBALS['myPear_current_module'],
		      'myPear_current_tab'   =>@$GLOBALS['myPear_current_tab']));
    }
    
    /*
     * Is it the front page?
     */
    public static function is_fp(){
      return empty($GLOBALS['myPear_current_module']) || (@$GLOBALS['myPear_current_module'] === myPear_MODULE);
    }
    
    public static function compat($q=''){
      static $known_modules = array('prp','ea','vm','wiw','lic','jam','hw','legacy','myPear');
      
      // Tidy up the url, remove double&leading slashes
      list($url,$QUERY_STRING) = explode('?',$_SERVER['REQUEST_URI']);
      $u = array();
      foreach(explode('/',$url) as $i) if (!empty($i)) $u[] = $i;
      
      if (empty($_REQUEST['q'])){
	//
	// build $_REQUEST['q'] (make wrapper compatibility with D6/D7) 
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
    
    /*
     *
     */
    public static function cnf_localhost(){
      if (!defined('cnf_localhost')) define('cnf_localhost',
					    (bool)(preg_match('/^((.*)?::1$|127.0.0.1|local)/',trim(@$_SERVER["REMOTE_ADDR"])) &&
						   strpos(__FILE__,'/trunk/')!==False));
      return cnf_localhost;
    }
    
    /*
     *
     */
    public static function cnf_dev(){
      return ((defined('cnf_dev') && cnf_dev) || self::cnf_localhost());
    }
    
    /*
     *
     */
    public static function var_dump($variable,$comment='',$type='debug'){
      if (self::cnf_dev()){
	ob_start();
	var_dump($variable);
	$dump = ob_get_contents();
	ob_end_clean();
	drupal_set_message(sprintf("<pre>%s(%s)\n%s</pre>",__METHOD__,$comment,$dump),
			   $type,
			   $keep_duplications=False);
      }
    }
    
    /*
     *
     */
    public static function dbg($text='',$instance=Null,$level=2,$type='debug',$color='red'){
      if (self::cnf_dev()){
	if (is_string($instance)) { $color=$instance; $instance=Null; }
	drupal_set_message(self::var_export(sprintf("%s<span style='color:%s;'>%s: <em>%s</em></span>",
						    (class_exists('b_debug',False) ? b_debug::s_e_depth() : ''),
						    $color,
						    self::__($level, $instance),
						    self::var_export($text))),
			   $type,
			   $keep_duplications=False);
      }
    }

    /*
     * var_export extension
     */
    public static function var_export($text){
      return (is_bool($text) 
	      ? var_export($text,True) 
	      : (is_string($text)
		 ? preg_replace('/(Drupal|Symfony)\\\\[a-zA-Z\\\\]*\\\\/','',
				str_replace(getcwd().'/','&lt;cwd&gt;',
					    str_replace(dirname(dirname(__FILE__)).'/','',$text)))
		 : (is_object($text)
		    ? 'object('.self::var_export(get_class($text)).')'
		    : (is_array($text)
		       ? self::joinX($text,False)
		       : $text))));
      
    }

    /*
     *
     */    
    public static function traceBack(){

      if (self::cnf_dev()){
	foreach(debug_backtrace() as $n=>$bt){
	  if (empty($bt['args'])) $bt['args'] = array();
	  $args = array(); foreach($bt['args'] as $a) $args[] = self::var_export($a);
	  $args = '('.join(',',$args).')';
	  
	  $tr[] = '<tr>'.join(" ",array('<td>'.$n.'</td>',
					'<td>'.(empty($bt['line'])
						? ''
						: self::var_export($bt['file']) . '&nbsp;;&nbsp;'. $bt['line']).'</td>',
					'<td>'.@$bt['class'] . @$bt['type'] . $bt['function'] . $args . '</td>',
					)).'</tr>';
	}
	$traceBack = '<table>'.'<tbody>'.join("\n",$tr).'</tbody></table>';
	if (cnf_CLI) print str_replace('&nbsp;',' ',strip_tags("\n".$traceBack."\n"));
	else self::dbg($traceBack,Null,2,'debug','gray');
      }
    }
    
    /*
     *
     */
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
      
      $calls = debug_backtrace(false,$level+1);
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
	if (is_object($a)){
	  $classArgs = array();
	  if ($id=@$a->ID)  $classArgs[] = $id;
	  if ($id=@$a->MODE)$classArgs[] = $id;
	  $args[] = get_class($a)."(".join(',',$classArgs).")";      
	}elseif (is_array($a))   $args[] = self::joinX($a);      
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
    
    /*
     *
     */
    static function joinX($array,$simple_output=True){

      if (empty($array)){
	$a = array();
	$simple_output = True;
	$is_associative = False;
      }else{ 
	$a_keys = array_keys($array);
	$is_associative = !($a_keys[0] === 0 &&
			    $a_keys[count($a_keys)-1] === count($a_keys)-1);
	$a = ($is_associative
	      ? array_keys($array)
	      : $array);
      }
      if ($simple_output || !$is_associative){
	if ($is_associative) $a = array_keys($a);
	$a_filtered = array();
	foreach($a as $item) $a_filtered[] = (is_object($item) ? 'object '.get_class($item) : $item);
	return '['.join(', ',$a_filtered).']';
      }else{
	$reply = array();
      foreach($array as $k=>$v) $reply[] = "$k=".self::var_export($v);
      return join(', ',$reply);
      }
    }
  }
}

if (class_exists('Drupal',False)){
  
  /*
   * module_list 
   */
  function module_list(){
    $modules = \Drupal::moduleHandler()->getModuleList();
    return array_keys($modules);
  }
  
  /*
   * variable_get / variable_set / variable_del
   */
  function variable_get($var,$default=Null){
    return (\Drupal::state()->get($var) ?: $default);
  }
  function variable_set($var,$value){
    \Drupal::state()->set($var, $value);
  }
  function variable_del($var){
    \Drupal::state()->delete($var);
  }
  
  /*
   * drupal_add_js / drupal_add_css
   */
  function drupal_add_js($arg1='',$arg2=''){
    //      \D8::dbg(sprintf("CALLING OBSOLETE %s(%s,%s)",__FUNCTION__,$arg1,$arg2));
  }
  function drupal_add_css($arg1='',$arg2=''){
    //      \D8::dbg(sprintf("CALLING OBSOLETE %s(%s,%s)",__FUNCTION__,$arg1,$arg2));
  }
  
  /*
   * D8 compats
   */
  if (class_exists('Drupal',False)){
    
    /*
     * Adjust the query string
     */
    D8::compat();
    
    /*
     * Get the current module & tab
     */
    //  D8::current_tab();
  }
}

/*
 * Workaround...
 */
function get_clean_url(){
  static $clean_url = Null;
  if ($clean_url === Null){
    $clean_url = (class_exists('Drupal',False)
		  ? (int)True
		  : (int)!\D8::cnf_localhost());
    D8::dbg(['getcwd()'=>getcwd()]);
  }
  $drupal_clean_url = variable_get('clean_url','???');

  /////////////////  Force "non clean url" for Drupal7
  if (HOSTED_BY_CMS_vrsn == 7) {  $clean_url = $drupal_clean_url = 0;  variable_set('clean_url',($clean_url ? 1 : 0)); } //workaround....
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if ($clean_url === $drupal_clean_url){
    D8::dbg($clean_url);
  }else{
    \D8::dbg(['drupal'     =>var_export($drupal_clean_url,True),
	      __FUNCTION__ =>var_export($clean_url,True)]);

    variable_set('clean_url',($clean_url ? 1 : 0));
    $msg = sprintf("Set 'clean_url' %s --> %s",var_export($drupal_clean_url,True),var_export($clean_url,True));
    if (class_exists('myPear',False))  myPear::WARNING($msg);
    else D8::dbg($msg);
  }
  return $clean_url;
}
