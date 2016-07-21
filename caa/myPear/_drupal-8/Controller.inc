<?php

/**
 * @file
 * Contains \Drupal\myPear\Controller.
 */

namespace Drupal\myPear;

use Drupal\Core\Access\AccessResult;
use Drupal\Core\Session\AccountInterface;
use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Routing\Access\AccessInterface;
use Drupal\Core\Routing\RouteSubscriberBase;
use Symfony\Component\Routing\Route;
use Symfony\Component\Routing\RouteCollection;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;

/**
 * Displays content for our menu links.
 */
class Controller extends ControllerBase{
  
  public function __construct($EntityManager) {
    // $this->dbg();
    // if (!empty($EntityManager)) $this->dbg($EntityManager->getBundleInfo('menu_link_content'));
  }
  
  public function getPageTitle() {
    global $myPear_custom_title;
    //    $this->dbg();

    \d8::current_tab();    
    if (empty($GLOBALS['myPear_current_module'])) return t('?');

    // Be sure that the init is done
    \myPear_init();
    call_user_func($GLOBALS['myPear_current_module'] . '_init');

    // Get the page title from the module APImenu class
    $_module_title_callback = sprintf('_%s_title_callback',$GLOBALS['myPear_current_module']);
    if (!function_exists($_module_title_callback)) die("??? where is $_module_title_callback");
    $_module_title_callback_reply = call_user_func($_module_title_callback,$GLOBALS['myPear_current_tab'],Null,True);
    $reply = (empty($myPear_custom_title) 
              ? $_module_title_callback_reply
              : $myPear_custom_title);
    $debug[] = htmlspecialchars($reply);
    if ($reply == $myPear_custom_title) $debug[] = "($_module_title_callback gives \"".htmlspecialchars($_module_title_callback_reply)."\")";
    //    $this->dbg(join(' ',$debug));
    return  t($reply);
  }
  
  /*
   *
   */
  public function getPageContent() {
    //    $this->dbg();

    \d8::current_tab();    
    $callback = sprintf('_%s_output',$GLOBALS['myPear_current_module']);
    $content  = call_user_func($callback,$GLOBALS['myPear_current_tab']);

    return array('#type' => 'markup',
		 //'#markup' => $content,
		 '#markup' => \Drupal\Core\Render\Markup::create($content),
                 );
  }

  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}  


/*
 *
 */
class AccessController implements AccessInterface{ 

  // Symfony\Component\Routing\Route;
  public $route = Null;

  public function __construct($EntityManager) {
    \d8::current_tab();    
    //    if (!empty($EntityManager))      $this->dbg($EntityManager->getBundleInfo('menu_link_content'));
    if (0) $this->dbg(array('module'=>$GLOBALS['myPear_current_module'],
			    'tab'   =>$GLOBALS['myPear_current_tab']));
  }
  

  /*
   * Return "Access allowed"
   */
  public function true(Route $route){
    $this->dbg(True);
    return AccessResult::allowed();
  }

  /*
   * Returns the PAGE access permission
   */
  public function getPageAccess() {
    
    \d8::current_tab();    
    if ($reply = !empty($GLOBALS['myPear_current_tab'])){    
      $reply = self::_access_exec();
    }
    $this->dbg($reply);
    return ($reply ? AccessResult::allowed() : AccessResult::forbidden()); 
  }
  
  /*
   *  public function access(Route $route, Request $request, AccountInterface $account) {
   */
  public function access(Route $route){

    \d8::current_tab();    
    if ($reply = !empty($GLOBALS['myPear_current_tab'])){    
      $this->route = $route;
      $reply = self::_access_exec($route->getPath());
      menuHandler::_toggle_enabled(self::$menu_route,$reply,$reply);
      menuHandler::_toggle_enabled('clean cache');
      if (!$reply) \myPear::ERROR(sprintf("??? %s(%s) access forbidden, but the link is active",
					  $route->getPath(),self::$tab));
    }
    $this->dbg(array('path' => $route->getPath(),
		     'tab'  => self::$tab,
		     'reply'=>var_export($reply,True)));

    return ($reply ? AccessResult::allowed() : AccessResult::forbidden()); 
  }
  
