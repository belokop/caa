<?php


// set_error_handler('myPear_exceptions_error_handler');

/*
 *
 */
function myPear_exceptions_error_handler($severity, $message, $filename, $lineno) {

  if ((error_reporting() == 0) || 
      (defined('cnf_dev') && !cnf_dev) ||
      (class_exists('b_cnf',0) && !b_cnf::get('warnings'))) return;
  
  if (error_reporting() & $severity) {
    throw new ErrorException($message, 0, $severity, $filename, $lineno);
  }
}

/*
 * Find the file in the class path and load it.
 * Return True if the object located, False otherwise
 */
function locateAndInclude($objects,$flag=''){
  static $include_depth = -1;
  static $dejaVu = array();
  
  // May be the class is already found...
  if    (empty($objects))     $objects = array();
  elseif(!is_array($objects)) $objects = array($objects);
  foreach($objects as $o) if (!class_exists($o)) $something_missing[] = $o;
  if (empty($something_missing)) return True;
  //elseif(($something_missing !== $objects) && class_exists('b_debug',0)) b_debug::print_r($something_missing,__FUNCTION__.'('.join(',',$objects).'), looking for:');

  // Switch on timing, if needed
  if ($bTiming = function_exists('bTiming')) bTiming()->cpu(__FUNCTION__);
  
  // Get the include paths
  $path  = myPear_register_config(__FILE__);
  
  $include_depth++;
  $reply = True;
  $n_msg_printed = 0;
  foreach($objects as $object){
    if (class_exists($object,False) || @$dejaVu[$object]) continue;
    foreach(array('.inc','.php','') as $ext){ 
      foreach ($path  as $d){
	foreach(array($object,
		      str_replace('_',DIRECTORY_SEPARATOR,$object)) as $o){                          
	  $source_file = "$d/$o$ext";
	  if (file_exists($source_file) && is_file($source_file)){
	    // if (class_exists('b_debug',False) && defined('cnf_dev') && !@$starting) { $starting=1; b_debug::_debug(starting); }
	    if (!$n_msg_printed++ & ($include_depth==0) && defined('cnf_show_l') && cnf_show_l){
	      $msg = "start ".__FUNCTION__.x('(',join(',',$objects));
	      if (class_exists('b_debug',0)) b_debug::xxx($msg,'L');
	      elseif (function_exists('drupal_set_message') && class_exists('b_cnf',0) && !b_cnf::get('nodrupal')) drupal_set_message("... $msg",'debug');
	    }
	        
	    if ($include_depth>=0 && defined('cnf_show_l') && cnf_show_l && class_exists('b_debug',0)){
	      b_debug::xxx("$include_depth&nbsp-&nbsp;".str_repeat('&nbsp;&nbsp;',$include_depth)." $source_file",'L');
	    }
	    if ($bTiming) bTiming()->cpu('load_located_file');
	    require_once($source_file);
	    if ($bTiming) bTiming()->cpu();
	    @$object_found[$object]++;
	    @$dejaVu[$object]++;
	    break 3;
	  }
	}
      }
    }
    if (empty($object_found[$object])) $reply = False;
  }
  
  // Object not found, see the options
  if (!$reply){
    if ($flag === 'fatal'){
      echo "<pre>\n";
      var_dump($path);
      $msg = sprintf("?????? Can't locate %s\n",var_export($object,True));
      debug_print_backtrace();
      if (class_exists('b_debug',0)) b_debug::internalError($msg);
      die($msg);
    }elseif($flag === 'any'){
      $reply = !empty($object_found);
    }
  }
  $include_depth--;
  if ($n_msg_printed && defined('cnf_show_l') && cnf_show_l && class_exists('b_debug',0) && ($include_depth<0)){
    if (!$reply) b_debug::xxx("&nbsp;?????? Can't locate \"$object\"",'L');
    b_debug::xxx("end ".__FUNCTION__.x('(',join(',',$objects)),'L');
  }
  if ($bTiming) bTiming()->cpu();
  // var_dump(sprintf("%s(%s) = %s",__function__,var_export($objects,True),var_export($reply,True)));
  if (@$starting) b_debug::_debug(exiting); 
  return $reply;
}

