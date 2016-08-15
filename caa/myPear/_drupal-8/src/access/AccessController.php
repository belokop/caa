<?php

/**
 * @file
 * Contains \Drupal\myPear\access\AccessController.
 */

namespace Drupal\myPear\access;

use Drupal\Core\Access\AccessResult;
use Drupal\Core\Routing\Access\AccessInterface;
use Symfony\Component\Routing\Route;


require_once drupal_get_path('module','myPear').'/includes/drupal8_compat.inc';

/*
 *
 */
function D8MENU_definitions(){

  define('no_changes_',True);
  
  // As is, no changes in the menu...
  define('D8MENU_VANILLA', False);
  
  // Seems that the both RouteSubscriber && RouteBuilder are called ONLY ONCE when the module is built, 
  // and not on the each page as desired
  define('D8MENU_DYNAMIC_ROUTES', False &&   !D8MENU_VANILLA);
  
  // This does not work, Twig caches the functions replies, and the first reply is always used
  define('D8MENU_CSS_HIDING_LINKS', False && !D8MENU_VANILLA);
  
  // Menu 'main':  enable level 1 links for all modules (except those which are private for other org)
  // menu 'tools': enable level 1 link for the module in active trail, disable level 1 links for the others
  define('D8MENU_DISABLING_MODULES', True);

  // In addition the above enable/disable the links in the "access check" method
  // Works on the development laptop, but not on the production site. Mystery... 
  define('D8MENU_DISABLING_MODULES_all_links',False && D8MENU_DISABLING_MODULES);

  // Same as D8MENU_DISABLING_MODULES + toggle the root link
  define('D8MENU_TOGGLE_ROOT_LINK',True && D8MENU_DISABLING_MODULES);
  
  if ((int)D8MENU_VANILLA +
      (int)D8MENU_TOGGLE_ROOT_LINK + 
      (int)D8MENU_DYNAMIC_ROUTES +
      (int)D8MENU_DISABLING_MODULES_all_links
      != 1) die("Two D8MENU_xxx options are active, please correct");
}

/*
 *
 */
function AC(){
  static $AccessController = Null;
  if (empty($AccessController)){
    $AccessController = new AccessController();
  }
  die("666\n");
  // Build the top menu
  if ($AC->is_ready && !D8MENU_VANILLA && !cnf_CLI && !\Drupal::service('router.admin_context')->isAdminRoute()){
    MH()->toggle_menu_items(\b_reg::get_modules());
  }
  return $AccessController;
}

class AccessController implements AccessInterface{ 
  /*
   * Activate the current module menu, deactivate the others
   */
  public $is_ready = False;
  public function __construct($EntityManager=Null) {
    //    if (!$this->is_ready) $this->init();
    $this->is_ready = True;
    //    \D8::traceBack();
  }

  /*
   *
   */
  private function init(){
    //    static $dejaVu = 0;    if ($dejaVu++) return;
    
    if (!defined('cnf_CLI'))  define('cnf_CLI',empty($_SERVER["HTTP_USER_AGENT"]));
    
    if (D8MENU_VANILLA)               $this->dbg('D8MENU_VANILLA');
    if (D8MENU_DYNAMIC_ROUTES)        $this->dbg('D8MENU_DYNAMIC_ROUTES');
    if (D8MENU_DISABLING_MODULES)     $this->dbg('D8MENU_DISABLING_MODULES');
    if (D8MENU_DISABLING_MODULES_all_links) $this->dbg('D8MENU_DISABLING_MODULES_all_links');

    // Load D8 compats
    \D8::current_tab();
    
    // Load myPear as a drupal module    
    myPear_init();

    // Start myPear
    bAuth();
    myOrg();

  }
  
