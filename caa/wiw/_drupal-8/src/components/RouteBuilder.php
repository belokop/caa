<?php

/**
 * @file
 * Contains Drupal\wiw\components\RouteBuilder
 */

namespace Drupal\wiw\components;
use Symfony\Component\Routing\RouteCollection;

/*
 * <route_name>:
 *   path: /<path>
 *   defaults:
 *     _controller:     Drupal\\myPear\\components\\PageController::getPageContent
 *     _title_callback: Drupal\\myPear\\components\\PageController::getPageTitle
 *   requirements:
 *     _access_check_token: 'TRUE'
 */
class RouteBuilder{
  
  public function routes(){
    static $dejaVu = 0;
    if (cnf_CLI) printf("%s   <br>\n",__METHOD__);
    else         $this->dbg(++$dejaVu);
    wiw_init();
    return array();
    foreach(\WIW::_MENU()->build_menuTree() as $path=>$item){
      $routes["wiw.$path"] = new Route('<path>',
				       array('_controller'    => 'Drupal\\myPear\\components\\PageController::getPageContent',
					     '_title_callback'=> 'Drupal\\myPear\\components\\PageController::getPageTitle'),
				       array('_permission'  => 'access content'));
    }
    return $routes;
  }
  
  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}
