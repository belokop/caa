<?php

/**
 * @file
 * Contains Drupal\theme_switcher\components\ThemeSwitcher
 */
 
namespace Drupal\theme_switcher\components;
 
use Drupal\Core\Routing\RouteMatchInterface;
use Drupal\Core\Theme\ThemeNegotiatorInterface;
 
class ThemeSwitcher implements ThemeNegotiatorInterface {
  
  // domain -> theme matching
  public  $themes = array('albanova.se' => 'an',
			  'nordita.org' => 'nordita');
  // Admin theme
  public $admin_theme = 'seven';

  private $theme = '';

  public function __construct(){}
  
  public function applies(RouteMatchInterface $route_match) {

    // list of available themes
    $valid_themes = array_keys(array_filter(\Drupal::service('theme_handler')->listInfo(), 
					    function ($theme) {return $theme->status;}));
      

    if (\Drupal::service('router.admin_context')->isAdminRoute()){
      if (in_array($this->admin_theme,$valid_themes)) $this->theme = $this->admin_theme;
    }else{    
      // Check first 'org' & 'flavor' arguments, save the value in the session cache
      foreach(array(@$_REQUEST['org'],
		    @$_REQUEST['flavor'],
		    @$_SESSION[__METHOD__]) as $theme){
	if (in_array($theme,$valid_themes)){
	  if (empty($this->theme))	$this->theme = $theme;
	  $_SESSION[__METHOD__] = $this->theme;
	}
      }
      
      // Check then the http server name
      foreach($this->themes as $url=>$theme){
	if (in_array($theme,$valid_themes) && (stripos($_SERVER['HTTP_HOST'],$url) !== False)){
	  if (empty($this->theme)) $this->theme = $theme;
	  foreach(['flavor','org'] as $arg){
	    if (class_exists('b_cnf',False) && !\b_cnf::get($arg)) \b_cnf::set($arg,$theme);
	  }
	}
      }
    }
    return !empty($this->theme);
  }
  
  /**
   * {@inheritdoc}
   */
  public function determineActiveTheme(RouteMatchInterface $route_match) {
    $reply = $this->theme;
    return $reply;
  }
}
