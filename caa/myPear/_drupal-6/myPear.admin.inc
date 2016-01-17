<?php
/**
 * @file
 * Admin settings forms for myPear
 */

/**
 * Implements hook_form_settings().
 */

function myPear_settings($form=array(), &$form_state=array()) {

  myPear_init();
  $form = array();

  foreach (myPear::_SETUP()->setup as $block=>$array){
    $prefix = $array['id'];
    $title  = $array['title'];
    $form[$prefix] = array('#title' => t($title),
			   '#type'  => 'fieldset'
			   );
    foreach($array as $var=>$data){
      if (is_array($data) && isset($data['name'])){
	$variable = "${prefix}_${var}";
	$form[$variable] = array('#type' => 'textfield',
				 '#title' => t($data['name']),
				 '#description' => t(str_replace('-BR-','<br/>',$data['descr'])),
				 '#default_value' => variable_get($variable,@$data['default']),
				 );
	if (!isset($data['default'])) unset($form[$variable]['#default_value']);
      }
    }
  }
  return system_settings_form($form);
}
