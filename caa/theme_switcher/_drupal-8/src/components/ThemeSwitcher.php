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
  private $theme = '';

  public $user;
  public $configFactory;
  public $entityManager;
  public $adminContext;

  /*
  public function __construct(AccountInterface $user, ConfigFactoryInterface $config_factory, EntityManagerInterface $entity_manager, AdminContext $admin_context) {
    $this->user = $user;
    $this->configFactory = $config_factory;
    $this->entityManager = $entity_manager;
    $this->adminContext = $admin_context;
    \D8::dbg();
  }
  */
  
  public function applies(RouteMatchInterface $route_match) {

    
    // list of available themes
    $themes = array_keys(array_filter(\Drupal::service('theme_handler')->listInfo(), 
				      function ($theme) {return $theme->status;}));
    
    // Check first 'org' & 'flavor' arguments, save the value in the session cache
    foreach(array(@$_REQUEST['org'],
		  @$_REQUEST['flavor'],
		  @$_SESSION[__METHOD__]) as $theme){
      if (in_array($theme,$themes)){
	if (empty($this->theme))	$this->theme = $theme;
	$_SESSION[__METHOD__] = $this->theme;
      }
    }

    // Check then the http server name
    foreach($this->themes as $url=>$theme){
      if (in_array($theme,$themes) && (stripos($_SERVER['HTTP_HOST'],$url) !== False)){
	if (empty($this->theme)) $this->theme = $theme;
	foreach(['flavor','org'] as $arg){
	  if (class_exists('b_cnf',False) && !\b_cnf::get($arg)) \b_cnf::set($arg,$theme);
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
