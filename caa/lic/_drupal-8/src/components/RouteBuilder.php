<?php

/**
 * @file
 * Contains Drupal\lic\components\RouteBuilder
 */

namespace Drupal\lic\components;
use Symfony\Component\Routing\RouteCollection;

class RouteBuilder{
  
  public function routes(){
    static $dejaVu = 0;
    printf("%s   <br>\n",__METHOD__);
    $this->dbg(++$dejaVu);
  }

  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}
