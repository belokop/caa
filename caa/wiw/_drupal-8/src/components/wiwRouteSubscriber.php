<?php

namespace Drupal\wiw\components;

use Drupal\Core\Routing\RouteSubscriberBase;
use Symfony\Component\Routing\Route;
use Symfony\Component\Routing\RouteCollection;


/**
 * Listens to the dynamic wiw route events.
 */
class wiwRouteSubscriber extends RouteSubscriberBase {
  
  public function routes(RouteCollection $collection) {
    
    if (cnf_CLI) printf("%s   <br>\n",__METHOD__); else         $this->dbg($collection->get('wiw.main_menu')->getPath()); return;
    
    foreach (wiw_get_types() as $type) {      
      $route = new Route(
			 // the url path to match
			 'wiw/add/' . $type,
			 // the defaults (see the wiw.routing.yml for structure)
			 array(
			       '_title' => $type->title,
			       '_controller' => '\Drupal\wiw\Controller\TrouserController::addType',
			       'type' => $type->machine_name,
			       ),
			 // the requirements
			 array(
			       '_permission' => 'create ' . $type-type,
			       )
			 );
      // Add our route to the collection with a unique key.
      $collection->add('wiw.add.' . $type->machine_name, $route);
    }
  }
  
  /*
   *
   */
  protected function alterRoutes(RouteCollection $collection){
    
    if (cnf_CLI) printf("%s   <br>\n",__METHOD__); else         $this->dbg($collection->get('wiw.main_menu')->getPath()); return;
    
    // Find the route we want to alter
    $route = $collection->get('example.route.name');
    if (@$v1){
      // Make a change to the path.
      $route->setPath('/my/new/path');
      // Re-add the collection to override the existing route.
      $collection->add('example.route.name', $route);
    }else{
      //Make a change to the controller.
      $route->setDefault('_controller', '\Drupal\example\Controller\IndexController::changed_callback_trouser_type_add_page');
    }
  } 
  
  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }

}