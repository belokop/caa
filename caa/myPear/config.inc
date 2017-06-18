<?php
ini_set("allow_url_fopen", true);
//var_dump(file_get_contents('php://input'));

// myPear module 
$releaseDate = '2017-06-18';
define('myPear_VERSION','5.3.201');
define('myPear_MODULE', 'myPear');
define('myPear_DATE',    $releaseDate);

// Protection against attacks...
if (isset($_GET['id']) && !in_array($_GET['id'],array('new','empty'))) $_REQUEST['id'] = $_GET['id'] = (string)((int)$_GET['id']);

define('cnf_dev_email','yb@nordita.org');
define('cnf_test_email','test@nordita.org');

// Getting strange problems with deadlocks...
// Let's for a while not use "auto-commit", i.e. cancel transactions. 
define('DB_autocommit',False);

///
// Avoid problems with the locales,
// Define monetary & language constants
// see http://php.net/manual/en/function.setlocale.php
///
if (function_exists("date_default_timezone_set") and 
    function_exists("date_default_timezone_get"))  @date_default_timezone_set(@date_default_timezone_get());

define('CONST_localeBase','en_US');
define('CONST_currency', ' SEK');
setlocale(LC_ALL, CONST_localeBase . '.UTF8');
setlocale(LC_MONETARY, 'en_SE');

///
// Load private information - database name / smtp host / passwords / etc.
///
if (file_exists($config_file = dirname(__FILE__).'/config.priv.inc')){
  require $config_file;
  if (!empty($GLOBALS['bMailer_noreply'])) define('bMailer_noreply',$GLOBALS['bMailer_noreply']);
}
if (!defined('bMailer_noreply')) die ("<h3>Missing OR incomplete configuration file <br/><code>$config_file</code>, <br/>please create it using as example <br/><code>".
				      str_replace('.inc','_example.inc',$config_file)."</code></h3>");


// Optional "Employees & Alumni" module
define('EA_MODULE', 'ea');

/*
 * Shortcuts to the EA statuses.
 * Not to be edited.
 * Philosophy:
 * - a person in EA module is might be 
 *   = EA_role_employee (members of the personnel)
 *   = EA_role_alumnus (alumnus)
 * - employee is a member of an "employee type" group, se bList_orgEmpTypes class, which is normally
 *   = Administration (optional)
 *   = Visitors (normally long term ones, optional)
 *   = Other user defined groups like "faculty", "students", etc.
 */
define('EA_role_alumnus' ,   'a');
define('EA_role_employee',   'e');
define('EA_role_lt_visitor', 'v');

$GLOBALS['imapServers'] = array('kth.se'     => array('imap'=>array('ssl://webmail.kth.se','webmail.kth.se'),
						      'port'=>993,
						      'alias'=>array('nordita.org','albanova.se')),
				'mbox.su.se' => array('imap'=>array('ssl://mbox.su.se','mbox.su.se'),
						      'port'=>993,
						      'alias'=>array('su.se','nordita.org','physto.se','albanova.se','fysik.su.se')));

//
// myPear recognizers 3 levels of the software robustness:
//  * "cnf_dev" development area, often running from the "localhost".
//           Activated either
//              - explicitly, by setting $_GET[dev]=1 (or "&dev=1" in the URL),
//                $_GET[dev]=0 terminates the development mode
//              - implicitly, True if the http server is localhost
//           When myPear is in "cnf_dev" mode:
//              - All php errors/warnings/notices are reported
//              - All e-mails are redirected to the developer e-mail address, defined by "cnf_dev_email"
//              - Debug printout of the critical algorithms is ON
//              - The user passwords are (almost) ignored, so the developer can login as any user
//  * "cnf_demo" demo / test area
//           Activated either
//              - explicitly by setting $_GET[demo]=1 (or "&demo=1" in the URL)
//              - implicitly if the http server name contains 'test'
//              - All e-mails are redirected to the tester e-mail address, defined by "cnf_demo_email"
//           When myPear is in "cnf_dev" mode:
//              - All e-mails are redirected to the person logged-in
//  * production area
//              - default
// 

// set flag "invoked as a script by the CLI interpreter, no httpd server"
if (!defined('cnf_CLI'))       define('cnf_CLI',empty($_SERVER["HTTP_USER_AGENT"]));

//  set flag "running locally, without a network on a Ryan-air jet"
if (!defined('cnf_localhost')) define('cnf_localhost',
				      (bool)(preg_match('/^((.*)?::1$|127.0.0.1|local)/',trim(@$_SERVER["REMOTE_ADDR"])) &&
					     strpos(__FILE__,'/trunk/')!==False));

