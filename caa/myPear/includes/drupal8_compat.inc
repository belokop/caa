<?php
// Tentative... Drupal8
if (!function_exists('variable_get') && class_exists('Drupal',False)){
  function variable_get($var,$default=Null){
    return (\Drupal::state()->get($var) ?: $default);
  }
  function variable_set($var,$value){
    \Drupal::state()->set($var, $value);
  }
  function variable_del($var){
    \Drupal::state()->delete($var);
  }
}
