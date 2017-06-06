<?php

/**
 * @file
 * Contains Drupal\vm\components\RouteBuilder
 */

namespace Drupal\vm\components;
use Symfony\Component\Routing\RouteCollection;

class RouteBuilder{
  
  public function routes(){
    static $dejaVu = 0;
    printf("%s   <br>\n",__method__);
    $this->dbg(++$dejaVu);
  }

  private function dbg($text=''){
    \D8::dbg($text,$this,3);
  }
}