  /*
   *
   */
  public function checkAccess(){
    $reply = self::_access_exec();
    $this->dbg($reply);
    return ($reply ? AccessResult::allowed() : AccessResult::forbidden()); 
  }
  
  /*
   * Check access to the tab, the current tab is a default
   */
  private static $menu_route = Null;
  public  static $tab = '?';
  public static function _access_exec($menu_route=''){

    if (!function_exists('myPear_init')) return Null;
    
    // Strip out the leading slash and add the module name to get the D8 route name from D8 path
    if (strpos($menu_route,'/') === 0){
      $q = explode('/',substr($menu_route,1));;
      $menu_route = $q[0] . '.' . join('_',$q);
      //      \D8::dbg($menu_route);
    }
    self::$menu_route = $menu_route;
    
    \myPear_init();
    \d8::current_tab();    
    bAuth();
    if (empty($GLOBALS['myPear_current_module'])){
      
      $reply = True;
      
    }else{
      if (empty($menu_route)){
	call_user_func($GLOBALS['myPear_current_module'] . '_init');
	call_user_func(strToUpper($GLOBALS['myPear_current_module'] . '_menu'));
	
	$q_exploded = explode('/',$_REQUEST['q']);
	$module = $GLOBALS['myPear_current_module'];
	self::$tab    = $GLOBALS['myPear_current_tab'];
	if (file_exists($f=drupal_get_path('module',$module) . '/config.tabs.inc')) require_once $f;
      }else{
	list($_module,$route) = explode('.',$menu_route,2);
	$q_exploded = explode('_',$route);
	$module = array_shift($q_exploded);
	if (file_exists($f=drupal_get_path('module',$module) . '/config.tabs.inc')) require_once $f;
	if (empty($q_exploded)) $q_exploded = array($module);
	$tab_code = array_pop($q_exploded);
	self::$tab = APItabs_code2tab($tab_code);
	if ($_module != $module) print "disaster !!!!";
      }

      /*
      // TEMPORARY
      static $tempory_fix = ['an'=>['hw','ea','wiw','myPear']];
      static $modules_ok = '?';
      if (class_exists('b_cnf',False)){
	if ($modules_ok === '?'){
	  $modules_ok = @$tempory_fix[\b_cnf::get('org')];
	  var_dump($modules_ok);
	}
	if ($modules_ok && !in_array($module,$modules_ok)) return False;
      }
      */

      if ($module == $GLOBALS['myPear_current_module']){
	if (empty(self::$tab)) self::$tab = $module;
	$_module_access_callback = sprintf('_%s_access_callback',$GLOBALS['myPear_current_module']);
	$reply = (bool)call_user_func($_module_access_callback,self::$tab);
      }else{
	$reply = False;
      }
    }
    //    if ($reply) 
   \D8::dbg(array('tab'=>self::$tab,'reply'=>$reply));
    return $reply;
  }
  
  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}



/**
 * Listens to the dynamic route events.
 */
class RouteSubscriber extends RouteSubscriberBase {

  /**
   * {@inheritdoc}
   */
  protected function alterRoutes(RouteCollection $collection) {

    $this->dbg();
    
    /*
    // As nodes are the primary type of content, the node listing should be
    // easily available. In order to do that, override admin/content to show
    // a node listing instead of the path's child links.
    $route = $collection->get('system.admin_content');
    */

    $route = $collection->get('ea.ea');
    if ($route) {
      $this->dbg(array('path'=>$route->getPath(),
		       ));
      /*
      $route->setDefaults(array(
				'_title' => 'Content',
				'_entity_list' => 'node',
				));
      $route->setRequirements(array('_permission' => 'access content overview',
				    ));
      */
    }
  }

  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}


/*
 * Doing the Drupal's work... ??????????????????????????????????????????????????
 */
class menuHandler{
  
  public static $menu_deph = 999;
  