  /*
   * Returns the page access permission
   */
  public function access(Route $route){
    
    $this->init();

    // Get the module & tab from the Route
    $module = $this->route2module($route->getPath());

    if (cnf_CLI){

      $reply = True;

    }elseif (($module === 'myPear') ||
	!\APImenu::_used_by_myOrg($module) || 
	!in_array($module,module_list())){ 

      // Unused or unknown module
      $reply = False;
      
    }else{
      
      // Module from myPear
      if ($this->main_menu_route){
	//
	// Main horizontal menu, all the modules to be shown (except the unused ones)
	//
	$reply = \APImenu::_used_by_myOrg($module); 

      }else{
	//
	// Side menu (tools), check the access
	//
	$_module_access_callback = sprintf('_%s_access_callback',$module);
	$reply = ($module == $GLOBALS['myPear_current_module']
		  ? call_user_func($_module_access_callback,$this->tab) // The active trail
		  : False);                                             // Non active trail
	
	if (D8MENU_TOGGLE_ROOT_LINK){
	  // Toggle the active link
	  if ($reply) MH()->toggle_enabled($this->menu_route,$reply);
	  // Toggle the root link
	  $root_link = MH()->get_all_route_names($module,'tools',True);
	  $this->dbg($root_link);
	  //	  var_dump(MH()->menu_link_manager()->getDefinition($root_link));
	  MH()->toggle_enabled($root_link,$reply);
	  MH()->toggle_enabled('clean cache');
	}
      }
    }
    
    if (D8MENU_DISABLING_MODULES_all_links){
      // Update on the fly the menu database.
      // However, to my understanding that should be done by the Drupal core...
      MH()->toggle_enabled($this->menu_route,$reply);
      MH()->toggle_enabled('clean cache');
    }
    if (!$reply) \D8::dbg(sprintf("??? %s(%s) access is forbidden, who calls me?",
				  $route->getPath(),$this->tab),'fuchsiaText');
    $this->dbg(array('path'   => $route->getPath(),
		     'tab'    => $this->tab,
		     'reply'  =>var_export($reply,True)));
    return ($reply ? AccessResult::allowed() : AccessResult::forbidden()); 
  }
  
  /*
   * Check access to the route
   */
  private $menu_route = Null;
  public  $tab = '?';
  private function _access_exec($menu_route){

    $this->init();

    if (empty($menu_route)){
      print_r(debug_backtrace(False,5));
      die();
    }

    $module = $this->route2module($menu_route);

    if (($module === 'myPear') ||
	!\APImenu::_used_by_myOrg($module) || 
	!in_array($module,module_list())){ 
      
      $reply = False;
      
    }else{


      // Main horizontal menu, all the modules to be shown
      if ($this->main_menu_route){
	$reply = \APImenu::_used_by_myOrg($module); 
      }
      // The active trail
      elseif ($module == $GLOBALS['myPear_current_module']){
	$_module_access_callback = sprintf('_%s_access_callback',$module);
	$reply = call_user_func($_module_access_callback,$this->tab);
      }	
      // Non active trail
      else{
	$reply = False;
      }
    }
    return $reply;
  }

  /*
   *
   */
  private $main_menu_route = Null;
  private function route2module($menu_route){
    
    // Strip out the leading&trailing slashes and add the module name to get the D8 route name from D8 path
    if (strpos($menu_route,'/') === 0){
      $q = explode('/',substr($menu_route,1));;
      if ($this->main_menu_route = ((($n=count($q)) > 0) && empty($q[$n-1])))  array_pop($q);
      $menu_route = $q[0] . '.' . join('_',$q);
    }
    $this->menu_route = $menu_route;
      
    list($_module,$route) = explode('.',$menu_route,2);
    $q_exploded = explode('_',$route);
    $__module = array_shift($q_exploded);
    if (empty($q_exploded)) $q_exploded = array($_module);
    $tab_code = array_pop($q_exploded);
    $this->tab = APItabs_code2tab($tab_code);
    if (empty($this->tab)) $this->tab = $_module;
    if ($_module != $__module) print "!!!disaster '$_module' != '$__module'";
    return $_module;
  }      
  
  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}

/*
 * Doing the Drupal's work... ??????????????????????????????????????????????????
 */
function MH(){
  static $menuHandler = Null;
  if (empty($menuHandler)) $menuHandler = new menuHandler();
  return $menuHandler;
}

class menuHandler{
    
  /*
   * On the fly disable the links which are not provided by the current myPear module,
   * and enable those which are accessible.
   * Apparently Drupal8 does not do this.
   * There are maybe other ways to to it, but let's try a simple approach...
   */
  function __construct(){
      \D8::current_tab();    
  }