// Set a shortcut "Processing task manager queue", see class "myPear_taskManager"
define('cnf_myPear_taskManager',0 && isset($_REQUEST['myPear_taskManager']));


// ------ cnf_demo
//            - The user passwords are (almost) ignored, so the developer can login as any user
 
// define name of the block in $_SESSION array to keep options
if (!defined('CONST_cnfOptions')) define('CONST_cnfOptions' , 'myPear_options');

// Define the cache name
if (!defined('myPear_cache'))     define('myPear_cache','myPear_cache');

// Set the session life time to 8 hours
define('myPear_autoLogoutTime',8 * 3600);

// if True, use a trick to replace "function header()" by a javascript
define('cnf_useTrickyHeader',!cnf_localhost);

// If cnf_dontSendMails_Point is set, NO e-mail is sent disregarding the other settings
define('cnf_dontSendMails_Point',False);

/*
 * Access level to the EA record is either public OR private.
 * In any organization there administrators
 */
define('EA_acl_public', 'p');
define('EA_group_adm','adm');

/*
 * The query string is normally encoded, decode it.
 * (See also b_crypt::encodeURL())
 */
define('b_crypt_key','y');
define('b_crypt_z'  ,'z');
define('b_crypt_no' ,'nocrypt_once');

//
// Developer debugging
//
define('starting','Тронули, барин...');   // Means "Let's go, sir..."
define('exiting', 'Приехали, барин...');  // Means "Have arrived, sir..."
define('missing_value', 'missing value');

if (!isset($_GET['showe'])) { $_REQUEST['showe'] = $_GET['showe'] = 1; define('cnf_show_e',True); }

// $_GET['debug'] = $_GET['nodrupal'] = $_GET['showl'] = 1; define('cnf_show_l',True);
// $_GET['authdebug'] = $_GET['warnings'] = $_GET['debug'] = $_GET['showi'] = $_GET['nodrupal'] = $_GET['YBhere'] = $_GET['dev'] = 0;
if (cnf_localhost && !cnf_myPear_taskManager){
  error_reporting(E_ALL); 
  if (!cnf_CLI) foreach(array(
			      'warnings', 'showdb', 'showd', 'timing', 
			      // 'showpv','showgv',
			      // 'showreg', // registry
			      // 'showl',     
			      // 'showi', // invocation
			      // 'showv', // visibility
			      // 'authdebug','showa',
			      // 'debug',
			      // 'nodrupal',        
			      ) as $d)  $_GET[$d] = 1;
}

/*
 * Define the basic built-in Employers & Alumni roles in the organization.
 * - "EA_title_employee" is automatically (by a cron job or an explicit call to "rebuild EA") set if the current date is within the employment period,
 *                       and is automatically replaced by "EA_title_alumnus" when the employment period expired
  *
 * See also classes bForm_Avatar_ea & bUnit_ea_members
 */
define('EA_title_alumnus' ,   'Alumnus');
define('EA_title_employee',   'Employee');
define('EA_title_lt_visitor', 'Visitor');

/*
 * The default size of the portrait photos (in pixels) 
 * Might be redefined on the fly by "&portraitPhoto_size=NEW_VALUE"
 */
define('portraitPhoto_size_default',128);

/*
 * Date in the "near future".
 * The dates after this will be shown as "YEAR (expected)"
 * The default 99 days from now
 */
define('CONST_near_future', time() + 99 * 24 * 3600); 

/* date format for the datepicker */
define('CONST_dp_format' , 'yy-mm-dd');


/*
 * There is nothing to change below this line 
 * ******************************************
 */

/*
 * Define the ranks, the basis for the Access Control.
 * By default the system knows about the superuser, secretary, administrator and authenticated person.
 * The extra roles/ranks are might be defined by APIs
 */
define('RANK__superuser'     , 99);
define('RANK__ea_org_manager',  8);
define('RANK__ea_admin'      ,  6); // Employees & Alumni
define('RANK__ea_member'     ,  4); // Employees & Alumni
define('RANK__authenticated' ,  2);
define('RANK__ea_contactPerson',1);
define('RANK__anonymous'     ,  0);

/*
 * Define the check-in / checkout time 
 */
define('CONST_TIME'    , '12:00:00');               // noon
define('CONST_TIME_CI' , '15:00:00');               // Accommodation check-in (arrival time in general)
define('CONST_TIME_CO' , '10:00:00');               // Accommodation check-out (departure time in general)
define('CONST_TIME_OAS', '08:00:00');               // Office hours start
define('CONST_TIME_OAE', '19:00:00');               // Office hours end
define('CONST_ical_cells_to_show',56);              // Default number of cells (usually 1cell == 1day) to show in calendar graphs
define('CONST_wic', 'wic');                         // Who Is Connected

