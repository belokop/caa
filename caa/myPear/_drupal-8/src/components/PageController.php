<?php

/**
 * @file
 * Contains \Drupal\myPear\components\PageController
 */

namespace Drupal\myPear\components;
use Drupal\Core\Controller\ControllerBase;

// require_once dirname(__FILE__).'/TwigExtension.php';

/**
 * Displays content for our menu links.
 */
class PageController extends ControllerBase{
  
  public function __construct($EntityManager) {
    if (!empty($EntityManager)) $this->dbg($EntityManager->getBundleInfo('menu_link_content'));
  }
  
  public function getPageTitle() {
    global $myPear_custom_title;
    //    $this->dbg();

    \D8::current_tab();    
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

    \D8::current_tab();    
    $callback = sprintf('_%s_output',$GLOBALS['myPear_current_module']);
    $content  = call_user_func($callback,$GLOBALS['myPear_current_tab']);
    $reply = array('#type' => 'markup',
                   '#markup' => \Drupal\Core\Render\Markup::create($content),
                   );
    return $reply;
  }

  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}  
