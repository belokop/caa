<?php

/**
 * @file
 * Contains \Drupal\myPear\components\AccessController.
 */

namespace Drupal\myPear\components;

use Drupal\Core\Access\AccessResult;
use Drupal\Core\Routing\Access\AccessInterface;
use Symfony\Component\Routing\Route;

define('D8MENU_DYNAMIC_ROUTES', False);
define('D8MENU_CSS_HIDING_LINKS', False);
define('D8MENU_DISABLING_MODULES', True);
define('D8MENU_DISABLING_MODULES_all_links',D8MENU_DISABLING_MODULES);
define('D8MENU_ALL_VISIBLE',!D8MENU_CSS_HIDING_LINKS && !D8MENU_DISABLING_MODULES);

if (D8MENU_CSS_HIDING_LINKS && D8MENU_DISABLING_MODULES) die("Both D8MENU_CSS_HIDING_LINKS && D8MENU_DISABLING_MODULES are enabled\n");

/*
 *
 */
function AC(){
  static $AccessController = Null;
  if (empty($AccessController)){
    $AccessController = new AccessController();
  }
  return $AccessController;
}

class AccessController implements AccessInterface{ 

  /*
   * Activate the current module menu, deactivate the others
   */
  public function __construct($EntityManager=Null) {
    static $dejaVu = 0;
    if (!$dejaVu++){
      if (D8MENU_CSS_HIDING_LINKS)      $this->dbg('D8MENU_CSS_HIDING_LINKS');
      if (D8MENU_ALL_VISIBLE)           $this->dbg('D8MENU_ALL_VISIBLE');
      if (D8MENU_DYNAMIC_ROUTES)        $this->dbg('D8MENU_DYNAMIC_ROUTES');
      if (D8MENU_DISABLING_MODULES)     $this->dbg('D8MENU_DISABLING_MODULES');
      if (D8MENU_DISABLING_MODULES_all_links) $this->dbg('D8MENU_DISABLING_MODULES_all_links');
      
      myPear_init();
      \d8::current_tab();    
      if (!defined('cnf_CLI'))  define('cnf_CLI',empty($_SERVER["HTTP_USER_AGENT"]));
      if (!cnf_CLI && !\Drupal::service('router.admin_context')->isAdminRoute()){
	// Build the top menu
	MH()->toggle_menu_items(\b_reg::get_modules());
      }
    }
  }
  
  /*
   * Returns the PAGE access permission
   */
  public function access(Route $route){
    
    if ((D8MENU_CSS_HIDING_LINKS || D8MENU_ALL_VISIBLE) && in_array(($module=str_replace('/','',$route->getPath())),module_list())){
      static $dejaVu = 0;
      if (!$dejaVu++){
	myPear_init();
	bAuth();
	myOrg();
	locateAndInclude('APImenu');    
      }
      $reply = \APImenu::_used_by_myOrg($module);
      if (D8MENU_CSS_HIDING_LINKS) $variant = 'D8MENU_CSS_HIDING_LINKS';
      if (D8MENU_ALL_VISIBLE)      $variant = 'D8MENU_ALL_VISIBLE';
    }else{
      
      $variant = (D8MENU_DISABLING_MODULES_all_links ? 'D8MENU_DISABLING_MODULES_all_links' : 'normal');
      
      $reply = $this->_access_exec($route->getPath());
      
      if ($reply || D8MENU_DISABLING_MODULES_all_links){
	// Update on the fly the menu database.
	// However, to my understanding that should be done by the Drupal core...
	MH()->toggle_enabled($this->menu_route,$reply);
	MH()->toggle_enabled('clean cache');
      }
      if (!$reply) \d8::dbg(sprintf("??? %s(%s) access is forbidden, who called me?",
				    $route->getPath(),$this->tab),'fuchsiaText');
    }
    $this->dbg(array('path'   => $route->getPath(),
		     'tab'    => $this->tab,
		     // 'variant'=> $variant,
		     'reply'  =>var_export($reply,True)));
    return ($reply ? AccessResult::allowed() : AccessResult::forbidden()); 
  }
  
