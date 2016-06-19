<?php
/**
 * Employees & Alumni (EA) configuration file
 */
// if we are not in the CMS environment...
if (file_exists($f='../myPear/config.inc'))  require_once $f;

// register the module in myPear registry
$releaseDate = '2016-06-19';
define('EA_VERSION','5.23');

b_reg::add_module(EA_MODULE,array('v' =>EA_VERSION,
				  'r' =>$releaseDate,
				  'c' =>__FILE__,
				  'tm'=>'Staff',
				  'd' =>'Employees & Alumni',
				  'i' =>'i-oxford',
				  ));

// decide what we want to see - Alumni or Employees?
if (!b_cnf::get('ea_see_a') && !b_cnf::get('ea_see_e')) b_cnf::set('ea_see_a',1);  
define('ea_see_a',b_cnf::get('ea_see_a'));  if (ea_see_a) $ea_tmp[] = 'Alumni';
define('ea_see_e',b_cnf::get('ea_see_e'));  if (ea_see_e) $ea_tmp[] = 'Stuff';

define('ea_year', b_cnf::get('ea_year',date('Y',time())));
