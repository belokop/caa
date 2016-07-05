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

    $cmt = '';
    if (empty($this->q)){
      $this->q = \Drupal::request()->query->get('q');
      $cmt = "Drupal::request()->query->get() = ".$this->q;
    }
    if (empty($this->q)){
      $this->q = $_SERVER['REQUEST_URI'];
      $cmt = ' _SERVER["REQUEST_URI"] = '.$this->q;
    }
    
    // Tidy up 'q', remove double&leading slashes
    $q = array();
    foreach(explode('/',$this->q) as $i) if (!empty($i)) $q[] = $i;
    $this->q = implode('/',$q);

    $this->extract_tab();
    $this->debug_controller(__FUNCTION__,$cmt);
  }

  private function extract_tab(){
    // Parce 'q', extract the tab
    if (empty($this->tab)){
      require_once drupal_get_path('module','myPear').'/includes/APImenu.inc';
    
      $q = explode('/',$this->q);
      $module  = array_shift($q);
      $tab_code= array_pop($q);
      $this->tab = (empty($tab_code) 
		    ? $module 
		    : APItabs_code2tab($tab_code)); //call_user_func('APItabs::code2tab',$tab_code));
      $this->debug_controller(__FUNCTION__,"tab_code=$tab_code");
    }
  }
  
  public function getTitle() {
    ea_init();
    $this->extract_tab();
    $reply = _ea_title_callback($this->tab,Null,True);
    $this->debug_controller(__FUNCTION__,$reply);
    return  t($reply);
  }
  
  public function getAccess() {
    ea_init();
    $this->extract_tab();
    $reply = (bool)(empty($this->tab) || ($this->tab == 'ea')
		    ? True
		    : _ea_access_callback($this->tab));
    $this->debug_controller(__FUNCTION__,$reply);
    return ($reply 
	    ? AccessResult::allowed() 
	    : AccessResult::forbidden());   // AccessResult::neutral();
  }
  
  public function getContent() {
    ea_init();
    $this->debug_controller(__FUNCTION__);
    $this->extract_tab();
    return array(
		 '#type' => 'markup',
		 '#markup' => _ea_output($this->tab),
		 );
  }
  
  private function debug_controller($__FUNCTION__,$reply=''){
    drupal_set_message(sprintf("%s%d->%s(%s,%s): <em>%s</em>",
			       __CLASS__,
			       self::$invocation,
			       $__FUNCTION__,
			       $this->q,
			       (empty($this->tab) ? "???" : $this->tab),
			       (empty($reply) ? '' : var_export($reply,True)),
			       'info'));    
		       }
}