  /*
   * Check access to the tab, the current tab is a default
   */
  private $menu_route = Null;
  public  $tab = '?';
  public function _access_exec($menu_route){

    if (empty($menu_route)){
      print_r(debug_backtrace(False,5));
      die();
    }
    if (!function_exists('myPear_init')) return True;
    
    \myPear_init();
    \d8::current_tab();    
    if (empty($GLOBALS['myPear_current_module'])){
      
      $reply = True;
      
    }else{

      $module = $this->route2module($menu_route);
      if ($this->main_menu_route){
	$reply = \APImenu::_used_by_myOrg($module);
      }elseif ($module == $GLOBALS['myPear_current_module']){
	$_module_access_callback = sprintf('_%s_access_callback',$module);
	$reply = call_user_func($_module_access_callback,$this->tab);
      }else{
	$reply = False;
      }
    }
    //    $this->dbg($reply);
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
    if (file_exists($f=drupal_get_path('module',$_module) . '/config.tabs.inc')) require_once $f;
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
      \d8::current_tab();    
  }

  /*
   * Drupal::service(plugin.manager.menu.link) class holder
   */
  private function menu_link_manager(){
    static $menu_link_manager = Null;
    if (empty($menu_link_manager)) $menu_link_manager = \Drupal::service('plugin.manager.menu.link');
    return $menu_link_manager;
  }

  /*
   * The main menu should not show modules which are not used by the current organization,
   * and the tools menu shows only the tree for the current module 
   */
  public function toggle_menu_items($module_list){
  
    if (D8MENU_CSS_HIDING_LINKS){
      // Do not deactivate items in the tools menu,
      // after activation the become "permanently collapsed" (bug in D8??)
      // Workaround - hiding the menu items, see my twig extesion class Drupal\myPear\components\myTwigExtension
      //
      $menus_to_preprocess = ['main']; 
    }else{
      // To my understanding that is the logical way to go. 
      $menus_to_preprocess = ['main','tools']; 
    }

    $modules_to_preprocess = array_intersect(module_list(),$module_list); sort($modules_to_preprocess);
    $this->dbg(['modules'=>$modules_to_preprocess]);
    
    bAuth();
    myOrg();
    
    foreach($menus_to_preprocess as $menu_name){
      foreach($modules_to_preprocess as $module){
	if ($enabled = \APImenu::_used_by_myOrg($module)){
	  if ($menu_name == 'tools'){
	    if     (D8MENU_CSS_HIDING_LINKS) {
	      $expanded = 'any';
	    }elseif(D8MENU_DISABLING_MODULES){
	      $enabled = ($module === $GLOBALS['myPear_current_module']);
	      $expanded = True;
	    }elseif(D8MENU_ALL_VISIBLE){
	      $expanded = 'any';
	    }else{
	      \myPear::internalError("Unexpected MENU_ options combination");
	    }
	  }	      
	}
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

    if (empty($route_name)){
      $this->dbg('???? empty route_name');
      return;
    }
    
    if (($route_name === 'clean cache') || ($route_name === 'cc')){
      if ($this->toggle_counter  || ($route_name === 'cc')){
        $this->toggle_counter = 0;
        foreach(array('menu','render') as $cache_name){
	  \Drupal::cache($cache_name)->deleteAll();
	  $this->dbg($cache_name);
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
	$this->dbg($dbg);

	$this->toggle_counter++;
	$link['enabled']  = $enabled  ? 1 : 0; 
	$link['expanded'] = $expanded ? 1 : 0; 
	$this->menu_link_manager()->updateDefinition($route_name, $link);
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


require_once drupal_get_path('module','myPear').'/includes/drupal8_compat.inc';
