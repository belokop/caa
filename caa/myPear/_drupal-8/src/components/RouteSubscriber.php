<?php

/**
 * @file
 * Contains \Drupal\myPear\components\RouteSubscriber
 */

namespace Drupal\myPear\components;
use Drupal\Core\Routing\RouteSubscriberBase;
use Symfony\Component\Routing\RouteCollection;

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