/*
 * Resolving clashes in the startup mess, when some modules need information which is not yet available.
 */
function myPear_startup_queue($function=Null,$args=array(),$condition=Null){
  static $myPear_startup_queue;

  if (!is_array($args)) $args = array($args);
  if (empty($args)) $args = array();
  if (empty($myPear_startup_queue)) $myPear_startup_queue = array();

  // Execution request    
  if (empty($function)){
    // Execute the collected functions, then empty the queue
    foreach($myPear_startup_queue as $entries){
      foreach($entries as $function=>$args) call_user_func_array($function,$args);
    }
    unset($myPear_startup_queue);
  }else{
    // Check the queuing condition, execute immediately if the condition is false
    if ($condition === Null) $condition = !myPear::$isReady;
    if(!$condition){
      b_debug::_debug("executing without queuing...",cnf_dev);
      call_user_func_array($function,$args);
      return;
    }
    // Otherwise put put the object to the queue
    $myPear_startup_queue[] = array($function => $args);
    if (class_exists('b_debug',False)) b_debug::_debug("adding to the startup queue",cnf_dev);
  }
}

function EA_members($parent_id=myOrg_ID){
  static $bUnit_ea_members = array();
  if (empty($parent_id)) $parent_id = myOrg_ID;
  if (empty($bUnit_ea_members[$parent_id])){
    locateAndInclude('bUnit_ea_members');
    $bUnit_ea_members[$parent_id] = new bUnit_ea_members($parent_id,RANK__ea_member);
  }
  return $bUnit_ea_members[$parent_id];
}

function EA_member_here(){
  if (!defined('EA_member_here')) define('EA_member_here',(is_object(bAuth::$av)
                                                           ? myOrg()->eaMembers()->isMember(bAuth::$av->ID)
                                                           : False));
  return EA_member_here;
}

/*
 * Create the path for the source files
 */
function myPear_register_config($config_file){
  static $cache = array();
  if (empty($cache['C'])) $cache['C'] = array();
  if (!in_array(md5($config_file),$cache['C'])){
    
    // Get the php include path when there is no modules loaded yet
    $PATH = @$cache['P'];
    if (empty($PATH)){ 
      $PATH = array();
      foreach(explode(PATH_SEPARATOR,get_include_path()) as $p){
	$PATH[] = $p; 
	if ((strpos($p,DIRECTORY_SEPARATOR) !== False) && is_dir($pp=$p.DIRECTORY_SEPARATOR."PEAR"))  $PATH[] = $pp;
      }
      $cache['I'] = join(PATH_SEPARATOR,array_unique($PATH));
    }
    
    // Parse the config file path into $PATH
    myPear_register_config_parser($config_file,$PATH);
    
    // Add the path to all known modules when the database becomes available
    // and save the path for the session. It is not large...
    if (class_exists('b_reg',0)){
      myPear_register_config_parser(b_reg::get_modules(True),$PATH);
    }
    $cache['C'][] = md5($config_file);
    $cache['P'] = $PATH;
  }
  
  static $dejaVu = 0;
  if (!$dejaVu++) {
    set_include_path($cache['I']);                  // Works as of PHP 4.3.0
    ini_set('include_path',$cache['I']);            // Works in all PHP versions
  }
  
  // give back the reply
  return $cache['P'];
}

function myPear_register_config_parser($config_files,&$path){

  static $includes = array('includes2','includes','templates','custom');

  if (!is_array($config_files)) $config_files = array($config_files);
  foreach(array_values($config_files) as $config){
    if (!is_file($config))  continue;
    foreach($includes as $src){
      $d = dirname($config) . "/$src";
      if (is_dir($d)) $path[] = $d;
    }
  }
  $path = array_unique($path);
  sort($path);
  return $path;
}



/*
 * Check that the server connects to the given port.
 * Optionally check also that the server replies request
 */