  /*
   * Drupal::service(plugin.manager.menu.link) class holder
   */
  function menu_link_manager(){
    static $menu_link_manager = Null;
    if (empty($menu_link_manager)) $menu_link_manager = \Drupal::service('plugin.manager.menu.link');
    return $menu_link_manager;
  }

  /*
   * The main menu should not show modules which are not used by the current organization,
   * and the tools menu shows only the tree for the current module 
   */
  public function toggle_menu_items($module_list){
    $this->dbg();
  
    $menus_to_preprocess = ['main'=>'/','tools'=>'']; 
    $modules_to_preprocess = array_intersect(module_list(),$module_list); sort($modules_to_preprocess);
    $this->dbg(['modules'=>$modules_to_preprocess]);
    
    bAuth();
    myOrg();
    
    foreach($menus_to_preprocess as $menu_name=>$slach){
      foreach($modules_to_preprocess as $module){
	$expanded = 'any';
	$enabled = AC()->access(new Route("/$module$slach"));
	$route_name = $this->get_all_route_names($module,$menu_name,True);
	// $this->dbg(['access'=>(bool)$enabled,'route'=>$route_name]);
	$this->toggle_enabled($route_name,$enabled,$expanded);
      }
    }
    $this->toggle_enabled('clean cache');
  }
  
  /*    
    // Preprocess 'tools' menu for the current module, enable/disable the relevant links 
    // according to the access callback value
    if ($enabled && ($menu_name != 'main')){
      foreach($this->get_all_route_names($module,$menu_name) as $route_name){
	$access = AC()->_access_exec($route_name); 
	$this->toggle_enabled($route_name,$access,'any');
      }
    }
  */
  
  /*
   * Get all route names for the module. The module might be in several menus
   */
  function get_all_route_names($module,$menu_name,$only_the_root=False){
    $search_for = ($menu_name == 'main'
		   ? "${module}.${menu_name}"
		   : "${module}.${module}");
    $reply = preg_grep("/$search_for/",array_keys($this->menu_link_manager()->getDefinitions()));
    sort($reply); // to kill the association...

    //    $this->dbg(['search_for'=>$search_for,'reply'=>$reply]);
    return ($only_the_root
	    ? array_shift($reply)
	    : $reply);
  }
  
  /*
   * Enable/disable the menu item
   */
  private static $toggle_counter = 0;
  public function toggle_enabled($route_name,$enabled=False,$expanded='any'){

    if (cnf_CLI || empty($route_name)){
      if (!cnf_CLI) \D8::traceBack('???? empty route_name');
      return;
    }
    
    if (($route_name === 'clean cache') || ($route_name === 'cc')){
      $this->dbg();
      if (!D8MENU_VANILLA && !($this->toggle_counter || ($route_name === 'cc'))){
        $this->toggle_counter = 0;
        foreach(array('menu','render') as $cache_name){
	  \Drupal::cache($cache_name)->deleteAll();
	}
      }
    }else{
      // Compare the existing link with the input arguments, update if needed
      $link = $this->menu_link_manager()->getDefinition($route_name);
      if ($expanded === 'any') $expanded = $link['expanded'];
      if (($e=((bool)$link['enabled']  !== (bool)$enabled)) || 
	  ($x=((bool)$link['expanded'] !== (bool)$expanded))){

	// debugging block
	if ($e) $dbg['enabled'] = var_export((bool)$link['enabled'],True) .'->'.var_export((bool)$enabled,True);
	if ($x) $dbg['expanded']= var_export((bool)$link['expanded'],True).'->'.var_export((bool)$expanded,True);
	$dbg['menu'] = $link['menu_name'];
	if (D8MENU_VANILLA) $dbg['ignored'] = 'D8MENU_VANILLA';
	elseif (no_changes_)$dbg['ignored'] = 'no_changes_'; 
	$this->dbg($dbg);

	// Toggle the value
	if (!no_changes_ && !D8MENU_VANILLA){
	  $this->toggle_counter++;
	  $link['enabled']  = $enabled  ? 1 : 0; 
	  $link['expanded'] = $expanded ? 1 : 0; 
	  $this->menu_link_manager()->updateDefinition($route_name, $link);
	}
      }
    }
  }
  
  /*
   *
   */
  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}

D8MENU_definitions();