/*
 * A date somewhere far away... 
 * Note, on systems where time_t is a 32bit signed integer, as most common today, 
 * the valid range for year is somewhere between 1901 and 2038
 */
define('CONST_eternity_year' , 2037);
define('CONST_eternity' , CONST_eternity_year.'-01-01');
define('CONST_eternity_t',mktime(12,0,0,1,1,CONST_eternity_year));

/* As is  */
define('CONST_year_not_considered' , -2147483648);

define('CONST_noname', 'none');
define('CONST_go_back','go back');
define('CONST_not_concerned','not concerned');

define('b_regsplit','/[ ,;]+/');
define('cnf_emptyString','<empty>');
define('b_emptyHTML','<html><body bgcolor="#FFFFFF"></body></html>');

define('MSG_NYI',        'Sorry, this function is not yet implemented...');
define('NO_ACCESS',      'Sorry, you are not authorized to access this resource');
define('EMPTY_SELECTION','At the time being there is no entries to show');
define('employment_record','Employment record');

/*
 * Locate the running environment - am i a CMS module?
 */

if (cnf_CLI){
  $HOSTED_BY_CMS ='CLI';
}elseif (defined('DRUPAL_CORE_COMPATIBILITY')){
  $HOSTED_BY_CMS = 'drupal';
  list($HOSTED_BY_CMS_vrsn,) = explode('.',DRUPAL_CORE_COMPATIBILITY,2);
}elseif (class_exists('Drupal',False)){
  $HOSTED_BY_CMS = 'drupal';
  if (defined('CORE_COMPATIBILITY')) list($HOSTED_BY_CMS_vrsn,) = explode('.',CORE_COMPATIBILITY,2);
  else                                    $HOSTED_BY_CMS_vrsn = 8;
}elseif (defined('_JEXEC')){
  $HOSTED_BY_CMS = 'joomla';
}elseif (False){
  $HOSTED_BY_CMS = 'wp';
}else{
  $HOSTED_BY_CMS = False;
}
if (empty($HOSTED_BY_CMS_vrsn)) $HOSTED_BY_CMS_vrsn = 1;
define('HOSTED_BY_CMS', $HOSTED_BY_CMS);
define('HOSTED_BY_CMS_vrsn',$HOSTED_BY_CMS_vrsn);

/*
 * Continue to locate the running environment - 
 * are we inside the modal window? (fb == fancybox).
 * Note, the "myPear_taskManager" also prefers to run as a modal window, without the html header/footer
 *
 * Unfortunately i can't find a reliable way to detect that we are in the 'iframe' modal window,                                                                                             
 * while for 'ajax' this is possible                                                                                                                                                          
 */                                                                                                                                                                                          
if (!defined('cnf_inside_fb')){
    define('cnf_inside_fb_ajax', isset($_SERVER['HTTP_X_REQUESTED_WITH']) &&
           strToLower($_SERVER['HTTP_X_REQUESTED_WITH']) == 'xmlhttprequest');
    define('cnf_inside_fb_iframe',cnf_myPear_taskManager || (@$_GET['modal_once'] === 'iframe'));
    define('cnf_inside_fb',  cnf_inside_fb_iframe || cnf_inside_fb_ajax);
    if (cnf_inside_fb_iframe) $_GET['modal_once'] = 'iframe';
}


/*
 * Initialize various file repositories,
 * those must exist and/or be writable by the web server
 */
define('DOCUMENT_ROOT', getcwd().DIRECTORY_SEPARATOR);

switch(HOSTED_BY_CMS){
case False:
case 'CLI':
  if (!defined('myPear_root_files')) define('myPear_root_files', '/tmp/myPear_files');
  if (!is_dir(myPear_root_files)) mkdir(myPear_root_files);
  break;
  
case 'drupal': 

  require_once drupal_get_path('module',myPear_MODULE).'/includes/drupal8_compat.inc';

  // Say "here is drupal" (DRUPAL_ROOT is not defined in some drupal versions)
  if (!defined('DRUPAL_ROOT'))        define('DRUPAL_ROOT', DOCUMENT_ROOT);
  
  // Hook myPear files to the standard drupal files area
  if (!function_exists('my_drupal_get_path')){
    function my_drupal_get_path($type,$name=''){
      if ($name) $p = drupal_get_path($type,$name);
      else       $p = $type;
      if (strpos($p,DIRECTORY_SEPARATOR) !== 0) $p = DIRECTORY_SEPARATOR . $p;
      if (!get_clean_url()) $p = '.' . $p;
      return $p;
    }
  }
  // Define the root directory for the myPear distribution
  define('myPear_root',       my_drupal_get_path('module',myPear_MODULE).DIRECTORY_SEPARATOR);
  define('myPear_root_files', my_drupal_get_path('/sites/default/files'));
  break;
  
case 'joomla':
  define('myPear_root_files', 'files'); 
  define('myPear_images','components/com_vm/myPear/images/');
  
  // suppress Joomla adverts (should be done in a better way)
  if (class_exists('JFactory',False)){
    $db = &JFactory::getDBO();
    $db->setQuery("UPDATE #__modules SET published=0 WHERE position='right'");
    $db->query();
  }else{
    print "Shit... JFactory";
  }
  break;
  
case 'wp':
    break;
}