function myPear_hostReplies($host,$port=443,$check_reply=False,$timeout=5){

  $reply = False;

  // Check the connection
  try{
    $fp = fsockopen($host, $port, $errno, $errstr, $timeout);
  }catch(Exception $e){
    $fp = False;
    //      echo "??? Error Exception: ".$e->xdebug_message."\n";
    $msg = "ERROR after $timeout secs. timeout: $errstr ($errno)";
    if (class_exists('b_debug',0)) b_debug::xxx($msg);
    else echo "$msg\n";
  }

  // Send request to the server, test reply
  if ($fp){
    if ($check_reply){
      fwrite($fp, ("GET / HTTP/1.1\r\n".
		   "Host: $host\r\n".
		   "Connection: Close\r\n\r\n"));
      while (!feof($fp)){
	$buffer = fgets($fp, 128);
	if (!empty($buffer)) $reply = True;
      }
    }else{
      $reply = True;
    }
    fclose($fp);
  }
  return $reply;
}


/*
 * bAuth class wrapper.
 * Keep the bAuth instance
 */
function bAuth(){
  static $bAuth = Null;
  static $inside_bAuth = 0;
  
  if (!myPear::$isReady){
    myPear_startup_queue(__function__,array(),True);
  }elseif (($bAuth === Null) && !$inside_bAuth++){
    if (($bTiming=!bAuth::$isReady && function_exists('bTiming'))) bTiming()->cpu(__FUNCTION__);
    b_debug::_debug(starting);
    
    // Load the class file
    locateAndInclude('bAuth');
    locateAndInclude('bForm_Avatar');
    
    // Get the class instance
    $bAuth = new bAuth();
    
    // Try to "auto-login", 
    // i.e. check if the SERVER[QUERY_STRING] contains the encoded password.
    // Auto-logout if the session is too old.
    if (!bAuth::authenticated())    $bAuth->autoLogin();
    $bAuth->authenticate();
      
    // cnf_demo_email();
    // debug_auth();
    
    // Create the first Avatar for the empty database
    switch(HOSTED_BY_CMS){
    case 'drupal':
      bAuth::drupal_hook_user();
      break;
    }
    
    // Get flag "superUser is logged in" 
    if (!isset($_SESSION[CONST_cnfOptions]['superUser_here'])){
      myPear_sessionStart();
      
      if (bAuth::authenticated()){
	// Give the superuser access to myPear API
	$su = ((bAuth::$av->getValue('av_email')==bMailer_noreply) || myPear::_UNIT(RANK__superuser)->isMember(bAuth::$av));
	if ($su) b_reg::set_module_component(myPear_MODULE,array('d'=>myPear_MODULE));
	$_SESSION[CONST_cnfOptions]['superUser_here'] = $su;
      }else{
	$_SESSION[CONST_cnfOptions]['superUser_here'] = False;
      }
    }
    define('superUser_here', $_SESSION[CONST_cnfOptions]['superUser_here']);
    
    
    // Set the current module
    b_reg::load_module(b_reg::_(False));
    
    // Fit the Authenticated Avatar to the current module
    myPear_set_roles();
    bForm_Avatar::set_context(bAuth::$av);
    
    // See who is currently connected
    myPear_logs()->who_is_connected();
    
    $inside_bAuth = 0;
    if ($bTiming) bTiming()->cpu();
    if (is_object(bAuth::$av)) b_debug::xxx(bAuth::$av->fmtName());
    b_debug::_debug(exiting);
  }
  return $bAuth;
}

/*
 * set the developer email address, to which ALL the emails should be sent when running in the "cnf_dev/cnf_demo" mode
 */
function cnf_demo_email($forced=''){
  if (!defined('cnf_demo_email')){
    
    $cnf_demo_email = (empty($forced)
		       ? (is_object(bAuth::$av_authenticated) && !cnf_localhost
			  ? bAuth::$av_authenticated->getEmail()
			  : cnf_dev_email)
		       : $forced);
    
    define('cnf_demo_email',$cnf_demo_email);
    
    if (cnf_demo || cnf_dev){
      MSG::MESSAGE_forced('',($msg=sprintf("All the Emails are redirected to '%s'",cnf_demo_email)));
      MSG::DEBUG($msg,True);
    }
  }
  return cnf_demo_email;
}

