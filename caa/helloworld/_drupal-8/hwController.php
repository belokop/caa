<?php
/**
 * @file
 * Contains \Drupal\hw\hwController
 */

namespace Drupal\hw;

use Drupal\Core\Controller\ControllerBase;

class hwController extends ControllerBase {
  public function content() {
    return array(
		 '#type' => 'markup',
		 '#markup' => \Drupal\Core\Render\Markup::create(hw_render()),
		 );
  }
  public function render() {
    return array(
		 '#type' => 'markup',
		 '#markup' => \Drupal\Core\Render\Markup::create(hw_render()),
		 );
  }
}