if ($cwd = str_replace(DOCUMENT_ROOT.DIRECTORY_SEPARATOR,'',dirname(__FILE__).DIRECTORY_SEPARATOR)) $cwd .= DIRECTORY_SEPARATOR;
if (!defined('myPear_root'))   define('myPear_root',  $cwd);
if (!defined('myPear_root_files'))   define('myPear_root_files', myPear_root.'../_tmp');

/*
 * The complete list of the repositories
 */
define('myPear_js',       join(DIRECTORY_SEPARATOR,array(myPear_root,'js','')));
define('myPear_css',      join(DIRECTORY_SEPARATOR,array(myPear_root,'css','')));
define('myPear_bin',      join(DIRECTORY_SEPARATOR,array(myPear_root,'bin','')));
define('myPear_images',   join(DIRECTORY_SEPARATOR,array(myPear_root,'images','')));
define('myPear_includes', join(DIRECTORY_SEPARATOR,array(myPear_root,'includes','')));
define('myPear_tmp',      join(DIRECTORY_SEPARATOR,array(myPear_root_files,myPear_MODULE,'tmp','')));
define('myPear_photos',   join(DIRECTORY_SEPARATOR,array(myPear_root_files,myPear_MODULE,'photos','')));
define('myPear_languages',join(DIRECTORY_SEPARATOR,array(myPear_root_files,myPear_MODULE,'languages','')));

//
// Load the basics
require_once (dirname(__FILE__).DIRECTORY_SEPARATOR.'includes'.DIRECTORY_SEPARATOR.'myPear.inc');
require_once (dirname(__FILE__).DIRECTORY_SEPARATOR.'includes'.DIRECTORY_SEPARATOR.'myPear_base.inc');
require_once (dirname(__FILE__).DIRECTORY_SEPARATOR.'includes'.DIRECTORY_SEPARATOR.'myPear_classes.inc');
require_once (dirname(__FILE__).DIRECTORY_SEPARATOR.'includes'.DIRECTORY_SEPARATOR.'myPear_functions.inc');

/*
 * Start session and initialise remaining constants
 */
myPear_sessionStart();
myPear_setConstants();
       
/*
 * Define the myPear common roles.
 *  i     - role favicon
 *  d     - role description for the role favicon
 *  class - (optional) bUnit class which handles this unit.
 *          If present, a bUnit class is created for this role  
 */
myPear_set_roles('roles',
                 array(myPear_MODULE=>array(RANK__superuser    => array('class' => 'bUnit',
                                                                        'i'=>'bb-zoo',
                                                                        'd'=>'SuperUser'),
                                            RANK__authenticated=> array('i'=>'user-small',
                                                                        'd'=>'Authenticated'),
                                            RANK__anonymous    => array('i'=>'user-silhouette',
                                                                        'd'=>'Anonymous')),
                       EA_MODULE    =>array(RANK__ea_org_manager   => array('class' => 'bUnit',
                                                                        'i'=>'bb-phone',
                                                                        'd'=>'Organization Manager'),
                                            RANK__ea_admin     => array('class' =>'bUnit_ea_admins',
                                                                        'i'=>'bb-man_folder',
                                                                        'd'=>'E&A administrator'),
                                            RANK__ea_member    => array('class' =>'bUnit_ea_members',
                                                                        'i'=> 'bb-man',
                                                                        'd'=> 'Staff member'),
                                            RANK__ea_contactPerson=> array('class' =>'bUnit_ea_contacts',
                                                                           'i'=> 'bb-man',
                                                                           'd'=> 'Contact person'),
                                            
                                            )));


/*
 * Affiliation with the organization
 */