// Get the superuser email
function cnf_superUser_email(){
  if (!defined('cnf_superUser_email')){
    foreach(myPear::_UNIT(RANK__superuser)->getMembers() as $k=>$v){
      $av = new bForm_Avatar($v['um_avid']);
      $su_emails[] = $av->getEmail();
    }
    define('cnf_superUser_email',(empty($su_emails)
				  ? cnf_demo_email()
				  : join(',',$su_emails)));
  }
  return cnf_superUser_email;
}

/*
 * Define constants, set the debug level. 
 * There are 4 debug levels: 
 *    0 production, no debug 
 *   >0 some debug
 *  >10 more debug 
 * >100 detailed debug
 */
function myPear_setConstants(){
  static $dejaVu = 0;
  if ($dejaVu++) return;

  // In the non-developer mode the debug printouts are switched on only if 'YBhere' is also set,
  //  Naive protection against someone switching on debug options on the production server by mistake 
  if (!defined('YBhere')) define('YBhere',((bool)b_cnf::get('YBhere') || cnf_localhost));
  
  // Switch on/off the debug printout
  // Fine grade the debugging option
  if (!defined('cnf_debug_level'))define('cnf_debug_level',(int)b_cnf::get('debug'));
  if (!defined('cnf_debug'))      define('cnf_debug',(cnf_debug_level > 0));

  // myPear first day
  define('myPear_day0',b_time::txt2unix('1999-12-31'));
  define('myPear_day9',CONST_eternity_t);

  // No File System
  if (!defined('cnf_noFS'))       define('cnf_noFS',      (bool)b_cnf::get('noFS')); 

  // Xxx-debug options 
  if (isset($_GET['debug'])){
    foreach(array('auth','form') as $opt)if (!isset($_GET[$opt.'debug']))       $_GET[$opt.'debug'] = $_GET['debug'];
  }
  
  // cnf_show_XXX options 
  if (isset($_GET['ii'])) $_GET['i'] = $_GET['ii'];
  $debug_options = array('show' =>array('a','b','c','cd','cot','d','dd','dupl','fnc','h','gv','l','m','i','ii','r','e','pe','pv','v','acl',
					's','t','w','z','hook','cache','ic','db','dp','st','er',
					'sm','ms','en','exec','mrg','pdf','sem','auth','reg','out','delete'),
			 'debug'=>array('auth','form','db'));
  foreach($debug_options as $prefix=>$opts){
    foreach($opts as $opt){
      $get = ($prefix == 'show' 
	      ? $prefix . $opt
	      : $opt . $prefix);
      if (YBhere){
	// "showii" forces "showi"
	if (isset($_GET[$get.$opt]))                      $_GET[$get] = $_GET[$get.$opt];
	if (isset($_GET['debug']) && !isset($_GET[$get])) $_GET[$get] = $_GET['debug'];
	$value = b_cnf::get($get); 
	if (empty($_GET[$get])) unset($_GET[$get]);
      }else{
	unset($value);
      }
      if (!defined($d = 'cnf_show_'.$opt)) define($d,!empty($value));    
    }
  }

  ///
  // Are we in the development or demo mode?
  ///
  if (!defined('cnf_dev')){
    define('cnf_dev',(bool)(cnf_localhost || (b_cnf::get('dev') && YBhere)));
  }
  
  // Demo site has "test" as part of the name
  if (!defined('cnf_demo')){
    if (cnf_dev)            define('cnf_demo',False);
    else                    define('cnf_demo',preg_match('/(test|norosx|^192.168|130.242.129.1[01])/',@$_SERVER["HTTP_HOST"]));
  }
  if (cnf_dev && cnf_inside_fb) MSG::DEBUG(cnf_inside_fb_iframe
					      ?'cnf_inside_fb_iframe YES'
					      :(cnf_inside_fb_ajax
						?'cnf_inside_fb_ajax YES'
						:''));
}

/*
 *
 */
