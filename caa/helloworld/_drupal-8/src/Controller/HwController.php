<?php
/**
 * @file
 * Contains \Drupal\hw\Controller\HwController
 */

namespace Drupal\hw\Controller;

use Drupal\Core\Controller\ControllerBase;

class HwController extends ControllerBase {
  public function content() {
    return array(
		 '#type' => 'markup',
		 '#markup' => $this->t(__METHOD__),
		 );
  }
  public function render() {
    return array(
		 '#type' => 'markup',
		 '#markup' => $this->t(hw_render()),
		 );
  }
}