myPear_set_roles('affiliation',          
                 array(EA_role_employee    => array('title' => EA_title_employee,
                                                    'group' => 'Staff',
                                                    'mode'  => 'RO',        // Always present 
                                                    'page'  => 1,           // Starts web page
                                                    'active'=> 1),          // active by default
                       EA_role_alumnus     => array('title' => EA_title_alumnus,
                                                    'group' => 'Alumni',
                                                    'page'  => 1,           // Starts web page
                                                    'active'=> 1),          // active by default
		       EA_role_lt_visitor  => array('title' => EA_title_lt_visitor,
                                                    'group' => 'Visitors',
                                                    'active'=> 0),          // NOT active by default
		       ));


/*
 * Load the core libraries
 */
myPear_register_config(__FILE__);
start_myPear();

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
      var_dump($path);
      if (class_exists('b_debug',0)) b_debug::internalError("?????? Can't load file \"$object\"");
      debug_print_backtrace();
      die("?????? Can't locate \"$object\"");
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
  return $reply;
}

/*
 *
 */
function locateAndLoad($class,$ref_file=__FILE__){
  if (class_exists($class,False)) return True;
  $dirname = dirname($ref_file);
  foreach(array('','/includes') as $d){
    foreach(array(str_replace('_','/',$class),$class) as $c){
      foreach(array('.inc','') as $p){
	$f = $dirname . "$d/$c$p";
	print"$f<br>";
	if (file_exists($f)){
	  require_once $f;
	  if (defined('cnf_dev') && cnf_dev && class_exists('b_debug',False)) b_debug::xxx('OK '.basename($f));
	  return True;
	} 
      } 
    }
  }
  if (defined('cnf_dev') && cnf_dev && class_exists('b_debug',False)) b_debug::xxx('??? failed');
  return False;
}


/*
 * Resolving clashes in the startup mess, when some modules need information which is not yet available.
 */
function myPear_startup_queue($function=Null,$args=array(),$condition=Null){

  if (!is_array($args)) $args = array($args);
  if (empty($args)) $args = array();
  if (empty($GLOBALS[__FUNCTION__])) $GLOBALS[__FUNCTION__] = array();

  // Execution request    
  if (empty($function)){
    // Execute the collected functions, then empty the queue
    foreach($GLOBALS[__FUNCTION__] as $entries){
      foreach($entries as $function=>$args){
	b_debug::xxx("Processing $function(".join(',',$args).")");
	call_user_func_array($function,$args);
      }
    }
    unset($GLOBALS[__FUNCTION__]);
  }else{
    // Check the queuing condition, execute immediately if the condition is false
    if ($condition === Null) $condition = (!myPear::$isReady);
    if(!$condition){
      b_debug::xxx("executing without queuing...");
      call_user_func_array($function,$args);
      return;
    }
    // Otherwise put put the object to the queue
    $GLOBALS[__FUNCTION__][] = array($function => $args);
    if (class_exists('b_debug',False)) b_debug::xxx("adding to the startup queue");
  }
}


/*
 * Create the path for the source files
 */
function myPear_register_config($config_file){

  // The files are not changed during the session...
  if (empty($_SESSION[__FUNCTION__]['C'])) $_SESSION[__FUNCTION__]['C'] = array();
  if (!in_array(md5($config_file),$_SESSION[__FUNCTION__]['C'])){
    
    // Get the initial path when there is no modules loaded yet
    $PATH = @$_SESSION[__FUNCTION__]['P'];
    if (empty($PATH)){ 
      $PATH = array();
      foreach(explode(PATH_SEPARATOR,get_include_path()) as $p){
	$PATH[] = $p; 
	if ((strpos($p,DIRECTORY_SEPARATOR) !== False) && is_dir($pp=$p.DIRECTORY_SEPARATOR."PEAR"))  $PATH[] = $pp;
      }
      $_SESSION[__FUNCTION__]['I'] = join(PATH_SEPARATOR,array_unique($PATH));
    }
    
    // Parse the config file path into $PATH
    myPear_register_config_parser($config_file,$PATH);
    
    // Add the path to all known modules when the database becomes available
    // and save the path for the session. It is not large...
    if (class_exists('b_reg',0)){
      myPear_register_config_parser(b_reg::get_modules(True),$PATH);
    }
    $_SESSION[__FUNCTION__]['C'][] = md5($config_file);
    $_SESSION[__FUNCTION__]['P'] = $PATH;
  }
  
  static $dejaVu = 0;
  if (!$dejaVu++) {
    set_include_path($_SESSION[__FUNCTION__]['I']);                  // Works as of PHP 4.3.0
    ini_set('include_path',$_SESSION[__FUNCTION__]['I']);            // Works in all PHP versions
  }
  
  // give back the reply
  return $_SESSION[__FUNCTION__]['P'];
}