function myPear_access($module=Null)   {

  b_reg::_();

  if (empty($module)) $module = b_reg::$current_module;
  $access_function = (empty($module) ? myPear_MODULE : strToUpper($module)) . '_access';

  // Look for the current module access class
  locateAndInclude('APIaccess');
  if (!locateAndInclude('APIaccess_'.strToLower($module.'.inc')) ||
      !function_exists($access_function)) $access_function = myPear_MODULE . '_access';

  if ($access_function === myPear_MODULE . '_access'){
    static $_ACCESS = Null;
    if ($_ACCESS === Null) $_ACCESS = new APIaccess();
    $reply = $_ACCESS;
  }else{
    $reply = call_user_func($access_function);
  }
  return $reply;
}

/*
 * APIaccess_ea wrapper
 */
function EA_access(){
  static $_ACCESS = Null;
  if ($_ACCESS === Null){
    b_reg::load_module(EA_MODULE);
    locateAndInclude('APIaccess_ea','fatal');
    $_ACCESS = new APIaccess_ea();
  }
  return $_ACCESS;
}

/*
 * Search-class instance holder
 */
function myPear_search(){
  static $instance = Null;
  if ($instance === Null){
    locateAndInclude('bForm_Avatar_Search');
    $instance = new bForm_Avatar_Search();
  }
  return $instance;
}

/*
 * Store myPear roles
 */
function myPear_set_roles($what='roles',$roles=array()){
  switch($what){
  case 'roles':
    static $myPear_role = array();
    if (empty($roles)){
      foreach($myPear_role as $module=>$ranks) b_reg_bUnit::register($module,$ranks);
    }else{
      $myPear_role = $roles;
    }
    break;
    
  case 'affiliation':
    // Save the default affiliation table
    static $myPear_affiliation = array();
    if (empty($roles)) return $myPear_affiliation;
    else  $myPear_affiliation = $roles;
  }
}

/*
 * Start the session
 */
function myPear_sessionStart(){
  static $dejaVu = 0;
  
  if (!isset($_SESSION)){
    // Start the session
    if (function_exists('drupal_session_start'))   drupal_session_start();
    else                                           session_start();
    
    // Rewrite the default session lifetime
    if (!$dejaVu++ && defined('myPear_autoLogoutTime')) ini_set('session.gc_maxlifetime',myPear_autoLogoutTime);
  }
}

/*
 * Database access class
 */
function myPear_db(){
  static $db = Null;
  if ($db === Null){      
    // Get the DB engine
    global $DBengine;

    // Locate the source file and create the class instance 
    locateAndInclude($DBengine,'fatal');
    $db = new $DBengine($GLOBALS[$DBengine]['host'],
			$GLOBALS[$DBengine]['user'],
			$GLOBALS[$DBengine]['passwd'],
			$GLOBALS[$DBengine]['dbname']);
  }
  return $db;
} 

/*
 * Log file
 */
function myPear_logs(){
  static $logs = Null;
  if ($logs === Null){
    locateAndInclude('bHolder_Logs','fatal');
    $logs = new bHolder_Logs();
  }
  return $logs;
}

function myPear_mailer(){
  static $mailer = Null;
  if ($mailer === Null){
    locateAndInclude('bMailer');
    $mailer = new bMailer();
  }
  return $mailer;
}

function bIcons(){
  static $class = Null;
  if ($class === Null){
    locateAndInclude('bIcons');
    $class = new bIcons();
  }
  return $class;
}

function bJS(){
  static $class = Null;
  if ($class === Null){
    locateAndInclude('bJS');
    $class = new bJS();
  }
  return $class;
}

/*
 *
 */
function myPear_cronjob(){
  static $cronjob = Null;
  if ($cronjob === Null){
    $cronjobs = bForm_Avatar::hookByEmail(bMailer_noreply);
    $cronjob = (empty($cronjobs)
		? new bForm_Avatar('empty')
		: array_pop($cronjobs));
    $cronjob->updateDB(array('av_email'    => bMailer_noreply,
			     'av_firstname'=> 'Cronjob',
			     'av_lastname' => 'Task',
			     'av_institute'=> 'crontab'));
    
  }
  return $cronjob;
}


/*
 * Holder of the Organization class(es)
 */
