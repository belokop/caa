<?php
function yourtheme_preprocess_page(&$vars) {
  global $js,$vars;
  drupal_add_js("js/jquery-latest.js", 'core'); //where you store your jquery
  //drupal_add_js("js/more_javascripts.js", 'theme'); //any other js files you may have
  
  $js = drupal_add_js(NULL, NULL, 'header'); //get header js files in an array
  unset($js['core']['misc/jquery.js']); //unset default drupal jquery js

  //make our own jquery file first 
  //$js['core'] = array_reverse($js['core'], 1); 
  $js['core'] = array_slice($js['core'], -1) + array_slice($js['core'], 0, -1);
  
  $vars['scripts'] = drupal_get_js('header', $js); //create script tags and set them to $scripts
}
