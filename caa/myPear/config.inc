<?php
       
ini_set("allow_url_fopen", true);
//var_dump(file_get_contents('php://input'));

// myPear module 
$releaseDate = '2020-03-19';
define('myPear_VERSION','5.16');
define('myPear_MODULE', 'myPear');
define('myPear_DATE',    $releaseDate);

// Protection against attacks...
if (isset($_GET['id']) && !in_array($_GET['id'],array('new','empty'))) $_REQUEST['id'] = $_GET['id'] = (string)((int)$_GET['id']);

// Do use collation in "ORDER BY" close (does not work if the database is not created carefully :-)
define('db_force_collation',False);

// Do not use CMS (drupal) authentication
define('use_CMS_authentication',False);

/*
 * Initialize various file repositories,
 * those must exist and/or be writable by the web server
 */
define('DOCUMENT_ROOT', getcwd().DIRECTORY_SEPARATOR);
define('DOCUMENT_ROOT_DEBUG', dirname(__FILE__));

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
					     (strpos(__FILE__,'/trunk/') !==False or
					      strpos(__FILE__,'/github/')!==False)));

// Set a shortcut "Processing task manager queue", see class "myPear_taskManager"
define('cnf_myPear_taskManager',0 && isset($_REQUEST['myPear_taskManager']));


// ------ cnf_demo
//            - The user passwords are (almost) ignored, so the developer can login as any user
 
if (!defined('CONST_cnfOptions'))        define('CONST_cnfOptions' , 'myPear_options');

// If cnf_dontSendMails_Point is set, NO e-mail is sent disregarding the other settings
if (!defined('cnf_dontSendMails_Point')) define('cnf_dontSendMails_Point',False);

// Define the cache name
if (!defined('myPear_cache'))     define('myPear_cache','myPear_cache');

// if True, use a trick to replace "function header()" by a javascript
define('cnf_useTrickyHeader',!cnf_localhost);

// Set the session life time to 8 hours
define('myPear_autoLogoutTime',8 * 3600);

// To use or not to use the unit "contacts"
define('cnf_useContactsUnit',False);


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
 * default ranks
 */
define('RANK__superuser'     , 99);
define('RANK__authenticated' ,  2);
define('RANK__anonymous'     ,  0);


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

define('NO_ACCESS',      'Sorry, you are not authorized to access this resource');
define('EMPTY_SELECTION','At the time being there is no entries to show');

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

/*
 * Continue to locate the running environment - 
 * are we inside the modal window? (fb == fancybox).
 * Note, the "myPear_taskManager" also prefers to run as a modal window, without the html header/footer
 *
 * Unfortunately i can't find a reliable way to detect that we are in the 'iframe' modal window,
 * while for 'ajax' this is possible
 */
define('fancybox3',False);
if (!defined('cnf_inside_fb')){
    define('cnf_inside_fb_ajax', isset($_SERVER['HTTP_X_REQUESTED_WITH']) &&
           strToLower($_SERVER['HTTP_X_REQUESTED_WITH']) == 'xmlhttprequest');
    define('cnf_inside_fb_iframe',cnf_myPear_taskManager || (@$_GET['modal_once'] === 'iframe'));
    define('cnf_inside_fb',  cnf_inside_fb_iframe || cnf_inside_fb_ajax);
    if (cnf_inside_fb_iframe) $_GET['modal_once'] = 'iframe';
}


// Add "slash" at the end of the path
if ($cwd = str_replace(DOCUMENT_ROOT.DIRECTORY_SEPARATOR,'',dirname(__FILE__).DIRECTORY_SEPARATOR)) $cwd .= DIRECTORY_SEPARATOR;
if (!defined('myPear_root'))         define('myPear_root', tempnam(sys_get_temp_dir(), 'myPear_root'));
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

//
// Load the basics
foreach(array('myPear',
	      'myPear_base',
	      'myPear_classes',
	      'myPear_functions',
	      ) as $m){
  $include = join(DIRECTORY_SEPARATOR,array(dirname(__FILE__),'includes',$m.'.inc'));
  require_once ($include);
}
//b_debug::_debug(sprintf("fancybox3='%s', cnf_inside_fb='%s'",var_export(fancybox3,True),var_export(cnf_inside_fb,True)),cnf_dev);

/*
 * Start session and initialise remaining constants
 */
myPear_sessionStart();
myPear_setConstants();

/* 
 * ================================= Settings specific for the "caa" package 
 */
$config_custom = join(DIRECTORY_SEPARATOR,array(dirname(__FILE__),'config_custom.inc'));
if (file_exists($config_custom)) require_once $config_custom;


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
		       ));


/*
 * Load the core libraries
 */
myPear_register_config(__FILE__);
start_myPear();