function myOrg($org_id=Null){

  if (!defined('myOrg_dummy')) define('myOrg_dummy',-999);
  
  if (!myPear::$isReady){
    myPear_startup_queue(__FUNCTION__,array(),True);
    return;
  }
  
  // Define the default organization
  locateAndInclude('bForm_Organization');
  if (!defined('myOrg_ID') && empty($org_id)){
    $org =  bForm_Organization::get_myOrg($org_id);
    if (is_object($org)){
      // The database is ready, get the "true" organization
      if (!defined('myOrg_ID'))       define('myOrg_ID',      ($org_id = $org->ID));
      if (!defined('myOrg_name'))     define('myOrg_name',    $org->name());
      if (!defined('myOrg_nameS'))    define('myOrg_nameS',   $org->name(True));
      if (!defined('myOrg_domain'))   define('myOrg_domain',  $org->domain());
      if (!defined('myOrg_favicon'))  define('myOrg_favicon', $org->getFavicon());
      if (!defined('myOrg_code'))     define('myOrg_code',    $org->getCode());
      if (!defined('myOrg_theme'))    define('myOrg_theme',   $org->getTheme());
    }else{
      // The database is not yet configured, get "a dummy" organization 
      if (!defined("myOrg_ID"))       define('myOrg_ID',      ($org_id = myOrg_dummy));
      if (!defined("myOrg_name"))     define('myOrg_name',    'Example Organization');
      if (!defined("myOrg_nameS"))    define('myOrg_nameS',   'Organization');
      if (!defined("myOrg_domain"))   define('myOrg_domain',  'example.com');
      if (!defined("myOrg_favicon"))  define('myOrg_favicon', 'example.ico');
      if (!defined("myOrg_code"))     define('myOrg_code',    'example');
      if (!defined("myOrg_theme"))    define('myOrg_theme',   '');
    }

    if (cnf_debug){
      $dbg = array('');
      foreach(array('myOrg_favicon',
		    'myOrg_ID', 
		    'myOrg_name',
		    'myOrg_nameS',
		    'myOrg_domain',
		    'myOrg_code',
		    'myOrg_theme',  
		    ) as $k ) $dbg[] = sprintf("define('%s','%s');",$k,constant($k));
      b_debug::xxx(join('<br/>',$dbg));
    }

    // Define the roles which people play in myPear
    myPear_set_roles();
  }
  
  if(empty($org_id)) $org_id = myOrg_ID;
  return ($org_id == myOrg_dummy
	  ? Null
	  : bForm_Organization::get_myOrg($org_id));
}

/*
 * Master class wrapper
 */
function bMaster(){
  static $class = Null;
  if ($class === Null){
    locateAndInclude('bMasterClass','fatal');
    $class = new bMasterClass();
  }
  return $class;
}

/*
 * b_tabs wrapper
 */
function myPear_tabs($tabs=array(),$selected=0){
  static $myPear_tabs = array();
  $id = serialize($tabs);
  if ($myPear_tabs[$id] === Null){
    locateAndInclude('b_tabs');
    $myPear_tabs[$id] = new b_tabs_myPear($tabs,$selected);
  }
  return $myPear_tabs[$id];
}

/*
 * Task Manager wrapper
 */
function myPear_taskManager(){
  static $myPear_taskManager = Null;
  if ($myPear_taskManager === Null){
    locateAndInclude('myPear_taskManager','fatal');
    $myPear_taskManager = new myPear_taskManager();
  }
  return $myPear_taskManager;
}

/*
 * Brackets
 */