  /*
   * On the fly disable the links which are not provided by the current myPear module,
   * and enable those which are accessible.
   * Apparently Drupal8 does not do this.
   * There are maybe other ways to to it, but let's try a simple approach...
   */
  function __construct(){
    self::_toggle_menu_items(\b_reg::get_modules());
  }
  
  public static function _toggle_menu_items($module_list,$menu_name='tools'){
    
    \d8::current_tab();    
    if (empty($GLOBALS['myPear_current_module'])) return;
    
    foreach($module_list as $module){
      $enable = ($module === $GLOBALS['myPear_current_module']);
      $message_printed = 0;
      foreach(self::get_all_route_names($module,$menu_name) as $route_name){
	if (strpos($route_name,'_') === False){
	  if (!$message_printed++) \D8::dbg($enable
					    ? "--------------------------------- $module - CURRENT module"
					    : "--------------------------------- $module - Disabling the top link");
	  self::_toggle_enabled($route_name,$enable,True,$menu_name);
	}elseif ($enable){
	  // Preprocess menu for the current module, enable/disable the relevant links 
	  // according to the access callback value
	  if (!$message_printed++) \D8::dbg("----------------------------------- $module - CURRENT module, checking access ");
	  $access = AccessController::_access_exec($route_name,False); 
	  // \D8::dbg(array('access'=>$access,'route'=>$route_name));
	  if ($access !== Null) self::_toggle_enabled($route_name,$access,'any',$menu_name);
	}
      }
    }
    \D8::dbg("-----------------------------------------------------------");
    menuHandler::_toggle_enabled('clean cache');
  }
  
  /*
   *
   */
  private static function get_all_route_names($module,$menu_name){
    $rows = db_query('SELECT * FROM {menu_tree} WHERE menu_name = :menu_name AND route_name REGEXP :route_name',
		     array(':menu_name' => $menu_name,
			   ':route_name'=> "$module.$module")
		     )->fetchAll();
    foreach ($rows as $row) $reply[] = $row->route_name;
    // \D8::dbg($reply,$this);
    return $reply;
  }
  
  /*
   * Enable/disable the menu item
   */
  private static $toggle_counter = 0;
  private static $clean_counter = 0;
  public static function _toggle_enabled($route_name,$enabled=False,$expanded=False,$menu_name='tools') {
    
    if ($route_name === 'clean cache'){
      if (self::$toggle_counter || $enabled===True){
	self::$toggle_counter = 0;
	self::$clean_counter++;
	foreach(array('menu',
		      'render',
		      ) as $cache_name){
	  $cache = \Drupal::cache($cache_name);
	  $cache->deleteAll();
	  \D8::dbg("cleaning cache '$cache_name-".self::$clean_counter."'");
	}
      }
    }else{
      $rows = db_query('SELECT * FROM {menu_tree} WHERE menu_name = :menu_name AND route_name = :route_name',
		       array(':menu_name' => $menu_name,
			     ':route_name'=> $route_name)
		       )->fetchAll();
      foreach ($rows as $row) {
	if (!$row->has_children  && ($expanded !== 'any')) $expanded = False;
	if (((bool)$row->enabled  !== (bool)$enabled) || 
	    ((bool)$row->expanded !== (bool)$expanded && $expanded !== 'any')){
	  db_query('UPDATE {menu_tree} SET enabled=:enabled, expanded=:expanded WHERE menu_name = :menu_name AND route_name = :route_name',
		   array(':enabled'   => (empty($enabled)  ? 0 : 1),
			 ':expanded'  => (empty($expanded) ? 0 : 1),
			 ':menu_name' => $menu_name,
			 ':route_name'=> $route_name));
	  self::$toggle_counter++;
	  \D8::dbg(array('tab'     => AccessController::$tab,
			 'enabled' =>var_export($enabled,True),
			 'expanded'=>var_export($expanded,True)));
	} 
      }
    }
  }
  
  /*
   *
   */
  private function dbg($reply=''){
    \D8::dbg((is_bool($reply) ? var_export($reply,True) : $reply));
  }
}


require_once drupal_get_path('module','myPear').'/includes/drupal8_compat.inc';

