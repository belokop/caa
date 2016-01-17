<?php
/*
 * JAM-4 encryption decoder, backword compatibility
 */
function bAuth_data_Login($encoded_login){
  // Force the current module being JAM
  if (defined('JAM_MODULE'))  $_GET['q'] = JAM_MODULE;
  else                        $_GET['q'] = 'jam';

  b_reg::$current_module = $_GET['q'];
  b_reg::load_module(b_reg::$current_module);

  // Start JAM
  start_JAM();
  
  list($login,$pwd) = explode(',',b_crypt::hex_decode($encoded_login),2);
  $loginData = array('org'   =>'nordita',
		     //                     'flavor'=>'nordita',
		     );
  return array($login,$pwd,$loginData);
}
