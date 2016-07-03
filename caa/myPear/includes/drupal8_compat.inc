<?php
// Tentative... Drupal8
if (class_exists('Drupal',False)){
  if (!function_exists('variable_get')){
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

  if (!function_exists('module_list')){
    function module_list(){
      $modules = \Drupal::moduleHandler()->getModuleList();
      return array_keys($modules);
    }
  }

  if (!function_exists('drupal_add_js')){
    function drupal_add_js($arg1='',$arg2=''){
      //      printf("%s(%s,%s) <br>",__FUNCTION__,$arg1,$arg2);
    }
  }
  if (!function_exists('drupal_add_css')){
    function drupal_add_css($arg1='',$arg2=''){
      //      printf("%s(%s,%s) <br>",__FUNCTION__,$arg1,$arg2);
    }
  }
}
  
