<?php

/**
 * @file
 * Contains Drupal\prp\components\RouteBuilder
 */

namespace Drupal\prp\components;
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