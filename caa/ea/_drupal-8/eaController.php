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

  private $q;
  private $tab;

  public function __construct() {
    $this->q = \Drupal::request()->query->get('q');
    $this->tab = \Drupal::request()->query->get('tab');
  }
  
  public function getTitle() {
    drupal_set_message(__FUNCTION__.'()','status');
    return  t('So you want to visit @q?', array('@q' => $this->q));
  }
  
  public function getAccess() {
    $reply = _ea_access_callback($this->tab);
    return ($reply 
	    ? AccessResult::allowed() 
	    : AccessResult::forbidden());   // AccessResult::neutral();
   return $reply;
  }
  
  public function getContent() {
    drupal_set_message(__FUNCTION__."($this->tab)",'status');
    return array(
		 '#type' => 'markup',
		 '#markup' => _ea_output($this->tab),
		 );
  }
}