function x($tag, $text=''){
  if     ($text === Null) return 'Null';
  elseif (empty($tag))    return $text;
  elseif (cnf_CLI)        return strip_tags($text);

  if(class_exists('b_fmt',False))  $text = b_fmt::_($text);
  
  // 'x' is an "empty" tag
  $tag_clean = preg_replace('/ .*/','',$tag);
  if ($tag_clean === 'x') return $text;

  // Usual brackets
  switch($tag){
  case '"':  if (!isset($reply))  $reply = '"'.str_replace('"',"'",$text).'"'; return $reply; 
  case "'":  if (!isset($reply))  $reply = "'".str_replace("'",'"',$text)."'"; return $reply;
  case "[":  if (!isset($reply))  $reply = '['.$text.']';
  case "(":  if (!isset($reply))  $reply = '('.$text.')';
  case "<":  if (!isset($reply))  $reply = '<'.$text.'>';
    if(class_exists('b_fmt',False))  $reply = b_fmt::escape($reply);
    return $reply;
  default:
  }
  
  // HTML tag as a "bracket"
  if (preg_match('/^(form|select|style)/i',$tag)) $delim="\n"; else $delim="";
  $reply = join($delim,array("",
			     "<".$tag.">",
			     $text,
			     "</$tag_clean>",
			     ""));
  return $reply;
}


/*
 * Escape spaces in one a text of "simple html" (i.e. <x>text</x>) string
 */ 
function no_wrap($text) { 
  $text = trim($text);
  if (strpos($text,'<') == False){
    list($o,$txt,$c) = array('',$text,'');
  }elseif(preg_match('/(^<[^>]*>)([^<]*)(<[^>]*>)$/',$text,$m)){
    list($o,$txt,$c) = array($m[1],$m[2],$m[3]);
  }else{
    $reply = $text;
  }

  if (empty($reply))    $reply = $o . str_replace(' ','&nbsp;',str_replace('  ',' ',$txt)) . $c; 

  return $reply;
}

/*
 * Escape spaces in one a "not simple html" (i.e. <x>text</x><br><y>text2</y>) string
 */ 
function nbsp($text) { 
  $dejaVu = array();
  
  $r = array();  
  foreach(preg_split(';<( *)?br( *)?/?( *)?>;',trim($text)) as $line){
    $escaped_line = no_wrap($line);
    $txt = trim(strip_tags($escaped_line));
    if (!empty($txt) && !@$dejaVu[mb_strToLower($txt,'utf-8')]++)  $r[] = $escaped_line;
  }
  $reply = join('<br/>',$r);
  return $reply;
}

/*
 * Workaround (may be there is a better one...)
 * Remove checkbox flag from _POST
 */
function set_cb_expected(&$data,$name='',$value_to_set='off'){
  if (empty($name))         $names = array_keys($data);
  elseif (!is_array($name)) $names = array($name);

  foreach($names as $ff){
    $f = str_replace('_cb_expected','',$ff);
    if ( isset($data[$f]) && isset($data["${f}_cb_expected"])) unset($data["${f}_cb_expected"]);
    if (!isset($data[$f]) && isset($data["${f}_cb_expected"])){
      $data[$f] = $value_to_set;
      unset($data["${f}_cb_expected"]);
      if (cnf_dev) b_debug::_debug(sprintf("impose %s = '%s'",$f,$data[$f]),True);
    }
  }
}



/*
 *
 */
function myPear_clean_once(){

  if (empty($_REQUEST['clean_once'])){
    if ($clean_once = preg_grep('/clean_once=/',array_keys($_REQUEST))){
      sort($clean_once);
      parse_str(preg_replace('/^.*\?/','',$clean_once[0]),$request);
      foreach(array('function','arg1_once','arg2_once') as $a){
	if (!empty($request[$a])) $_GET[$a] = $_REQUEST[$a] = $request[$a];
      }
    }
  }else{
    $request = $_REQUEST;
  }
      //  parse_str($_SERVER['QUERY_STRING'],$query);
    //  b_debug::var_dump($post,'preg_grep');
  $unsets = (empty($request['clean_once'])
	     ? array()
	     : array_diff(explode(',',$request['clean_once']),array('x','y')));
  b_debug::_debug($unsets,True);
  //  b_debug::var_dump($query,$_GET['q'],$_SERVER);
  foreach(array_intersect(array('v_avid','mode','tab'),$unsets) as $should_stay){
    $unsets = array_diff($unsets,array($should_stay));
    if (cnf_dev) MSG::WARNING_T("?????????????? clean_once $should_stay");
    if (cnf_dev) MSG::WARNING  ("?????????????? clean_once $should_stay");
  }
  if (in_array('form',$unsets)) $unsets[] = '_post_bForm_class_once';
  b_cnf::_unset($unsets);
  
  // Save the query string
  foreach(preg_grep('#^(?!(.*_once)$)#i',array_keys($_GET))as $item) b_cnf::get($item);
}

