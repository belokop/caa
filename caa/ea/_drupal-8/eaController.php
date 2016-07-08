<?php

/**
 * @file
 * Contains \Drupal\ea\eaController.
 */

namespace Drupal\ea;
use Drupal\Core\Access\AccessResult;
use Drupal\Core\Controller\ControllerBase;

/**
 * Displays content for our menu links.
 */
class eaController extends ControllerBase{

  private $q = Null;
  private $tab = Null;

  static $invocation = 0;  
  public function __construct() {
    self::$invocation++;

    require_once drupal_get_path('module','myPear').'/includes/drupal8_compat.inc';
    $this->q = $_REQUEST['q'];
    $this->extract_tab();
  }
  
  private function extract_tab(){
    // Parce 'q', extract the tab
    if (empty($this->tab)){
      ea_init();
      require_once drupal_get_path('module','myPear').'/includes/APImenu.inc';
      require_once drupal_get_path('module','ea').'/config.tabs.inc';
    
      $q = explode('/',$this->q);
      $module  = array_shift($q);
      $tab_code= (int)array_pop($q);
      $this->tab = (empty($tab_code) 
		    ? $module 
		    : APItabs_code2tab($tab_code)); 
      // $this->debug_controller(__FUNCTION__,"tab_code=$tab_code");
    }
  }
  
  public function getMenuTitle() {
    global $myPear_custom_title;

    $this->extract_tab();

    EA_MENU();
    $_ea_title_callback = trim(_ea_title_callback($this->tab,Null,True));
    $reply = (empty($myPear_custom_title)  // || (trim(strip_tags($myPear_custom_title)) != trim($myPear_custom_title))
	      ? $_ea_title_callback
	      : $myPear_custom_title);
    $debug[] = htmlspecialchars($reply);
    if ($reply == $myPear_custom_title) $debug[] = "(_ea_title_callback gives \"".htmlspecialchars($_ea_title_callback)."\")";
    $this->debug_controller(__FUNCTION__,join(' ',$debug));
    return  t($reply);
  }
  
  public function getAccess() {
    $this->extract_tab();

    EA_MENU();
    $reply = (empty($this->tab) || ($this->tab == 'ea')
	      ? True
	      : _ea_access_callback($this->tab));
    $this->debug_controller(__FUNCTION__,(bool)$reply);
    return ($reply 
	    ? AccessResult::allowed() 
	    : AccessResult::forbidden());   // AccessResult::neutral();
  }
  
  public function getPageContent() {

    //    $this->debug_controller(__FUNCTION__);
    $this->extract_tab();

    EA_MENU();
    return array(
		 '#type' => 'markup',
		 '#markup' => _ea_output($this->tab),
		 );
  }
  
  private function debug_controller($__FUNCTION__,$reply=''){
    drupal_set_message(sprintf("%s->%s(%s,%s): <em>%s</em>",
			       __CLASS__,
			       $__FUNCTION__,
			       $this->q,
			       (empty($this->tab) ? "???" : $this->tab),
			       (($reply !== False) && empty($reply) ? '' : var_export($reply,True)),
			       'info'));    
  }
}

