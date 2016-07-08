<?php

function ea_menu_links_discovered_alter(&$links) {
  printf("\n<!-- <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< %s -->\n",__FUNCTION__);
  drupal_set_message(sprintf("%s(array(%s))",__FUNCTION__,implode(', ',preg_grep('/^ea/',array_keys($links)))),'error');
  //  foreach(preg_grep('/^ea/',array_keys($links)) as $m) print_r($links[$m]);
  printf("\n<!-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s -->\n",__FUNCTION__);
}

function ea_menu_local_actions_alter(&$local_actions) {
  printf("\n<!-- --------------------------------------------- %s --------------------------------------------- -->\n",__FUNCTION__);
  drupal_set_message(__FUNCTION__,'error');
}

function ea_menu_local_tasks_alter(&$data, $route_name) {
  printf("\n<!-- --------------------------------------------- %s --------------------------------------------- -->\n",__FUNCTION__);
  drupal_set_message(sprintf("%s(%s, array(%s))",__FUNCTION__,$route_name,implode(',',array_keys($data))),'error');
}

function ea_contextual_links_alter(array &$links, $group, array $route_parameters) {
  drupal_set_message(sprintf("%s(array(%s))",__FUNCTION__,implode(',',array_keys($links))),'error');
}

function ea_contextual_links_plugins_alter(array &$contextual_links) {
  drupal_set_message(sprintf("%s(array(%s))",__FUNCTION__,implode(',',array_keys($contextual_links))),'error');
}

function ea_contextual_links_view_alter(&$element, $items) {
  drupal_set_message(sprintf("%s(array(%s))",__FUNCTION__,implode(',',array_keys($element))),'error');
}

function ea_link_alter(&$variables) {
  drupal_set_message(sprintf("%s(array(%s))",__FUNCTION__,implode(',',preg_grep('/^ea/',array_keys($variables)))),'error');
  drupal_set_message(sprintf("%s(array(%s))",__FUNCTION__,implode(',',array_keys($variables))),'error');
}