/*
 *
 */
function start_myPear(){

  bTiming()->cpu(__FUNCTION__);
  b_debug::_debug(starting);

  // Decode $_SERVER["QUERY_STRING"];
  b_crypt::decodeURL();
  
  // Please the php interpreter
  if (empty($_POST))   $_POST = array();
  if (empty($_GET))    $_GET = array();
  if (empty($_REQUEST))$_REQUEST = array();

  // Remove checkbox flag from _POST
  set_cb_expected($_POST);
  set_cb_expected($_REQUEST);
  
  ///
  // Save the "clean" (i.e. without <xxx>_once) query string in a cache.
  // We will need it to build the forms' action URL  
  ///
  myPear_clean_once();
  
  // Debug...
  if (cnf_show_d || cnf_show_dd) b_debug::dumpOptions();
  if (cnf_dev)    b_cnf::set('warnings', True);
  
  // Turn off all error reporting in the production & demo modes
  if (cnf_dev || b_cnf::get('warnings'))  error_reporting(E_ALL);
  else                                    error_reporting(0);  
    
  // some useful strings
  define('nobody_at_nowhere' , 'nobody@nowhere.earth');
      
  /*
   * Check repositories
   */
  foreach(array('myPear_tmp',
		'myPear_bin',
		'myPear_images',
		'myPear_photos',
		) as $p){
    $path = b_os::url2path(constant($p),True);
    if (!cnf_noFS){
      if (!is_dir($path) && ! cnf_CLI)  MSG::ERROR("Configuration error - can't create '$p' ($path)");
    }
    $key = "${p}_path";
    define($key, $path); 
  }

  // Load that first, it is used by all the modules
  locateAndInclude(array('bAuth',         // authentication
			 'bForm_Avatar',  // Basic Avatar class
			 'b_table',       // frequently used class...
			 'b_cache_file',  // cache HTML pages
			 'b_registry',    // Registry for modules, ranks, lists
			 ));

  //
  // Instantiate the myPear class.
  // Describe the myPear module and register it
  $myPear = new myPear();

  // Start the ranks
  myPear_set_roles();

  // Choose the preferred theme if possible  
  myPear::myPear_enabled_theme();
  
  ///
  // Instantiate the Organization we happen to serve
  ///
  myOrg();
  
  // Apply the updates
  require_once dirname(__FILE__).DIRECTORY_SEPARATOR.'b_updates.inc';
  
  ///
  // Start translation
  ///
  if (locateAndInclude('bText'))  b_t::getLangName();
  
  ///
  // Initialize first day for the bIcal matrices as Sunday of the selected week.
  // Let every module to have its local "day1" by including the module name in the "day1 id" (module_day1)
  ///
  b_reg::_();
  b_cnf::set(b_reg::$current_module.'_day1',
	     b_time::f_sunday(b_cnf::get(b_reg::$current_module.'_day1',time()),'week'));
  
  ///
  // Print "demo welcome" message
  ///
  if (cnf_demo||cnf_dev) MSG::INFO_once(array('This portal is a mirror of our real system, '.
					      'it is  used for testing and validation of the new versions of software.',
					      ' You are welcome to use this portal.<br/> ',
					      'Please send us your comments and keep in mind:'.
					      '<br/>- the code is NOT guaranteed to be bug-free'.
					      '<br/>- be aware that all your changes will be removed during the next data '.
					      'synchronization with the real system.'),
					(cnf_demo ? 'Demo version' : 'Developer sandbox'));
  // Now myPear is ready
  myPear::$isReady = True;
  bTiming()->cpu();

  // Now we can do everything what was postponed before...
  // Nice moment...
  myPear_startup_queue();

  // Start authentication
  bAuth();

  /*
  // To be replaced my bTaskManager...
  // Mails should be sent asynchronioucly...
  locateAndInclude('bMailer');
  myPear_taskManager_queue();
  */
  b_debug::_debug(exiting);
}
